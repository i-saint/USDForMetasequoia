#pragma once
#include "MQWidget.h"

namespace mqusd {

class mqabcPlugin;

class mqabcWindow : public MQWindow
{
public:
    mqabcWindow(mqabcPlugin* plugin, MQWindowBase& parent);

    BOOL OnShow(MQWidgetBase* sender, MQDocument doc);
    BOOL OnHide(MQWidgetBase* sender, MQDocument doc);
    BOOL OnImportClicked(MQWidgetBase* sender, MQDocument doc);
    BOOL OnInsertClicked(MQWidgetBase* sender, MQDocument doc);
    BOOL OnExportClicked(MQWidgetBase* sender, MQDocument doc);
    BOOL OnRecordingClicked(MQWidgetBase* sender, MQDocument doc);

private:
    mqabcPlugin* m_plugin = nullptr;

    MQButton* m_button_import = nullptr;
    MQButton* m_button_insert = nullptr;
    MQButton* m_button_export = nullptr;
    MQButton* m_button_recording = nullptr;
};

} // namespace mqusd
