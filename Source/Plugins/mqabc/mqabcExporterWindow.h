#pragma once
#include "MQWidget.h"
#include "mqCommon/mqDocumentExporter.h"
#include "mqabcInternal.h"

namespace mqusd {

class mqabcExporterWindow : public MQWindow
{
using super = MQWindow;
public:
    mqabcExporterWindow(MQBasePlugin* plugin);
    BOOL OnShow(MQWidgetBase* sender, MQDocument doc);
    BOOL OnHide(MQWidgetBase* sender, MQDocument doc);
    BOOL OnSettingsUpdate(MQWidgetBase* sender, MQDocument doc);
    BOOL OnOKClicked(MQWidgetBase* sender, MQDocument doc);

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

    MQCheckBox* m_check_flip_v = nullptr;
    MQCheckBox* m_check_flip_x = nullptr;
    MQCheckBox* m_check_flip_yz = nullptr;
    MQCheckBox* m_check_flip_faces = nullptr;

    MQCheckBox* m_check_merge = nullptr;
    MQCheckBox* m_check_merge_only_visible = nullptr;

    MQButton* m_button_ok = nullptr;


    ExportOptions m_options;
    std::wstring m_out_path;
};

} // namespace mqusd
