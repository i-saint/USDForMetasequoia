#pragma once
#include "MQWidget.h"
#include "mqCommon/mqTWindow.h"
#include "mqCommon/mqDocumentExporter.h"

namespace mqusd {

class mqusdExportWindow : public mqTWindow<mqusdExportWindow>
{
using super = mqTWindow<mqusdExportWindow>;
friend mqusdExportWindow* super::create(MQBasePlugin* plugin);
protected:
    mqusdExportWindow(MQBasePlugin* plugin, MQWindowBase& parent);

public:
    BOOL OnShow(MQWidgetBase* sender, MQDocument doc);
    BOOL OnHide(MQWidgetBase* sender, MQDocument doc);
    BOOL OnSettingsUpdate(MQWidgetBase* sender, MQDocument doc);
    BOOL OnExportClicked(MQWidgetBase* sender, MQDocument doc);

    void SyncSettings();
    void LogInfo(const char *message);

    void SetOutputPath(const std::wstring& path);
    bool DoExport(MQDocument doc);

private:
    MQBasePlugin* m_plugin = nullptr;

    MQEdit* m_edit_scale = nullptr;

    MQCheckBox* m_check_mirror = nullptr;
    MQCheckBox* m_check_lathe = nullptr;
    MQCheckBox* m_check_subdiv = nullptr;

    MQCheckBox* m_check_normals = nullptr;
    MQCheckBox* m_check_colors = nullptr;
    MQCheckBox* m_check_mids = nullptr;
    MQCheckBox* m_check_blendshapes = nullptr;
    MQCheckBox* m_check_skeletons = nullptr;

    MQCheckBox* m_check_flip_x = nullptr;
    MQCheckBox* m_check_flip_yz = nullptr;
    MQCheckBox* m_check_flip_faces = nullptr;
    MQCheckBox* m_check_merge = nullptr;

    MQButton* m_button_export = nullptr;
    MQMemo* m_log = nullptr;


    ExportOptions m_options;
    std::wstring m_out_path;
};

} // namespace mqusd
