#pragma once
#include "MQWidget.h"

namespace mqusd {

class mqabcPlayerPlugin;

class mqabcPlayerWindow : public MQWindow
{
public:
    mqabcPlayerWindow(mqabcPlayerPlugin* plugin, MQWindowBase& parent);

    BOOL OnShow(MQWidgetBase* sender, MQDocument doc);
    BOOL OnHide(MQWidgetBase* sender, MQDocument doc);
    BOOL OnOpenClicked(MQWidgetBase* sender, MQDocument doc);
    BOOL OnSampleEdit(MQWidgetBase* sender, MQDocument doc);
    BOOL OnSampleSlide(MQWidgetBase* sender, MQDocument doc);
    BOOL OnSettingsUpdate(MQWidgetBase* sender, MQDocument doc);

    void SyncSettings();
    void LogInfo(const char *message);

private:
    mqabcPlayerPlugin* m_plugin = nullptr;

    MQFrame* m_frame_open = nullptr;
    MQButton* m_button_open = nullptr;

    MQFrame* m_frame_play = nullptr;
    MQEdit* m_edit_time = nullptr;
    MQSlider* m_slider_time = nullptr;

    MQEdit* m_edit_scale = nullptr;
    MQCheckBox* m_check_flip_faces = nullptr;
    MQCheckBox* m_check_flip_x = nullptr;
    MQCheckBox* m_check_flip_yz = nullptr;
    MQCheckBox* m_check_merge = nullptr;

    MQMemo* m_log = nullptr;
};

} // namespace mqusd
