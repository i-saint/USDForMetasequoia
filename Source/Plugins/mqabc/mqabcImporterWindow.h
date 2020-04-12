#pragma once
#include "MQWidget.h"
#include "mqCommon/mqTWindow.h"
#include "mqCommon/mqDocumentImporter.h"
#include "mqabcInternal.h"

namespace mqusd {

class mqabcImporterWindow : public mqTWindow<mqabcImporterWindow>
{
using super = mqTWindow<mqabcImporterWindow>;
friend mqabcImporterWindow* super::create(MQBasePlugin* plugin);
protected:
    mqabcImporterWindow(MQBasePlugin* plugin, MQWindowBase& parent);

public:
    BOOL OnShow(MQWidgetBase* sender, MQDocument doc);
    BOOL OnHide(MQWidgetBase* sender, MQDocument doc);
    BOOL OnSampleEdit(MQWidgetBase* sender, MQDocument doc);
    BOOL OnSampleSlide(MQWidgetBase* sender, MQDocument doc);
    BOOL OnSettingsUpdate(MQWidgetBase* sender, MQDocument doc);

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

    MQCheckBox* m_check_flip_x = nullptr;
    MQCheckBox* m_check_flip_yz = nullptr;
    MQCheckBox* m_check_flip_faces = nullptr;

    MQCheckBox* m_check_merge = nullptr;
    MQCheckBox* m_check_merge_only_visible = nullptr;


    ScenePtr m_scene;
    ImportOptions m_options;
    DocumentImporterPtr m_importer;
    double m_seek_time = 0;
};

} // namespace mqusd
