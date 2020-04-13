#include "pch.h"
#include "mqusd.h"
#include "mqusdImporterWindow.h"

namespace mqusd {

mqusdImporterWindow::mqusdImporterWindow(MQBasePlugin* plugin, MQWindowBase& parent)
    : super(parent)
{
    setlocale(LC_ALL, "");

    m_plugin = plugin;

    SetTitle(L"Import USD");
    SetOutSpace(0.2);

    double outer_margin = 0.3;
    double inner_margin = 0.3;

    MQFrame* vf = CreateVerticalFrame(this);
    vf->SetInSpace(inner_margin);
    vf->SetOutSpace(outer_margin);


    {
        MQGroupBox* group = CreateGroupBox(vf, L"Time");
        MQFrame* hf = CreateHorizontalFrame(group);
        CreateLabel(hf, L"Time");
        m_edit_time = CreateEdit(hf);
        m_edit_time->SetNumeric(MQEdit::NUMERIC_INT);
        m_edit_time->SetText(L"0");
        m_edit_time->AddChangedEvent(this, &mqusdImporterWindow::OnSampleEdit);

        m_slider_time = CreateSlider(group);
        m_slider_time->AddChangingEvent(this, &mqusdImporterWindow::OnSampleSlide);
    }
    {
        MQGroupBox* group = CreateGroupBox(vf, L"Scale");

        MQFrame* hf = CreateHorizontalFrame(group);
        CreateLabel(hf, L"Scale Factor");
        m_edit_scale = CreateEdit(hf);
        m_edit_scale->SetNumeric(MQEdit::NUMERIC_DOUBLE);
        m_edit_scale->AddChangedEvent(this, &mqusdImporterWindow::OnSettingsUpdate);
    }
    {
        MQGroupBox* group = CreateGroupBox(vf, L"Components");

        m_check_blendshapes = CreateCheckBox(group, L"Import Blendshapes");
        m_check_blendshapes->AddChangedEvent(this, &mqusdImporterWindow::OnSettingsUpdate);

        m_check_skeletons = CreateCheckBox(group, L"Import Skeletons");
        m_check_skeletons->AddChangedEvent(this, &mqusdImporterWindow::OnSettingsUpdate);

        m_check_instancers = CreateCheckBox(group, L"Import Instancers");
        m_check_instancers->AddChangedEvent(this, &mqusdImporterWindow::OnSettingsUpdate);
    }
    {
        MQGroupBox* group = CreateGroupBox(vf, L"Convert Options");

        m_check_flip_v = CreateCheckBox(group, L"Flip V");
        m_check_flip_v->AddChangedEvent(this, &mqusdImporterWindow::OnSettingsUpdate);

        m_check_flip_x = CreateCheckBox(group, L"Flip X");
        m_check_flip_x->AddChangedEvent(this, &mqusdImporterWindow::OnSettingsUpdate);

        m_check_flip_yz = CreateCheckBox(group, L"Flip YZ");
        m_check_flip_yz->AddChangedEvent(this, &mqusdImporterWindow::OnSettingsUpdate);

        m_check_flip_faces = CreateCheckBox(group, L"Flip Faces");
        m_check_flip_faces->AddChangedEvent(this, &mqusdImporterWindow::OnSettingsUpdate);
    }
    {
        MQGroupBox* group = CreateGroupBox(vf, L"Bake");

        m_check_bake = CreateCheckBox(group, L"Bake Meshes");
        m_check_bake->AddChangedEvent(this, &mqusdImporterWindow::OnSettingsUpdate);

        m_check_merge = CreateCheckBox(group, L"Merge Meshes");
        m_check_merge->AddChangedEvent(this, &mqusdImporterWindow::OnSettingsUpdate);

        MQFrame* hf = CreateHorizontalFrame(group);
        MQLabel* space = CreateLabel(hf, L" ");
        space->SetWidth(32);
        m_check_merge_only_visible = CreateCheckBox(hf, L"Only Visible");
        m_check_merge_only_visible->AddChangedEvent(this, &mqusdImporterWindow::OnSettingsUpdate);
    }
    {
        m_button_ok = CreateButton(vf, L"OK");
        m_button_ok->AddClickEvent(this, &mqusdImporterWindow::OnOKClicked);
    }

    this->AddShowEvent(this, &mqusdImporterWindow::OnShow);
    this->AddHideEvent(this, &mqusdImporterWindow::OnHide);
}

BOOL mqusdImporterWindow::OnShow(MQWidgetBase* sender, MQDocument doc)
{
    SyncSettings();
    return 0;
}

BOOL mqusdImporterWindow::OnHide(MQWidgetBase* sender, MQDocument doc)
{
    Close();
    return 0;
}

BOOL mqusdImporterWindow::OnSampleEdit(MQWidgetBase* sender, MQDocument doc)
{
    auto str = mu::ToMBS(m_edit_time->GetText());
    auto value = std::atof(str.c_str());
    m_slider_time->SetPosition(value);
    Repaint(true);

    Seek(doc, value);
    return 0;
}

BOOL mqusdImporterWindow::OnSampleSlide(MQWidgetBase* sender, MQDocument doc)
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

BOOL mqusdImporterWindow::OnSettingsUpdate(MQWidgetBase* sender, MQDocument doc)
{
    UpdateRelations();

    auto& opt = m_options;

    {
        auto str = mu::ToMBS(m_edit_scale->GetText());
        auto value = std::atof(str.c_str());
        if (value != 0.0)
            opt.scale_factor = (float)value;
    }

    opt.import_blendshapes = m_check_blendshapes->GetChecked();
    opt.import_skeletons = m_check_skeletons->GetChecked();
    opt.import_instancers = m_check_instancers->GetChecked();

    opt.flip_v = m_check_flip_v->GetChecked();
    opt.flip_x = m_check_flip_x->GetChecked();
    opt.flip_yz = m_check_flip_yz->GetChecked();
    opt.flip_faces = m_check_flip_faces->GetChecked();

    opt.bake_meshes = m_check_bake->GetChecked();
    opt.merge_meshes = m_check_merge->GetChecked();
    opt.merge_only_visible = m_check_merge_only_visible->GetChecked();

    Refresh(doc);
    return 0;
}

BOOL mqusdImporterWindow::OnOKClicked(MQWidgetBase* sender, MQDocument doc)
{
    SetVisible(false);
    return true;
}

void mqusdImporterWindow::UpdateRelations()
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

void mqusdImporterWindow::SyncSettings()
{
    const size_t buf_len = 128;
    wchar_t buf[buf_len];
    auto& opt = m_options;

    swprintf(buf, buf_len, L"%.2f", opt.scale_factor);
    m_edit_scale->SetText(buf);

    m_check_blendshapes->SetChecked(opt.import_blendshapes);
    m_check_skeletons->SetChecked(opt.import_skeletons);
    m_check_instancers->SetChecked(opt.import_instancers);

    m_check_flip_v->SetChecked(opt.flip_v);
    m_check_flip_x->SetChecked(opt.flip_x);
    m_check_flip_yz->SetChecked(opt.flip_yz);
    m_check_flip_faces->SetChecked(opt.flip_faces);

    m_check_bake->SetChecked(opt.bake_meshes);
    m_check_merge->SetChecked(opt.merge_meshes);
    m_check_merge_only_visible->SetChecked(opt.merge_only_visible);

    UpdateRelations();
}

bool mqusdImporterWindow::Open(MQDocument doc, const std::string& path)
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
    m_importer->read(doc, m_scene->time_start);

    m_slider_time->SetMin(0.0);
    m_slider_time->SetMax(GetTimeRange());

    return true;
}

bool mqusdImporterWindow::Close()
{
    m_importer = {};
    m_scene = {};
    m_seek_time = 0;
    return true;
}

void mqusdImporterWindow::Seek(MQDocument doc, double t)
{
    if (!m_importer)
        return;

    m_seek_time = t;
    m_importer->read(doc, m_seek_time + m_scene->time_start);

    // repaint
    MQ_RefreshView(nullptr);
}

void mqusdImporterWindow::Refresh(MQDocument doc)
{
    Seek(doc, m_seek_time);
}

bool mqusdImporterWindow::IsOpened() const
{
    return m_scene != nullptr;
}

double mqusdImporterWindow::GetTimeRange() const
{
    return m_scene ? m_scene->time_end - m_scene->time_start : 0.0;
}


} // namespace mqusd
