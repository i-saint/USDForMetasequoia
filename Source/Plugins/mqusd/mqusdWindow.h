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



template<class WindowT>
class mqusdTWindow : public MQWindow
{
using super = MQWindow;
public:
    static std::vector<WindowT*> getInstances()
    {
        static std::vector<WindowT*> s_instances;
        return s_instances;
    }

    template<class Body>
    static void each(const Body& body)
    {
        static std::vector<WindowT*> s_tmp;
        s_tmp = getInstances(); // to avoid crash when s_instances is modified in the loop
        for (auto* i : s_tmp)
            body(i);
    }

    static WindowT* create(mqusdPlugin* plugin)
    {
        WindowT* ret = nullptr;
        for (auto* w : getInstances()) {
            if (!w->GetVisible()) {
                ret = w;
                break;
            }
        }
        if (!ret)
            ret = new WindowT(plugin, MQWindow::GetMainWindow());
        ret->SetVisible(true);
        return ret;
    }

protected:
    mqusdTWindow(MQWindowBase& parent)
        : super(parent)
    {
        getInstances().push_back(static_cast<WindowT*>(this));
    }

public:
    ~mqusdTWindow()
    {
        auto& container = getInstances();
        container.erase(std::find(container.begin(), container.end(), static_cast<WindowT*>(this)));
    }
};


} // namespace mqusd
