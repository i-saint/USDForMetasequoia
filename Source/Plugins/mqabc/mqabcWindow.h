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
    BOOL OnRecordingClicked(MQWidgetBase* sender, MQDocument doc);

private:
    mqabcPlugin* m_plugin = nullptr;
    MQButton* m_button_recording = nullptr;
};

} // namespace mqusd
