#include "pch.h"
#include "mqusd.h"
#include "mqusdImportWindow.h"

namespace mqusd {

mqusdImportWindow::mqusdImportWindow(MQBasePlugin* plugin, MQWindowBase& parent)
    : super(parent)
{
    setlocale(LC_ALL, "");

    m_plugin = plugin;

    SetTitle(L"Import USD");
    SetOutSpace(0.4);

    double outer_margin = 0.2;
    double inner_margin = 0.1;


    {
        MQFrame* vf = CreateVerticalFrame(this);
        vf->SetOutSpace(outer_margin);
        vf->SetInSpace(inner_margin);

        MQFrame* hf = CreateHorizontalFrame(vf);
        CreateLabel(hf, L"Time");
        m_edit_time = CreateEdit(hf);
        m_edit_time->SetNumeric(MQEdit::NUMERIC_INT);
        m_edit_time->SetText(L"0");
        m_edit_time->AddChangedEvent(this, &mqusdImportWindow::OnSampleEdit);

        m_slider_time = CreateSlider(vf);
        m_slider_time->AddChangingEvent(this, &mqusdImportWindow::OnSampleSlide);
    }
    {
        MQFrame* vf = CreateVerticalFrame(this);
        vf->SetOutSpace(outer_margin);
        vf->SetInSpace(inner_margin);

        MQFrame* hf = CreateHorizontalFrame(vf);
        CreateLabel(hf, L"Scale Factor");
        m_edit_scale = CreateEdit(hf);
        m_edit_scale->SetNumeric(MQEdit::NUMERIC_DOUBLE);
        m_edit_scale->AddChangedEvent(this, &mqusdImportWindow::OnSettingsUpdate);

        m_check_flip_x = CreateCheckBox(vf, L"Flip X");
        m_check_flip_x->AddChangedEvent(this, &mqusdImportWindow::OnSettingsUpdate);

        m_check_flip_yz = CreateCheckBox(vf, L"Flip YZ");
        m_check_flip_yz->AddChangedEvent(this, &mqusdImportWindow::OnSettingsUpdate);

        m_check_flip_faces = CreateCheckBox(vf, L"Flip Faces");
        m_check_flip_faces->AddChangedEvent(this, &mqusdImportWindow::OnSettingsUpdate);

        m_check_blendshapes = CreateCheckBox(vf, L"Import Blendshapes");
        m_check_blendshapes->AddChangedEvent(this, &mqusdImportWindow::OnSettingsUpdate);

        m_check_skeletons = CreateCheckBox(vf, L"Import Skeletons");
        m_check_skeletons->AddChangedEvent(this, &mqusdImportWindow::OnSettingsUpdate);

        m_check_instancers = CreateCheckBox(vf, L"Import Instancers");
        m_check_instancers->AddChangedEvent(this, &mqusdImportWindow::OnSettingsUpdate);

        m_check_bake = CreateCheckBox(vf, L"Bake Meshes");
        m_check_bake->AddChangedEvent(this, &mqusdImportWindow::OnSettingsUpdate);

        m_check_merge = CreateCheckBox(vf, L"Merge Meshes");
        m_check_merge->AddChangedEvent(this, &mqusdImportWindow::OnSettingsUpdate);

        hf = CreateHorizontalFrame(vf);
        MQLabel* space = CreateLabel(hf, L" ");
        space->SetWidth(32);
        m_check_merge_only_visible = CreateCheckBox(hf, L"Only Visible");
        m_check_merge_only_visible->AddChangedEvent(this, &mqusdImportWindow::OnSettingsUpdate);
    }

    {
        MQFrame* vf = CreateVerticalFrame(this);
        vf->SetOutSpace(outer_margin);
        vf->SetInSpace(inner_margin);

        m_log = CreateMemo(vf);
        m_log->SetHorzBarStatus(MQMemo::SCROLLBAR_OFF);
        m_log->SetVertBarStatus(MQMemo::SCROLLBAR_OFF);
    }

    this->AddShowEvent(this, &mqusdImportWindow::OnShow);
    this->AddHideEvent(this, &mqusdImportWindow::OnHide);
}

BOOL mqusdImportWindow::OnShow(MQWidgetBase* sender, MQDocument doc)
{
    m_log->SetText(L"");
    SyncSettings();
    return 0;
}

BOOL mqusdImportWindow::OnHide(MQWidgetBase* sender, MQDocument doc)
{
    Close();
    return 0;
}

BOOL mqusdImportWindow::OnSampleEdit(MQWidgetBase* sender, MQDocument doc)
{
    auto str = mu::ToMBS(m_edit_time->GetText());
    auto value = std::atof(str.c_str());
    m_slider_time->SetPosition(value);
    Repaint(true);

    Seek(doc, value);
    return 0;
}

BOOL mqusdImportWindow::OnSampleSlide(MQWidgetBase* sender, MQDocument doc)
{
    const size_t buf_len = 128;
    wchar_t buf[buf_len];
    auto value = m_slider_time->GetPosition();
    swprintf(buf, buf_len, L"%lf", value);
    m_edit_time->SetText(buf);
    Repaint(true);

    Seek(doc, value);
    return 0;
}

BOOL mqusdImportWindow::OnSettingsUpdate(MQWidgetBase* sender, MQDocument doc)
{
    UpdateRelations();

    auto& opt = m_options;

    {
        auto str = mu::ToMBS(m_edit_scale->GetText());
        auto value = std::atof(str.c_str());
        if (value != 0.0)
            opt.scale_factor = (float)value;
    }

    opt.flip_x = m_check_flip_x->GetChecked();
    opt.flip_yz = m_check_flip_yz->GetChecked();
    opt.flip_faces = m_check_flip_faces->GetChecked();
    opt.import_blendshapes = m_check_blendshapes->GetChecked();
    opt.import_skeletons = m_check_skeletons->GetChecked();
    opt.import_instancers = m_check_instancers->GetChecked();
    opt.bake_meshes = m_check_bake->GetChecked();
    opt.merge_meshes = m_check_merge->GetChecked();
    opt.merge_only_visible = m_check_merge_only_visible->GetChecked();

    Refresh(doc);
    return 0;
}

void mqusdImportWindow::UpdateRelations()
{
    if (m_check_bake->GetChecked() || m_check_merge->GetChecked()) {
        m_check_blendshapes->SetEnabled(false);
        m_check_skeletons->SetEnabled(false);
    }
    else {
        m_check_blendshapes->SetEnabled(true);
        m_check_skeletons->SetEnabled(true);
    }
}

void mqusdImportWindow::SyncSettings()
{
    const size_t buf_len = 128;
    wchar_t buf[buf_len];
    auto& opt = m_options;

    swprintf(buf, buf_len, L"%.2f", opt.scale_factor);
    m_edit_scale->SetText(buf);

    m_check_flip_x->SetChecked(opt.flip_x);
    m_check_flip_yz->SetChecked(opt.flip_yz);
    m_check_flip_faces->SetChecked(opt.flip_faces);
    m_check_blendshapes->SetChecked(opt.import_blendshapes);
    m_check_skeletons->SetChecked(opt.import_skeletons);
    m_check_instancers->SetChecked(opt.import_instancers);
    m_check_bake->SetChecked(opt.bake_meshes);
    m_check_merge->SetChecked(opt.merge_meshes);
    m_check_merge_only_visible->SetChecked(opt.merge_only_visible);

    UpdateRelations();
}

void mqusdImportWindow::LogInfo(const char* message)
{
    if (m_log) {
        m_log->SetText(mu::ToWCS(message));
    }
}


bool mqusdImportWindow::Open(MQDocument doc, const std::string& path)
{
    Close();

    m_scene = CreateUSDScene();
    if (!m_scene)
        return false;

    if (!m_scene->open(path.c_str())) {
        mqusdLog(
            "failed to open %s\n"
            "it may not an usd file"
            , path.c_str());
        m_scene = {};
        return false;
    }

    m_importer.reset(new DocumentImporter(m_plugin, m_scene.get(), &m_options));
    m_importer->initialize(doc);
    m_importer->read(doc, m_scene->time_start);

    m_slider_time->SetMin(0.0);
    m_slider_time->SetMax(GetTimeRange());

    return true;
}

bool mqusdImportWindow::Close()
{
    m_importer = {};
    m_scene = {};
    m_seek_time = 0;
    return true;
}

void mqusdImportWindow::Seek(MQDocument doc, double t)
{
    if (!m_importer)
        return;

    m_seek_time = t + m_scene->time_start;
    m_importer->read(doc, m_seek_time);

    // repaint
    MQ_RefreshView(nullptr);
}

void mqusdImportWindow::Refresh(MQDocument doc)
{
    Seek(doc, m_seek_time);
}

bool mqusdImportWindow::IsOpened() const
{
    return m_scene != nullptr;
}

double mqusdImportWindow::GetTimeRange() const
{
    return m_scene ? m_scene->time_end - m_scene->time_start : 0.0;
}


} // namespace mqusd
