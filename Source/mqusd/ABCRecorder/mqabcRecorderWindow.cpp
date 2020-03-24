#include "pch.h"
#include "mqusd.h"
#include "mqabcRecorderPlugin.h"
#include "mqabcRecorderWindow.h"

namespace mqusd {

mqabcRecorderWindow::mqabcRecorderWindow(mqabcRecorderPlugin* plugin, MQWindowBase& parent)
    : MQWindow(parent)
{
    setlocale(LC_ALL, "");

    m_plugin = plugin;

    SetTitle(L"Alembic Recorder");
    SetOutSpace(0.4);

    double outer_margin = 0.2;
    double inner_margin = 0.1;

    {
        MQFrame* vf = CreateVerticalFrame(this);
        vf->SetOutSpace(outer_margin);
        vf->SetInSpace(inner_margin);
        m_frame_settings = vf;

        {
            MQFrame* hf = CreateHorizontalFrame(vf);
            CreateLabel(hf, L"Scale Factor");
            m_edit_scale = CreateEdit(hf);
            m_edit_scale->SetNumeric(MQEdit::NUMERIC_DOUBLE);
            m_edit_scale->AddChangedEvent(this, &mqabcRecorderWindow::OnSettingsUpdate);
        }

        m_check_normals = CreateCheckBox(vf, L"Export Normals");
        m_check_normals->AddChangedEvent(this, &mqabcRecorderWindow::OnSettingsUpdate);

        m_check_colors = CreateCheckBox(vf, L"Export Vertex Colors");
        m_check_colors->AddChangedEvent(this, &mqabcRecorderWindow::OnSettingsUpdate);

        m_check_mids = CreateCheckBox(vf, L"Export Material IDs");
        m_check_mids->AddChangedEvent(this, &mqabcRecorderWindow::OnSettingsUpdate);

        m_check_mirror = CreateCheckBox(vf, L"Freeze Mirror");
        m_check_mirror->AddChangedEvent(this, &mqabcRecorderWindow::OnSettingsUpdate);

        m_check_lathe = CreateCheckBox(vf, L"Freeze Lathe");
        m_check_lathe->AddChangedEvent(this, &mqabcRecorderWindow::OnSettingsUpdate);

        m_check_subdiv = CreateCheckBox(vf, L"Freeze Subdiv");
        m_check_subdiv->AddChangedEvent(this, &mqabcRecorderWindow::OnSettingsUpdate);

        m_check_flip_faces = CreateCheckBox(vf, L"Flip Faces");
        m_check_flip_faces->AddChangedEvent(this, &mqabcRecorderWindow::OnSettingsUpdate);

        m_check_flip_x = CreateCheckBox(vf, L"Flip X");
        m_check_flip_x->AddChangedEvent(this, &mqabcRecorderWindow::OnSettingsUpdate);

        m_check_flip_yz = CreateCheckBox(vf, L"Flip YZ");
        m_check_flip_yz->AddChangedEvent(this, &mqabcRecorderWindow::OnSettingsUpdate);

        m_check_merge = CreateCheckBox(vf, L"Merge Meshes");
        m_check_merge->AddChangedEvent(this, &mqabcRecorderWindow::OnSettingsUpdate);
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

        std::string plugin_version = "Plugin Version: " mqusdVersionString;
        CreateLabel(vf, mu::ToWCS(plugin_version));
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
    m_plugin->CloseUSD();
    m_plugin->WindowClose();
    return 0;
}

BOOL mqabcRecorderWindow::OnSettingsUpdate(MQWidgetBase* sender, MQDocument doc)
{
    auto& settings = m_plugin->GetOptions();
    {
        auto str = mu::ToMBS(m_edit_scale->GetText());
        auto value = std::atof(str.c_str());
        if (value != 0.0)
            settings.scale_factor = (float)value;
    }
    {
        auto str = mu::ToMBS(m_edit_interval->GetText());
        auto value = std::atof(str.c_str());
        settings.capture_interval = value;
    }
    settings.freeze_mirror = m_check_mirror->GetChecked();
    settings.freeze_lathe = m_check_lathe->GetChecked();
    settings.freeze_subdiv = m_check_subdiv->GetChecked();
    settings.export_normals = m_check_normals->GetChecked();
    settings.export_colors = m_check_colors->GetChecked();
    settings.export_material_ids = m_check_mids->GetChecked();

    settings.flip_faces = m_check_flip_faces->GetChecked();
    settings.flip_x = m_check_flip_x->GetChecked();
    settings.flip_yz = m_check_flip_yz->GetChecked();
    settings.merge_meshes = m_check_merge->GetChecked();

    return 0;
}

BOOL mqabcRecorderWindow::OnRecordingClicked(MQWidgetBase* sender, MQDocument doc)
{
    if (!m_plugin->IsArchiveOpened()) {
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
            if (m_plugin->OpenUSD(doc, mu::ToMBS(path))) {
                m_plugin->CaptureFrame(doc);
            }
        }
    }
    else {
        if (m_plugin->CloseUSD()) {
        }
    }
    SyncSettings();

    return 0;
}

void mqabcRecorderWindow::SyncSettings()
{
    const size_t buf_len = 128;
    wchar_t buf[buf_len];
    auto& settings = m_plugin->GetOptions();

    swprintf(buf, buf_len, L"%.2lf", settings.capture_interval);
    m_edit_interval->SetText(buf);

    swprintf(buf, buf_len, L"%.3f", settings.scale_factor);
    m_edit_scale->SetText(buf);

    m_check_mirror->SetChecked(settings.freeze_mirror);
    m_check_lathe->SetChecked(settings.freeze_lathe);
    m_check_subdiv->SetChecked(settings.freeze_subdiv);

    m_check_normals->SetChecked(settings.export_normals);
    m_check_colors->SetChecked(settings.export_colors);
    m_check_mids->SetChecked(settings.export_material_ids);

    m_check_flip_faces->SetChecked(settings.flip_faces);
    m_check_flip_x->SetChecked(settings.flip_x);
    m_check_flip_yz->SetChecked(settings.flip_yz);
    m_check_merge->SetChecked(settings.merge_meshes);

    if (m_plugin->IsRecording()) {
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

void mqabcRecorderWindow::LogInfo(const char* message)
{
    if (m_log && message)
        m_log->SetText(mu::ToWCS(message));
}

} // namespace mqusd
