#pragma once
#include "MQWidget.h"
#include "mqCommon/mqDocumentImporter.h"
#include "mqabcInternal.h"

namespace mqusd {

class mqabcImporterWindow : public MQDialog
{
using super = MQDialog;
public:
    mqabcImporterWindow(MQBasePlugin* plugin);
    BOOL OnShow(MQWidgetBase* sender, MQDocument doc);
    BOOL OnHide(MQWidgetBase* sender, MQDocument doc);
    BOOL OnSampleEdit(MQWidgetBase* sender, MQDocument doc);
    BOOL OnSampleSlide(MQWidgetBase* sender, MQDocument doc);
    BOOL OnSettingsUpdate(MQWidgetBase* sender, MQDocument doc);
    BOOL OnOKClicked(MQWidgetBase* sender, MQDocument doc);

    void SyncSettings();
    bool Open(MQDocument doc, const std::string& path);
    bool Close();
    void Seek(MQDocument doc, int frame);
    void Refresh(MQDocument doc);
    bool IsOpened() const;

private:
    MQBasePlugin* m_plugin = nullptr;

    MQEdit* m_edit_frame = nullptr;
    MQSlider* m_slider_frame = nullptr;
    MQEdit* m_edit_scale = nullptr;

    MQCheckBox* m_check_flip_v = nullptr;
    MQCheckBox* m_check_flip_x = nullptr;
    MQCheckBox* m_check_flip_yz = nullptr;
    MQCheckBox* m_check_flip_faces = nullptr;

    MQCheckBox* m_check_merge = nullptr;
    MQCheckBox* m_check_merge_only_visible = nullptr;

    MQButton* m_button_ok = nullptr;


    ScenePtr m_scene;
    ImportOptions m_options;
    DocumentImporterPtr m_importer;
    int m_last_frame = 0;
};

} // namespace mqusd
