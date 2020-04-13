#pragma once
#include "MQWidget.h"
#include "mqCommon/mqTWindow.h"
#include "mqCommon/mqDocumentExporter.h"
#include "mqabcInternal.h"

namespace mqusd {

class mqabcRecorderWindow : public mqTWindow<mqabcRecorderWindow>
{
using super = mqTWindow<mqabcRecorderWindow>;
friend mqabcRecorderWindow* super::create(MQBasePlugin* plugin);
protected:
    mqabcRecorderWindow(MQBasePlugin* plugin, MQWindowBase& parent);

public:
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
    mqabcRecorderPlugin* m_plugin = nullptr;

    MQFrame* m_frame_settings = nullptr;
    MQEdit* m_edit_interval = nullptr;
    MQEdit* m_edit_scale = nullptr;

    MQCheckBox* m_check_mirror = nullptr;
    MQCheckBox* m_check_lathe = nullptr;
    MQCheckBox* m_check_subdiv = nullptr;

    MQCheckBox* m_check_normals = nullptr;
    MQCheckBox* m_check_colors = nullptr;
    MQCheckBox* m_check_materials = nullptr;

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
