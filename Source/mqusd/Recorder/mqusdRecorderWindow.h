#pragma once
#include "MQWidget.h"

namespace mqusd {

class mqusdRecorderPlugin;

class mqusdRecorderWindow : public MQWindow
{
public:
    mqusdRecorderWindow(mqusdRecorderPlugin* plugin, MQWindowBase& parent);

    BOOL OnShow(MQWidgetBase* sender, MQDocument doc);
    BOOL OnHide(MQWidgetBase* sender, MQDocument doc);
    BOOL OnIntervalChange(MQWidgetBase* sender, MQDocument doc);
    BOOL OnSettingsUpdate(MQWidgetBase* sender, MQDocument doc);
    BOOL OnRecordingClicked(MQWidgetBase* sender, MQDocument doc);
#ifdef mqusdDebug
    BOOL OnDebugClicked(MQWidgetBase* sender, MQDocument doc);
#endif // mqusdDebug

    void SyncSettings();
    void LogInfo(const char *message);

private:
    mqusdRecorderPlugin* m_plugin = nullptr;

    MQFrame* m_frame_settings = nullptr;
    MQEdit* m_edit_interval = nullptr;
    MQEdit* m_edit_scale = nullptr;
    MQCheckBox* m_check_mirror = nullptr;
    MQCheckBox* m_check_lathe = nullptr;
    MQCheckBox* m_check_subdiv = nullptr;
    MQCheckBox* m_check_normals = nullptr;
    MQCheckBox* m_check_colors = nullptr;
    MQCheckBox* m_check_mids = nullptr;
    MQButton* m_button_recording = nullptr;
    MQMemo* m_log = nullptr;
#ifdef mqusdDebug
    MQButton* m_button_debug = nullptr;
#endif // mqusdDebug
};

} // namespace mqusd
