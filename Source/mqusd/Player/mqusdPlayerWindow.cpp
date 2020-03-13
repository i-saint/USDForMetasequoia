#include "pch.h"
#include "mqusd.h"
#include "mqusdPlayerPlugin.h"
#include "mqusdPlayerWindow.h"

mqusdPlayerWindow::mqusdPlayerWindow(mqusdPlayerPlugin* plugin, MQWindowBase& parent)
    : MQWindow(parent)
{
    setlocale(LC_ALL, "");

    m_plugin = plugin;

    SetTitle(L"USD Player");
    SetOutSpace(0.4);

    double outer_margin = 0.2;
    double inner_margin = 0.1;


    {
        MQFrame* vf = CreateVerticalFrame(this);
        vf->SetOutSpace(outer_margin);
        vf->SetInSpace(inner_margin);
        m_frame_open = vf;

        m_button_open = CreateButton(vf, L"Open");
        m_button_open->AddClickEvent(this, &mqusdPlayerWindow::OnOpenClicked);
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
        m_edit_time->AddChangedEvent(this, &mqusdPlayerWindow::OnSampleEdit);

        m_slider_time = CreateSlider(vf);
        m_slider_time->AddChangingEvent(this, &mqusdPlayerWindow::OnSampleSlide);
    }
    {
        MQFrame* vf = CreateVerticalFrame(this);
        vf->SetOutSpace(outer_margin);
        vf->SetInSpace(inner_margin);

        MQFrame* hf = CreateHorizontalFrame(vf);
        CreateLabel(hf, L"Scale Factor");
        m_edit_scale = CreateEdit(hf);
        m_edit_scale->SetNumeric(MQEdit::NUMERIC_DOUBLE);
        m_edit_scale->AddChangedEvent(this, &mqusdPlayerWindow::OnSettingsUpdate);

        m_check_flip_x = CreateCheckBox(vf, L"Flip X");
        m_check_flip_x->AddChangedEvent(this, &mqusdPlayerWindow::OnSettingsUpdate);

        m_check_flip_yz = CreateCheckBox(vf, L"Flip YZ");
        m_check_flip_yz->AddChangedEvent(this, &mqusdPlayerWindow::OnSettingsUpdate);

        m_check_flip_faces = CreateCheckBox(vf, L"Flip Faces");
        m_check_flip_faces->AddChangedEvent(this, &mqusdPlayerWindow::OnSettingsUpdate);
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

    this->AddShowEvent(this, &mqusdPlayerWindow::OnShow);
    this->AddHideEvent(this, &mqusdPlayerWindow::OnHide);
}

BOOL mqusdPlayerWindow::OnShow(MQWidgetBase* sender, MQDocument doc)
{
    SyncSettings();
    return 0;
}

BOOL mqusdPlayerWindow::OnHide(MQWidgetBase* sender, MQDocument doc)
{
    m_plugin->CloseUSD();
    m_plugin->WindowClose();

    m_frame_open->SetVisible(true);
    m_frame_play->SetVisible(false);
    return 0;
}

BOOL mqusdPlayerWindow::OnOpenClicked(MQWidgetBase* sender, MQDocument doc)
{
    if (!m_plugin->IsArchiveOpened()) {
        MQOpenFileDialog dlg(*this);
        dlg.AddFilter(L"Alembic file (*.abc)|*.abc");
        dlg.SetDefaultExt(L"abc");
        if (dlg.Execute()) {
            auto path = dlg.GetFileName();
            if (m_plugin->OpenUSD(mu::ToMBS(path))) {
                m_slider_time->SetMin(m_plugin->GetTimeStart());
                m_slider_time->SetMax(m_plugin->GetTimeEnd());
                m_slider_time->SetPosition(m_plugin->GetTimeStart());

                m_frame_open->SetVisible(false);
                m_frame_play->SetVisible(true);

                auto& settings = m_plugin->GetSettings();
                if (settings.import_materials)
                    m_plugin->ImportMaterials(doc);
                m_plugin->Seek(doc, 0);
            }
        }
    }
    else {
    }

    return 0;
}

BOOL mqusdPlayerWindow::OnSampleEdit(MQWidgetBase* sender, MQDocument doc)
{
    auto str = mu::ToMBS(m_edit_time->GetText());
    auto value = std::atof(str.c_str());
    m_slider_time->SetPosition(value);

    m_plugin->Seek(doc, value);
    return 0;
}

BOOL mqusdPlayerWindow::OnSampleSlide(MQWidgetBase* sender, MQDocument doc)
{
    const size_t buf_len = 128;
    wchar_t buf[buf_len];
    auto value = m_slider_time->GetPosition();
    swprintf(buf, buf_len, L"%lf", value);
    m_edit_time->SetText(buf);

    m_plugin->Seek(doc, value);
    return 0;
}

BOOL mqusdPlayerWindow::OnSettingsUpdate(MQWidgetBase* sender, MQDocument doc)
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

    m_plugin->Refresh(doc);
    return 0;
}

void mqusdPlayerWindow::SyncSettings()
{
    const size_t buf_len = 128;
    wchar_t buf[buf_len];
    auto& settings = m_plugin->GetSettings();

    swprintf(buf, buf_len, L"%.2f", settings.scale_factor);
    m_edit_scale->SetText(buf);

    m_check_flip_x->SetChecked(settings.flip_x);
    m_check_flip_yz->SetChecked(settings.flip_yz);
    m_check_flip_faces->SetChecked(settings.flip_faces);
}

void mqusdPlayerWindow::LogInfo(const char* message)
{
    if (m_log) {
        m_log->SetText(mu::ToWCS(message));
    }
}
