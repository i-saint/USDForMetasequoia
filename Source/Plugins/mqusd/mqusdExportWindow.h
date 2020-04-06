#pragma once
#include "MQWidget.h"
#include "mqCommon/mqusdDocumentExporter.h"
#include "mqusdWindow.h"

namespace mqusd {

class mqusdExportWindow : public mqusdTWindow<mqusdExportWindow>
{
using super = mqusdTWindow<mqusdExportWindow>;
friend class super;
private:
    mqusdExportWindow(mqusdPlugin* plugin, MQWindowBase& parent);

public:
    BOOL OnShow(MQWidgetBase* sender, MQDocument doc);
    BOOL OnHide(MQWidgetBase* sender, MQDocument doc);
    BOOL OnSettingsUpdate(MQWidgetBase* sender, MQDocument doc);
    BOOL OnRecordingClicked(MQWidgetBase* sender, MQDocument doc);

    void SyncSettings();
    void LogInfo(const char *message);

    bool Open(MQDocument doc, const std::string& v);
    bool Close();

private:
    mqusdPlugin* m_plugin = nullptr;

    MQEdit* m_edit_scale = nullptr;

    MQCheckBox* m_check_mirror = nullptr;
    MQCheckBox* m_check_lathe = nullptr;
    MQCheckBox* m_check_subdiv = nullptr;

    MQCheckBox* m_check_normals = nullptr;
    MQCheckBox* m_check_colors = nullptr;
    MQCheckBox* m_check_mids = nullptr;
    MQCheckBox* m_check_blendshapes = nullptr;
    MQCheckBox* m_check_skeletons = nullptr;

    MQCheckBox* m_check_flip_faces = nullptr;
    MQCheckBox* m_check_flip_x = nullptr;
    MQCheckBox* m_check_flip_yz = nullptr;
    MQCheckBox* m_check_merge = nullptr;

    MQButton* m_button_export = nullptr;
    MQMemo* m_log = nullptr;


    ScenePtr m_scene;
    ExportOptions m_options;
    DocumentExporterPtr m_exporter;
};

} // namespace mqusd
