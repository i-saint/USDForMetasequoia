#include "pch.h"
#include "mqabcRecorderPlugin.h"
#include "mqabcRecorderWindow.h"
#include "SceneGraph/ABC/sgabc.h"

namespace mqusd {

mqabcRecorderWindow::mqabcRecorderWindow(MQBasePlugin* plugin)
{
    m_plugin = dynamic_cast<mqabcRecorderPlugin*>(plugin);

    m_options.merge_meshes = true;
    m_options.merge_only_visible = true;
    m_options.export_blendshapes = false;
    m_options.export_skeletons = false;
    m_options.separate_xform = true;
    m_options.log_info = [this](const char* message) { LogInfo(message); };

    setlocale(LC_ALL, "");
    SetTitle(L"Alembic Recorder");
    SetOutSpace(0.2);

    double outer_margin = 0.3;
    double inner_margin = 0.3;

    MQFrame* vf = CreateVerticalFrame(this);
    m_frame_settings = vf;
    vf->SetOutSpace(outer_margin);
    vf->SetInSpace(inner_margin);

    {
        MQGroupBox* group = CreateGroupBox(vf, L"Scale");
        {
            MQFrame* hf = CreateHorizontalFrame(group);
            CreateLabel(hf, L"Scale Factor");
            m_edit_scale = CreateEdit(hf);
            m_edit_scale->SetNumeric(MQEdit::NUMERIC_DOUBLE);
            m_edit_scale->AddChangedEvent(this, &mqabcRecorderWindow::OnSettingsUpdate);
        }
    }
    {
        MQGroupBox* group = CreateGroupBox(vf, L"Vertex Elements");

        m_check_normals = CreateCheckBox(group, L"Export Normals");
        m_check_normals->AddChangedEvent(this, &mqabcRecorderWindow::OnSettingsUpdate);

        m_check_colors = CreateCheckBox(group, L"Export Vertex Colors");
        m_check_colors->AddChangedEvent(this, &mqabcRecorderWindow::OnSettingsUpdate);

        m_check_materials = CreateCheckBox(group, L"Export Materials");
        m_check_materials->AddChangedEvent(this, &mqabcRecorderWindow::OnSettingsUpdate);
    }
    {
        MQGroupBox* group = CreateGroupBox(vf, L"Freeze");

        m_check_mirror = CreateCheckBox(group, L"Freeze Mirror");
        m_check_mirror->AddChangedEvent(this, &mqabcRecorderWindow::OnSettingsUpdate);

        m_check_lathe = CreateCheckBox(group, L"Freeze Lathe");
        m_check_lathe->AddChangedEvent(this, &mqabcRecorderWindow::OnSettingsUpdate);

        m_check_subdiv = CreateCheckBox(group, L"Freeze Subdiv");
        m_check_subdiv->AddChangedEvent(this, &mqabcRecorderWindow::OnSettingsUpdate);
    }
    {
        MQGroupBox* group = CreateGroupBox(vf, L"Convert Options");

        m_check_flip_v = CreateCheckBox(group, L"Flip V");
        m_check_flip_v->AddChangedEvent(this, &mqabcRecorderWindow::OnSettingsUpdate);

        m_check_flip_x = CreateCheckBox(group, L"Flip X");
        m_check_flip_x->AddChangedEvent(this, &mqabcRecorderWindow::OnSettingsUpdate);

        m_check_flip_yz = CreateCheckBox(group, L"Flip YZ");
        m_check_flip_yz->AddChangedEvent(this, &mqabcRecorderWindow::OnSettingsUpdate);

        m_check_flip_faces = CreateCheckBox(group, L"Flip Faces");
        m_check_flip_faces->AddChangedEvent(this, &mqabcRecorderWindow::OnSettingsUpdate);
    }

    {
        MQFrame* vf = CreateVerticalFrame(this);
        vf->SetOutSpace(outer_margin);
        vf->SetInSpace(inner_margin);

        {
            MQFrame* hf = CreateHorizontalFrame(vf);
            CreateLabel(hf, L"Capture Interval (second)");

            m_edit_interval = CreateEdit(hf);
            m_edit_interval->SetNumeric(MQEdit::NUMERIC_DOUBLE);
            m_edit_interval->AddChangedEvent(this, &mqabcRecorderWindow::OnSettingsUpdate);
        }

        m_button_recording = CreateButton(vf, L"Start Recording");
        m_button_recording->AddClickEvent(this, &mqabcRecorderWindow::OnRecordingClicked);
    }

    {
        MQFrame* vf = CreateVerticalFrame(this);
        vf->SetOutSpace(outer_margin);
        vf->SetInSpace(inner_margin);

        m_log = CreateMemo(vf);
        m_log->SetHorzBarStatus(MQMemo::SCROLLBAR_OFF);
        m_log->SetVertBarStatus(MQMemo::SCROLLBAR_OFF);
    }

    this->AddShowEvent(this, &mqabcRecorderWindow::OnShow);
    this->AddHideEvent(this, &mqabcRecorderWindow::OnHide);
}

BOOL mqabcRecorderWindow::OnShow(MQWidgetBase* sender, MQDocument doc)
{
    SyncSettings();
    return 0;
}

BOOL mqabcRecorderWindow::OnHide(MQWidgetBase* sender, MQDocument doc)
{
    m_plugin->WindowClose();
    return 0;
}

BOOL mqabcRecorderWindow::OnSettingsUpdate(MQWidgetBase* sender, MQDocument doc)
{
    auto& opt = m_options;
    {
        auto str = mu::ToMBS(m_edit_scale->GetText());
        auto value = std::atof(str.c_str());
        if (value != 0.0)
            opt.scale_factor = (float)value;
    }
    {
        auto str = mu::ToMBS(m_edit_interval->GetText());
        auto value = std::atof(str.c_str());
        opt.capture_interval = value;
    }
    opt.freeze_mirror = m_check_mirror->GetChecked();
    opt.freeze_lathe = m_check_lathe->GetChecked();
    opt.freeze_subdiv = m_check_subdiv->GetChecked();

    opt.export_normals = m_check_normals->GetChecked();
    opt.export_colors = m_check_colors->GetChecked();
    opt.export_materials = m_check_materials->GetChecked();

    opt.flip_v = m_check_flip_v->GetChecked();
    opt.flip_x = m_check_flip_x->GetChecked();
    opt.flip_yz = m_check_flip_yz->GetChecked();
    opt.flip_faces = m_check_flip_faces->GetChecked();

    return 0;
}

BOOL mqabcRecorderWindow::OnRecordingClicked(MQWidgetBase* sender, MQDocument doc)
{
    if (!IsOpened()) {
        // show file open directory

        MQSaveFileDialog dlg(*this);
        dlg.AddFilter(L"Alembic File (*.abc)|*.abc");
        dlg.SetDefaultExt(L"usd");

        auto& mqo_path = m_plugin->GetMQOPath();
        auto dir = mu::GetDirectory(mqo_path.c_str());
        auto filename = mu::GetFilename_NoExtension(mqo_path.c_str());
        if (filename.empty())
            filename = "Untitled";
        filename += ".abc";

        if (!dir.empty())
            dlg.SetInitialDir(mu::ToWCS(dir));
        dlg.SetFileName(mu::ToWCS(filename));

        if (dlg.Execute()) {
            auto path = dlg.GetFileName();
            if (Open(doc, mu::ToMBS(path))) {
                CaptureFrame(doc);
            }
            else {
                // todo
                //MQMessageDialog();
            }
        }
    }
    else {
        if (Close()) {
            LogInfo("recording finished");
        }
    }
    SyncSettings();

    return 0;
}

void mqabcRecorderWindow::LogInfo(const char* message)
{
    m_log->SetText(mu::ToWCS(message));
}

void mqabcRecorderWindow::SyncSettings()
{
    const size_t buf_len = 128;
    wchar_t buf[buf_len];
    auto& opt = m_options;

    swprintf(buf, buf_len, L"%.2lf", opt.capture_interval);
    m_edit_interval->SetText(buf);

    swprintf(buf, buf_len, L"%.3f", opt.scale_factor);
    m_edit_scale->SetText(buf);

    m_check_mirror->SetChecked(opt.freeze_mirror);
    m_check_lathe->SetChecked(opt.freeze_lathe);
    m_check_subdiv->SetChecked(opt.freeze_subdiv);

    m_check_normals->SetChecked(opt.export_normals);
    m_check_colors->SetChecked(opt.export_colors);
    m_check_materials->SetChecked(opt.export_materials);

    m_check_flip_v->SetChecked(opt.flip_v);
    m_check_flip_x->SetChecked(opt.flip_x);
    m_check_flip_yz->SetChecked(opt.flip_yz);
    m_check_flip_faces->SetChecked(opt.flip_faces);

    if (IsRecording()) {
        SetBackColor(MQCanvasColor(255, 0, 0));
        m_button_recording->SetText(L"Stop Recording");
        m_frame_settings->SetEnabled(false);
    }
    else {
        SetBackColor(GetDefaultBackColor());
        m_button_recording->SetText(L"Start Recording");
        m_frame_settings->SetEnabled(true);
    }
}


bool mqabcRecorderWindow::Open(MQDocument doc, const std::string& path)
{
    Close();

    m_scene = CreateABCOScene();
    if (!m_scene)
        return false;

    if (!m_scene->create(path.c_str())) {
        m_scene = {};
        return false;
    }

    m_exporter.reset(new DocumentExporter(m_plugin, m_scene.get(), &m_options));
    m_exporter->initialize(doc);

    m_recording = true;
    m_dirty = true;

    LogInfo("recording started");
    return true;
}

bool mqabcRecorderWindow::Close()
{
    if (m_recording) {
        m_exporter = {};
        m_scene = {};
        m_recording = false;
    }
    return true;
}

void mqabcRecorderWindow::CaptureFrame(MQDocument doc)
{
    if (!IsRecording() || !m_dirty)
        return;

    if (m_exporter->write(doc, false)) {
        m_dirty = false;
    }
}


void mqabcRecorderWindow::MarkSceneDirty()
{
    m_dirty = true;
}

bool mqabcRecorderWindow::IsOpened() const
{
    return m_scene != nullptr;
}

bool mqabcRecorderWindow::IsRecording() const
{
    return m_exporter && m_recording;
}

} // namespace mqusd
