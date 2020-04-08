#include "pch.h"
#include "mqusd.h"
#include "mqusdPlugin.h"
#include "mqusdWindow.h"
#include "mqusdImportWindow.h"
#include "mqusdExportWindow.h"
#include "mqusdRecorderWindow.h"

namespace mqusd {

mqusdWindow::mqusdWindow(mqusdPlugin* plugin, MQWindowBase& parent)
    : MQWindow(parent)
{
    setlocale(LC_ALL, "");

    m_plugin = plugin;

    SetTitle(L"USD For Metasequoia");
    SetOutSpace(0.4);

    double outer_margin = 0.2;
    double inner_margin = 0.1;


    {
        MQFrame* vf = CreateVerticalFrame(this);
        vf->SetOutSpace(outer_margin);
        vf->SetInSpace(inner_margin);

        m_button_import = CreateButton(vf, L"Import USD");
        m_button_import->AddClickEvent(this, &mqusdWindow::OnImportClicked);

        m_button_insert = CreateButton(vf, L"Insert USD");
        m_button_insert->AddClickEvent(this, &mqusdWindow::OnInsertClicked);

        m_button_export = CreateButton(vf, L"Export USD");
        m_button_export->AddClickEvent(this, &mqusdWindow::OnExportClicked);

        m_button_recording = CreateButton(vf, L"Recording USD");
        m_button_recording->AddClickEvent(this, &mqusdWindow::OnRecordingClicked);
    }
    {
        MQFrame* vf = CreateVerticalFrame(this);
        vf->SetOutSpace(outer_margin);
        vf->SetInSpace(inner_margin);

        std::string plugin_version = "Plugin Version: " mqusdVersionString;
        CreateLabel(vf, mu::ToWCS(plugin_version));
    }

    this->AddShowEvent(this, &mqusdWindow::OnShow);
    this->AddHideEvent(this, &mqusdWindow::OnHide);
}

BOOL mqusdWindow::OnShow(MQWidgetBase* sender, MQDocument doc)
{
    return 0;
}

BOOL mqusdWindow::OnHide(MQWidgetBase* sender, MQDocument doc)
{
    m_plugin->WindowClose();
    return 0;
}

BOOL mqusdWindow::OnImportClicked(MQWidgetBase* sender, MQDocument doc)
{
    auto w = mqusdImportWindow::create(m_plugin);
    w->SetAdditive(false);
    return 0;
}

BOOL mqusdWindow::OnInsertClicked(MQWidgetBase* sender, MQDocument doc)
{
    auto w = mqusdImportWindow::create(m_plugin);
    w->SetAdditive(true);
    return 0;
}

BOOL mqusdWindow::OnExportClicked(MQWidgetBase* sender, MQDocument doc)
{
    mqusdExportWindow::create(m_plugin);
    return 0;
}

BOOL mqusdWindow::OnRecordingClicked(MQWidgetBase* sender, MQDocument doc)
{
    mqusdRecorderWindow::create(m_plugin);
    return 0;
}

} // namespace mqusd
