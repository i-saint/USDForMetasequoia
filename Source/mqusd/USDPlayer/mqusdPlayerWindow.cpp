#include "pch.h"
#include "mqusd.h"
#include "mqusdPlayerPlugin.h"
#include "mqusdPlayerWindow.h"

namespace mqusd {

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

        m_check_blendshapes = CreateCheckBox(vf, L"Import Blendshapes");
        m_check_blendshapes->AddChangedEvent(this, &mqusdPlayerWindow::OnSettingsUpdate);

        m_check_skeletons = CreateCheckBox(vf, L"Import Skeletons");
        m_check_skeletons->AddChangedEvent(this, &mqusdPlayerWindow::OnSettingsUpdate);

        m_check_bake = CreateCheckBox(vf, L"Bake Meshes");
        m_check_bake->AddChangedEvent(this, &mqusdPlayerWindow::OnSettingsUpdate);

        m_check_merge = CreateCheckBox(vf, L"Merge Meshes");
        m_check_merge->AddChangedEvent(this, &mqusdPlayerWindow::OnSettingsUpdate);
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
        dlg.AddFilter(L"USD Files (*.usd;*.usda;*.usdc;*.usdz)|*.usd;*.usda;*.usdc;*.usdz");
        dlg.AddFilter(L"All Files (*.*)|*.*");
        dlg.SetDefaultExt(L"usd");
        if (dlg.Execute()) {
            auto path = dlg.GetFileName();
            if (m_plugin->OpenUSD(doc, mu::ToMBS(path))) {
                m_slider_time->SetMin(0.0);
                m_slider_time->SetMax(m_plugin->GetTimeRange());

                m_frame_open->SetVisible(false);
                m_frame_play->SetVisible(true);
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
    Repaint(true);

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
    Repaint(true);

    m_plugin->Seek(doc, value);
    return 0;
}

BOOL mqusdPlayerWindow::OnSettingsUpdate(MQWidgetBase* sender, MQDocument doc)
{
    UpdateRelations();

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
    settings.import_blendshapes = m_check_blendshapes->GetChecked();
    settings.import_skeletons = m_check_skeletons->GetChecked();
    settings.bake_meshes = m_check_bake->GetChecked();
    settings.merge_meshes = m_check_merge->GetChecked();

    m_plugin->Refresh(doc);
    return 0;
}

void mqusdPlayerWindow::UpdateRelations()
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
    m_check_blendshapes->SetChecked(settings.import_blendshapes);
    m_check_skeletons->SetChecked(settings.import_skeletons);
    m_check_bake->SetChecked(settings.bake_meshes);
    m_check_merge->SetChecked(settings.merge_meshes);

    UpdateRelations();
}

void mqusdPlayerWindow::LogInfo(const char* message)
{
    if (m_log) {
        m_log->SetText(mu::ToWCS(message));
    }
}

} // namespace mqusd
