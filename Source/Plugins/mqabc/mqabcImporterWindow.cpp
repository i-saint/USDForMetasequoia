#include "pch.h"
#include "mqabcImporterWindow.h"
#include "SceneGraph/ABC/sgabc.h"

namespace mqusd {

mqabcImporterWindow::mqabcImporterWindow(MQBasePlugin* plugin)
    : super(MQGetMainWindow())
{
    m_plugin = plugin;

    setlocale(LC_ALL, "");
    SetTitle(L"Import Alembic");
    SetOutSpace(0.2);

    double outer_margin = 0.3;
    double inner_margin = 0.3;

    MQFrame* vf = CreateVerticalFrame(this);
    vf->SetInSpace(inner_margin);
    vf->SetOutSpace(outer_margin);

    {

        MQGroupBox* group = CreateGroupBox(vf, L"Time");
        MQFrame* hf = CreateHorizontalFrame(group);
        CreateLabel(hf, L"Frame");
        m_edit_frame = CreateEdit(hf);
        m_edit_frame->SetNumeric(MQEdit::NUMERIC_INT);
        m_edit_frame->SetText(L"1");
        m_edit_frame->AddChangedEvent(this, &mqabcImporterWindow::OnSampleEdit);

        m_slider_frame = CreateSlider(group);
        m_slider_frame->AddChangingEvent(this, &mqabcImporterWindow::OnSampleSlide);
    }
    {
        MQGroupBox* group = CreateGroupBox(vf, L"Scale");
        MQFrame* hf = CreateHorizontalFrame(group);
        CreateLabel(hf, L"Scale Factor");
        m_edit_scale = CreateEdit(hf);
        m_edit_scale->SetNumeric(MQEdit::NUMERIC_DOUBLE);
        m_edit_scale->AddChangedEvent(this, &mqabcImporterWindow::OnSettingsUpdate);
    }
    {
        MQGroupBox* group = CreateGroupBox(vf, L"Convert Options");

        m_check_flip_v = CreateCheckBox(group, L"Flip V");
        m_check_flip_v->AddChangedEvent(this, &mqabcImporterWindow::OnSettingsUpdate);

        m_check_flip_x = CreateCheckBox(group, L"Flip X");
        m_check_flip_x->AddChangedEvent(this, &mqabcImporterWindow::OnSettingsUpdate);

        m_check_flip_yz = CreateCheckBox(group, L"Flip YZ");
        m_check_flip_yz->AddChangedEvent(this, &mqabcImporterWindow::OnSettingsUpdate);

        m_check_flip_faces = CreateCheckBox(group, L"Flip Faces");
        m_check_flip_faces->AddChangedEvent(this, &mqabcImporterWindow::OnSettingsUpdate);

        m_check_merge = CreateCheckBox(group, L"Merge Meshes");
        m_check_merge->AddChangedEvent(this, &mqabcImporterWindow::OnSettingsUpdate);

        MQFrame* hf = CreateHorizontalFrame(group);
        MQLabel* space = CreateLabel(hf, L" ");
        space->SetWidth(32);
        m_check_merge_only_visible = CreateCheckBox(hf, L"Only Visible");
        m_check_merge_only_visible->AddChangedEvent(this, &mqabcImporterWindow::OnSettingsUpdate);
    }
    {
        m_button_ok = CreateButton(vf, L"OK");
        m_button_ok->AddClickEvent(this, &mqabcImporterWindow::OnOKClicked);
    }

    this->AddShowEvent(this, &mqabcImporterWindow::OnShow);
    this->AddHideEvent(this, &mqabcImporterWindow::OnHide);
}

BOOL mqabcImporterWindow::OnShow(MQWidgetBase* sender, MQDocument doc)
{
    SyncSettings();
    return 0;
}

BOOL mqabcImporterWindow::OnHide(MQWidgetBase* sender, MQDocument doc)
{
    Close();
    return 0;
}

BOOL mqabcImporterWindow::OnSampleEdit(MQWidgetBase* sender, MQDocument doc)
{
    std::string str = mu::ToMBS(m_edit_frame->GetText());
    int value = std::atoi(str.c_str());
    m_slider_frame->SetPosition((double)value);
    Repaint(true);

    Seek(doc, value);
    return 0;
}

BOOL mqabcImporterWindow::OnSampleSlide(MQWidgetBase* sender, MQDocument doc)
{
    const size_t buf_len = 128;
    wchar_t buf[buf_len];
    int value = (int)m_slider_frame->GetPosition();
    swprintf(buf, buf_len, L"%d", value);
    m_edit_frame->SetText(buf);
    Repaint(true);

    Seek(doc, value);
    return 0;
}

BOOL mqabcImporterWindow::OnSettingsUpdate(MQWidgetBase* sender, MQDocument doc)
{
    auto& opt = m_options;

    {
        auto str = mu::ToMBS(m_edit_scale->GetText());
        auto value = std::atof(str.c_str());
        if (value != 0.0)
            opt.scale_factor = (float)value;
    }

    opt.flip_v = m_check_flip_v->GetChecked();
    opt.flip_x = m_check_flip_x->GetChecked();
    opt.flip_yz = m_check_flip_yz->GetChecked();
    opt.flip_faces = m_check_flip_faces->GetChecked();

    opt.merge_meshes = m_check_merge->GetChecked();
    opt.merge_only_visible = m_check_merge_only_visible->GetChecked();

    Refresh(doc);
    return 0;
}

BOOL mqabcImporterWindow::OnOKClicked(MQWidgetBase* sender, MQDocument doc)
{
    SetVisible(false);
    return true;
}

void mqabcImporterWindow::SyncSettings()
{
    const size_t buf_len = 128;
    wchar_t buf[buf_len];
    auto& opt = m_options;

    swprintf(buf, buf_len, L"%.2f", opt.scale_factor);
    m_edit_scale->SetText(buf);

    m_check_flip_v->SetChecked(opt.flip_v);
    m_check_flip_x->SetChecked(opt.flip_x);
    m_check_flip_yz->SetChecked(opt.flip_yz);
    m_check_flip_faces->SetChecked(opt.flip_faces);

    m_check_merge->SetChecked(opt.merge_meshes);
    m_check_merge_only_visible->SetChecked(opt.merge_only_visible);
}

bool mqabcImporterWindow::Open(MQDocument doc, const std::string& path)
{
    Close();

    m_scene = CreateABCIScene();
    if (!m_scene) {
        MQShowError("Failed to create Alembic scene.");
        return false;
    }

    if (!m_scene->open(path.c_str())) {
        m_scene = {};

        MQShowError("Failed to open Alembic file.\nPossible reason: path contains multi-byte characters.");
        return false;
    }

    m_importer.reset(new DocumentImporter(m_plugin, m_scene.get(), &m_options));
    m_importer->read(doc, m_scene->time_start);

    if (m_scene->frame_count > 0) {
        m_slider_frame->SetMin(1.0);
        m_slider_frame->SetMax(double(m_scene->frame_count));
    }
    else {
        m_slider_frame->SetMin(1.0);
        m_slider_frame->SetMax(1.0);
    }

    return true;
}

bool mqabcImporterWindow::Close()
{
    m_importer = {};
    m_scene = {};
    m_last_frame = 0;
    return true;
}

void mqabcImporterWindow::Seek(MQDocument doc, int frame)
{
    if (!m_importer)
        return;

    m_last_frame = frame;
    m_importer->read(doc, m_scene->frameToTime(frame - 1));

    // repaint
    MQ_RefreshView(nullptr);
}

void mqabcImporterWindow::Refresh(MQDocument doc)
{
    Seek(doc, m_last_frame);
}

bool mqabcImporterWindow::IsOpened() const
{
    return m_scene != nullptr;
}

} // namespace mqusd
