#pragma once
#include "MQWidget.h"
#include "mqCommon/mqTWindow.h"
#include "mqCommon/mqDocumentImporter.h"
#include "mqusdInternal.h"

namespace mqusd {

class mqusdImporterWindow : public mqTWindow<mqusdImporterWindow>
{
using super = mqTWindow<mqusdImporterWindow>;
friend mqusdImporterWindow* super::create(MQBasePlugin* plugin);
protected:
    mqusdImporterWindow(MQBasePlugin* plugin, MQWindowBase& parent);

public:
    BOOL OnShow(MQWidgetBase* sender, MQDocument doc);
    BOOL OnHide(MQWidgetBase* sender, MQDocument doc);
    BOOL OnSampleEdit(MQWidgetBase* sender, MQDocument doc);
    BOOL OnSampleSlide(MQWidgetBase* sender, MQDocument doc);
    BOOL OnSettingsUpdate(MQWidgetBase* sender, MQDocument doc);
    BOOL OnOKClicked(MQWidgetBase* sender, MQDocument doc);

    void UpdateRelations();
    void SyncSettings();
    bool Open(MQDocument doc, const std::string& path);
    bool Close();
    void Seek(MQDocument doc, double t);
    void Refresh(MQDocument doc);
    bool IsOpened() const;
    double GetTimeRange() const;

private:
    MQBasePlugin* m_plugin = nullptr;

    MQEdit* m_edit_time = nullptr;
    MQSlider* m_slider_time = nullptr;
    MQEdit* m_edit_scale = nullptr;

    MQCheckBox* m_check_blendshapes = nullptr;
    MQCheckBox* m_check_skeletons = nullptr;
    MQCheckBox* m_check_instancers = nullptr;

    MQCheckBox* m_check_flip_v = nullptr;
    MQCheckBox* m_check_flip_x = nullptr;
    MQCheckBox* m_check_flip_yz = nullptr;
    MQCheckBox* m_check_flip_faces = nullptr;

    MQCheckBox* m_check_bake = nullptr;
    MQCheckBox* m_check_merge = nullptr;
    MQCheckBox* m_check_merge_only_visible = nullptr;

    MQButton* m_button_ok = nullptr;


    ScenePtr m_scene;
    ImportOptions m_options;
    DocumentImporterPtr m_importer;
    double m_seek_time = 0;
};

} // namespace mqusd
