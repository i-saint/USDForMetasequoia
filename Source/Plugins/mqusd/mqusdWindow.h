#pragma once
#include "MQWidget.h"
#include "mqCommon/mqusdDocumentImporter.h"

namespace mqusd {

class mqusdPlugin;

class mqusdWindow : public MQWindow
{
public:
    mqusdWindow(mqusdPlugin* plugin, MQWindowBase& parent);

    BOOL OnShow(MQWidgetBase* sender, MQDocument doc);
    BOOL OnHide(MQWidgetBase* sender, MQDocument doc);
    BOOL OnImportClicked(MQWidgetBase* sender, MQDocument doc);
    BOOL OnExportClicked(MQWidgetBase* sender, MQDocument doc);
    BOOL OnRecordingClicked(MQWidgetBase* sender, MQDocument doc);

private:
    mqusdPlugin* m_plugin = nullptr;

    MQButton* m_button_import = nullptr;
    MQButton* m_button_export = nullptr;
    MQButton* m_button_recording = nullptr;
};

} // namespace mqusd
