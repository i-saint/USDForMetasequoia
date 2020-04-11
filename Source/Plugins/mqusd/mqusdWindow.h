#pragma once
#include "MQWidget.h"

namespace mqusd {

class mqusdPlugin;

class mqusdWindow : public MQWindow
{
public:
    mqusdWindow(mqusdPlugin* plugin, MQWindowBase& parent);

    BOOL OnShow(MQWidgetBase* sender, MQDocument doc);
    BOOL OnHide(MQWidgetBase* sender, MQDocument doc);
    BOOL OnRecordingClicked(MQWidgetBase* sender, MQDocument doc);

private:
    mqusdPlugin* m_plugin = nullptr;

    MQButton* m_button_recording = nullptr;
};

} // namespace mqusd
