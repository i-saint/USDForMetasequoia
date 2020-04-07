#pragma once

namespace mqusd {

template<class WindowT>
class mqTWindow : public MQWindow
{
using super = MQWindow;
public:
    static std::vector<WindowT*>& getInstances()
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

    template<class PluginT>
    static WindowT* create(PluginT* plugin)
    {
        WindowT* ret = nullptr;
        for (auto* w : getInstances()) {
            if (!w->GetVisible()) {
                ret = w;
                break;
            }
        }
        if (!ret) {
            auto parent = MQWindow::GetMainWindow();
            ret = new WindowT(plugin, parent);
        }
        ret->SetVisible(true);
        return ret;
    }

protected:
    mqTWindow(MQWindowBase& parent)
        : super(parent)
    {
        getInstances().push_back(static_cast<WindowT*>(this));
    }

public:
    ~mqTWindow()
    {
        auto& container = getInstances();
        container.erase(std::find(container.begin(), container.end(), static_cast<WindowT*>(this)));
    }
};

} // namespace mqusd
