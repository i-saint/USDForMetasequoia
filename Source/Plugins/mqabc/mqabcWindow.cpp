#include "pch.h"
#include "mqusd.h"
#include "mqabcPlugin.h"
#include "mqabcWindow.h"
#include "mqabcImportWindow.h"
#include "mqabcExportWindow.h"
#include "mqabcRecorderWindow.h"

namespace mqusd {

mqabcWindow::mqabcWindow(mqabcPlugin* plugin, MQWindowBase& parent)
    : MQWindow(parent)
{
    setlocale(LC_ALL, "");

    m_plugin = plugin;

    SetTitle(L"Alembic For Metasequoia");
    SetOutSpace(0.4);

    double outer_margin = 0.2;
    double inner_margin = 0.1;


    {
        MQFrame* vf = CreateVerticalFrame(this);
        vf->SetOutSpace(outer_margin);
        vf->SetInSpace(inner_margin);

        m_button_recording = CreateButton(vf, L"Recording Alembic");
        m_button_recording->AddClickEvent(this, &mqabcWindow::OnRecordingClicked);
    }
    {
        MQFrame* vf = CreateVerticalFrame(this);
        vf->SetOutSpace(outer_margin);
        vf->SetInSpace(inner_margin);

        std::string plugin_version = "Plugin Version: " mqusdVersionString;
        CreateLabel(vf, mu::ToWCS(plugin_version));
    }

    this->AddShowEvent(this, &mqabcWindow::OnShow);
    this->AddHideEvent(this, &mqabcWindow::OnHide);
}

BOOL mqabcWindow::OnShow(MQWidgetBase* sender, MQDocument doc)
{
    return 0;
}

BOOL mqabcWindow::OnHide(MQWidgetBase* sender, MQDocument doc)
{
    m_plugin->WindowClose();
    return 0;
}

BOOL mqabcWindow::OnRecordingClicked(MQWidgetBase* sender, MQDocument doc)
{
    mqabcRecorderWindow::create(m_plugin);
    return 0;
}

} // namespace mqusd
