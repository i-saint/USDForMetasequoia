#pragma once
#include "MQWidget.h"
#include "mqCommon/mqTWindow.h"
#include "mqCommon/mqDocumentExporter.h"
#include "mqabcInternal.h"

namespace mqusd {

class mqabcExporterWindow : public mqTWindow<mqabcExporterWindow>
{
using super = mqTWindow<mqabcExporterWindow>;
friend mqabcExporterWindow* super::create(MQBasePlugin* plugin);
protected:
    mqabcExporterWindow(MQBasePlugin* plugin, MQWindowBase& parent);

public:
    BOOL OnShow(MQWidgetBase* sender, MQDocument doc);
    BOOL OnHide(MQWidgetBase* sender, MQDocument doc);
    BOOL OnSettingsUpdate(MQWidgetBase* sender, MQDocument doc);
    BOOL OnExportClicked(MQWidgetBase* sender, MQDocument doc);

    void SyncSettings();
    void SetOutputPath(const std::wstring& path);
    bool DoExport(MQDocument doc);

private:
    MQBasePlugin* m_plugin = nullptr;

    MQFrame* m_frame_settings = nullptr;
    MQEdit* m_edit_scale = nullptr;

    MQCheckBox* m_check_mirror = nullptr;
    MQCheckBox* m_check_lathe = nullptr;
    MQCheckBox* m_check_subdiv = nullptr;

    MQCheckBox* m_check_flip_x = nullptr;
    MQCheckBox* m_check_flip_yz = nullptr;
    MQCheckBox* m_check_flip_faces = nullptr;
    MQCheckBox* m_check_merge = nullptr;
    MQCheckBox* m_check_merge_only_visible = nullptr;

    MQButton* m_button_export = nullptr;


    ExportOptions m_options;
    std::wstring m_out_path;
};

} // namespace mqusd