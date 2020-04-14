#pragma once
#include "MQWidget.h"
#include "mqCommon/mqDocumentExporter.h"
#include "mqusdInternal.h"

namespace mqusd {

class mqusdRecorderWindow : public MQWindow
{
using super = MQWindow;
public:
    mqusdRecorderWindow(MQBasePlugin* plugin);
    BOOL OnShow(MQWidgetBase* sender, MQDocument doc);
    BOOL OnHide(MQWidgetBase* sender, MQDocument doc);
    BOOL OnSettingsUpdate(MQWidgetBase* sender, MQDocument doc);
    BOOL OnRecordingClicked(MQWidgetBase* sender, MQDocument doc);

    void LogInfo(const char* message);
    void SyncSettings();

    bool Open(MQDocument doc, const std::string& v);
    bool Close();
    void CaptureFrame(MQDocument doc);
    void MarkSceneDirty();
    bool IsOpened() const;
    bool IsRecording() const;

private:
    mqusdRecorderPlugin* m_plugin = nullptr;

    MQFrame* m_frame_settings = nullptr;
    MQEdit* m_edit_interval = nullptr;
    MQEdit* m_edit_scale = nullptr;

    MQCheckBox* m_check_normals = nullptr;
    MQCheckBox* m_check_colors = nullptr;
    MQCheckBox* m_check_materials = nullptr;

    MQCheckBox* m_check_mirror = nullptr;
    MQCheckBox* m_check_lathe = nullptr;
    MQCheckBox* m_check_subdiv = nullptr;

    MQCheckBox* m_check_flip_v = nullptr;
    MQCheckBox* m_check_flip_x = nullptr;
    MQCheckBox* m_check_flip_yz = nullptr;
    MQCheckBox* m_check_flip_faces = nullptr;

    MQButton* m_button_recording = nullptr;
    MQMemo* m_log = nullptr;


    ScenePtr m_scene;
    ExportOptions m_options;
    DocumentExporterPtr m_exporter;
    bool m_dirty = false;
    bool m_recording = false;
};

} // namespace mqusd
