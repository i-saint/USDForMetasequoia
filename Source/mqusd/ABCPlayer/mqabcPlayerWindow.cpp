#include "pch.h"
#include "mqusd.h"
#include "mqabcPlayerPlugin.h"
#include "mqabcPlayerWindow.h"

namespace mqusd {

mqabcPlayerWindow::mqabcPlayerWindow(mqabcPlayerPlugin* plugin, MQWindowBase& parent)
    : MQWindow(parent)
{
    setlocale(LC_ALL, "");

    m_plugin = plugin;

    SetTitle(L"Alembic Player");
    SetOutSpace(0.4);

    double outer_margin = 0.2;
    double inner_margin = 0.1;


    {
        MQFrame* vf = CreateVerticalFrame(this);
        vf->SetOutSpace(outer_margin);
        vf->SetInSpace(inner_margin);
        m_frame_open = vf;

        m_button_open = CreateButton(vf, L"Open");
        m_button_open->AddClickEvent(this, &mqabcPlayerWindow::OnOpenClicked);
    }
    {
        MQFrame* vf = CreateVerticalFrame(this);
        vf->SetOutSpace(outer_margin);
        vf->SetInSpace(inner_margin);
        vf->SetVisible(false);
        m_frame_play = vf;

        MQFrame* hf = CreateHorizontalFrame(vf);
        CreateLabel(hf, L"Time");
        m_edit_time = CreateEdit(hf);
        m_edit_time->SetNumeric(MQEdit::NUMERIC_INT);
        m_edit_time->SetText(L"0");
        m_edit_time->AddChangedEvent(this, &mqabcPlayerWindow::OnSampleEdit);

        m_slider_time = CreateSlider(vf);
        m_slider_time->AddChangingEvent(this, &mqabcPlayerWindow::OnSampleSlide);
    }
    {
        MQFrame* vf = CreateVerticalFrame(this);
        vf->SetOutSpace(outer_margin);
        vf->SetInSpace(inner_margin);

        MQFrame* hf = CreateHorizontalFrame(vf);
        CreateLabel(hf, L"Scale Factor");
        m_edit_scale = CreateEdit(hf);
        m_edit_scale->SetNumeric(MQEdit::NUMERIC_DOUBLE);
        m_edit_scale->AddChangedEvent(this, &mqabcPlayerWindow::OnSettingsUpdate);

        m_check_flip_x = CreateCheckBox(vf, L"Flip X");
        m_check_flip_x->AddChangedEvent(this, &mqabcPlayerWindow::OnSettingsUpdate);

        m_check_flip_yz = CreateCheckBox(vf, L"Flip YZ");
        m_check_flip_yz->AddChangedEvent(this, &mqabcPlayerWindow::OnSettingsUpdate);

        m_check_flip_faces = CreateCheckBox(vf, L"Flip Faces");
        m_check_flip_faces->AddChangedEvent(this, &mqabcPlayerWindow::OnSettingsUpdate);

        m_check_merge = CreateCheckBox(vf, L"Merge Meshes");
        m_check_merge->AddChangedEvent(this, &mqabcPlayerWindow::OnSettingsUpdate);
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

    this->AddShowEvent(this, &mqabcPlayerWindow::OnShow);
    this->AddHideEvent(this, &mqabcPlayerWindow::OnHide);
}

BOOL mqabcPlayerWindow::OnShow(MQWidgetBase* sender, MQDocument doc)
{
    SyncSettings();
    return 0;
}

BOOL mqabcPlayerWindow::OnHide(MQWidgetBase* sender, MQDocument doc)
{
    m_plugin->CloseUSD();
    m_plugin->WindowClose();

    m_frame_open->SetVisible(true);
    m_frame_play->SetVisible(false);
    return 0;
}

BOOL mqabcPlayerWindow::OnOpenClicked(MQWidgetBase* sender, MQDocument doc)
{
    if (!m_plugin->IsArchiveOpened()) {
        MQOpenFileDialog dlg(*this);
        dlg.AddFilter(L"Alembic Files (*.abc)|*.abc");
        dlg.AddFilter(L"All Files (*.*)|*.*");
        dlg.SetDefaultExt(L"usd");
        if (dlg.Execute()) {
            auto path = dlg.GetFileName();
            if (m_plugin->OpenUSD(doc, mu::ToMBS(path))) {
                m_slider_time->SetMin(m_plugin->GetTimeStart());
                m_slider_time->SetMax(m_plugin->GetTimeEnd());
                m_slider_time->SetPosition(m_plugin->GetTimeStart());

                m_frame_open->SetVisible(false);
                m_frame_play->SetVisible(true);
            }
        }
    }
    else {
    }

    return 0;
}

BOOL mqabcPlayerWindow::OnSampleEdit(MQWidgetBase* sender, MQDocument doc)
{
    auto str = mu::ToMBS(m_edit_time->GetText());
    auto value = std::atof(str.c_str());
    m_slider_time->SetPosition(value);

    m_plugin->Seek(doc, value);
    return 0;
}

BOOL mqabcPlayerWindow::OnSampleSlide(MQWidgetBase* sender, MQDocument doc)
{
    const size_t buf_len = 128;
    wchar_t buf[buf_len];
    auto value = m_slider_time->GetPosition();
    swprintf(buf, buf_len, L"%lf", value);
    m_edit_time->SetText(buf);

    m_plugin->Seek(doc, value);
    return 0;
}

BOOL mqabcPlayerWindow::OnSettingsUpdate(MQWidgetBase* sender, MQDocument doc)
{
    auto& settings = m_plugin->GetSettings();

    {
        auto str = mu::ToMBS(m_edit_scale->GetText());
        auto value = std::atof(str.c_str());
        if (value != 0.0) {
            auto& settings = m_plugin->GetSettings();
            settings.scale_factor = (float)value;
        }
    }

    settings.flip_x = m_check_flip_x->GetChecked();
    settings.flip_yz = m_check_flip_yz->GetChecked();
    settings.flip_faces = m_check_flip_faces->GetChecked();
    settings.merge_meshes = m_check_merge->GetChecked();

    m_plugin->Refresh(doc);
    return 0;
}

void mqabcPlayerWindow::SyncSettings()
{
    const size_t buf_len = 128;
    wchar_t buf[buf_len];
    auto& settings = m_plugin->GetSettings();

    swprintf(buf, buf_len, L"%.2f", settings.scale_factor);
    m_edit_scale->SetText(buf);

    m_check_flip_x->SetChecked(settings.flip_x);
    m_check_flip_yz->SetChecked(settings.flip_yz);
    m_check_flip_faces->SetChecked(settings.flip_faces);
    m_check_merge->SetChecked(settings.merge_meshes);
}

void mqabcPlayerWindow::LogInfo(const char* message)
{
    if (m_log) {
        m_log->SetText(mu::ToWCS(message));
    }
}

} // namespace mqusd
