#include "pch.h"
#include "mqusd.h"
#include "mqusdRecorderPlugin.h"
#include "mqusdRecorderWindow.h"

namespace mqusd {

mqusdRecorderWindow::mqusdRecorderWindow(mqusdRecorderPlugin* plugin, MQWindowBase& parent)
    : MQWindow(parent)
{
    setlocale(LC_ALL, "");

    m_plugin = plugin;

    SetTitle(L"USD Recorder");
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
            m_edit_scale->AddChangedEvent(this, &mqusdRecorderWindow::OnSettingsUpdate);
        }

        m_check_normals = CreateCheckBox(vf, L"Export Normals");
        m_check_normals->AddChangedEvent(this, &mqusdRecorderWindow::OnSettingsUpdate);

        m_check_colors = CreateCheckBox(vf, L"Export Vertex Colors");
        m_check_colors->AddChangedEvent(this, &mqusdRecorderWindow::OnSettingsUpdate);

        m_check_mids = CreateCheckBox(vf, L"Export Material IDs");
        m_check_mids->AddChangedEvent(this, &mqusdRecorderWindow::OnSettingsUpdate);

        m_check_blendshapes = CreateCheckBox(vf, L"Export Morphs");
        m_check_blendshapes->AddChangedEvent(this, &mqusdRecorderWindow::OnSettingsUpdate);

        m_check_skeletons = CreateCheckBox(vf, L"Export Bones");
        m_check_skeletons->AddChangedEvent(this, &mqusdRecorderWindow::OnSettingsUpdate);

        m_check_mirror = CreateCheckBox(vf, L"Freeze Mirror");
        m_check_mirror->AddChangedEvent(this, &mqusdRecorderWindow::OnSettingsUpdate);

        m_check_lathe = CreateCheckBox(vf, L"Freeze Lathe");
        m_check_lathe->AddChangedEvent(this, &mqusdRecorderWindow::OnSettingsUpdate);

        m_check_subdiv = CreateCheckBox(vf, L"Freeze Subdiv");
        m_check_subdiv->AddChangedEvent(this, &mqusdRecorderWindow::OnSettingsUpdate);

        m_check_flip_faces = CreateCheckBox(vf, L"Flip Faces");
        m_check_flip_faces->AddChangedEvent(this, &mqusdRecorderWindow::OnSettingsUpdate);

        m_check_flip_x = CreateCheckBox(vf, L"Flip X");
        m_check_flip_x->AddChangedEvent(this, &mqusdRecorderWindow::OnSettingsUpdate);

        m_check_flip_yz = CreateCheckBox(vf, L"Flip YZ");
        m_check_flip_yz->AddChangedEvent(this, &mqusdRecorderWindow::OnSettingsUpdate);

        m_check_merge = CreateCheckBox(vf, L"Merge Meshes");
        m_check_merge->AddChangedEvent(this, &mqusdRecorderWindow::OnSettingsUpdate);
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
            m_edit_interval->AddChangedEvent(this, &mqusdRecorderWindow::OnSettingsUpdate);
        }

        m_button_recording = CreateButton(vf, L"Start Recording");
        m_button_recording->AddClickEvent(this, &mqusdRecorderWindow::OnRecordingClicked);
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

//#ifdef mqusdDebug
//    {
//        MQFrame* vf = CreateVerticalFrame(this);
//        vf->SetOutSpace(outer_margin);
//        vf->SetInSpace(inner_margin);
//
//        m_button_debug = CreateButton(vf, L"Debug");
//        m_button_debug->AddClickEvent(this, &mqusdRecorderWindow::OnDebugClicked);
//    }
//#endif // mqusdDebug

    this->AddShowEvent(this, &mqusdRecorderWindow::OnShow);
    this->AddHideEvent(this, &mqusdRecorderWindow::OnHide);
}

BOOL mqusdRecorderWindow::OnShow(MQWidgetBase* sender, MQDocument doc)
{
    SyncSettings();
    return 0;
}

BOOL mqusdRecorderWindow::OnHide(MQWidgetBase* sender, MQDocument doc)
{
    m_plugin->CloseUSD();
    m_plugin->WindowClose();
    return 0;
}

BOOL mqusdRecorderWindow::OnSettingsUpdate(MQWidgetBase* sender, MQDocument doc)
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
    settings.export_blendshapes = m_check_blendshapes->GetChecked();
    settings.export_skeletons = m_check_skeletons->GetChecked();

    settings.flip_faces = m_check_flip_faces->GetChecked();
    settings.flip_x = m_check_flip_x->GetChecked();
    settings.flip_yz = m_check_flip_yz->GetChecked();
    settings.merge_meshes = m_check_merge->GetChecked();

    return 0;
}

BOOL mqusdRecorderWindow::OnRecordingClicked(MQWidgetBase* sender, MQDocument doc)
{
    if (!m_plugin->IsArchiveOpened()) {
        // show file open directory

        MQSaveFileDialog dlg(*this);
        dlg.AddFilter(L"USD File (*.usd)|*.usd");
        dlg.AddFilter(L"USD ASCII File (*.usda)|*.usda");
        dlg.SetDefaultExt(L"usd");

        auto& mqo_path = m_plugin->GetMQOPath();
        auto dir = mu::GetDirectory(mqo_path.c_str());
        auto filename = mu::GetFilename_NoExtension(mqo_path.c_str());
        if (filename.empty())
            filename = "Untitled";
        filename += ".usd";

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

#ifdef mqusdDebug
BOOL mqusdRecorderWindow::OnDebugClicked(MQWidgetBase* sender, MQDocument doc)
{
    return 0;
}
#endif // mqusdDebug

void mqusdRecorderWindow::SyncSettings()
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
    m_check_blendshapes->SetChecked(settings.export_blendshapes);
    m_check_skeletons->SetChecked(settings.export_skeletons);

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

void mqusdRecorderWindow::LogInfo(const char* message)
{
    if (m_log && message)
        m_log->SetText(mu::ToWCS(message));
}

} // namespace mqusd
