#pragma once
#include "MQWidget.h"
#include "Foundation/mqusdDocumentExporter.h"

namespace mqusd {

class mqabcPlayerPlugin;

class mqabcRecorderWindow : public MQWindow
{
public:
    static void each(const std::function<void (mqabcRecorderWindow*)>& body);

    mqabcRecorderWindow(mqabcPlayerPlugin* plugin, MQWindowBase& parent);
    ~mqabcRecorderWindow();

    BOOL OnShow(MQWidgetBase* sender, MQDocument doc);
    BOOL OnHide(MQWidgetBase* sender, MQDocument doc);
    BOOL OnSettingsUpdate(MQWidgetBase* sender, MQDocument doc);
    BOOL OnRecordingClicked(MQWidgetBase* sender, MQDocument doc);

    void SyncSettings();
    void LogInfo(const char *message);

    bool Open(MQDocument doc, const std::string& v);
    bool Close();
    void CaptureFrame(MQDocument doc);
    void MarkSceneDirty();
    bool IsOpened() const;
    bool IsRecording() const;

private:
    mqabcPlayerPlugin* m_plugin = nullptr;

    MQFrame* m_frame_settings = nullptr;
    MQEdit* m_edit_interval = nullptr;
    MQEdit* m_edit_scale = nullptr;

    MQCheckBox* m_check_mirror = nullptr;
    MQCheckBox* m_check_lathe = nullptr;
    MQCheckBox* m_check_subdiv = nullptr;

    MQCheckBox* m_check_normals = nullptr;
    MQCheckBox* m_check_colors = nullptr;
    MQCheckBox* m_check_mids = nullptr;

    MQCheckBox* m_check_flip_faces = nullptr;
    MQCheckBox* m_check_flip_x = nullptr;
    MQCheckBox* m_check_flip_yz = nullptr;
    MQCheckBox* m_check_merge = nullptr;

    MQButton* m_button_recording = nullptr;
    MQMemo* m_log = nullptr;
#ifdef mqusdDebug
    MQButton* m_button_debug = nullptr;
#endif // mqusdDebug


    ScenePtr m_scene;
    ExportOptions m_options;
    DocumentExporterPtr m_exporter;
    bool m_dirty = false;
    bool m_recording = false;
};

} // namespace mqusd
