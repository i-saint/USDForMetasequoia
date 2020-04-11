#include "mqusd.h"
#include "mqCommon/mqDummyPlugins.h"

MQBasePlugin* (*_mqabcGetRecorderPlugin)();

MQBasePlugin* GetPluginClass()
{
    if (!_mqabcGetRecorderPlugin) {
        auto mod = mu::GetModule(mqabcModuleFile);
        if (!mod) {
            std::string path = mqusd::GetMiscDir();
            path += mqabcModuleFile;
            mod = mu::LoadModule(path.c_str());
        }
        if (mod) {
            (void*&)_mqabcGetRecorderPlugin = mu::GetSymbol(mod, "mqabcGetRecorderPlugin");
        }
    }
    if (_mqabcGetRecorderPlugin)
        return _mqabcGetRecorderPlugin();
    return &mqusd::DummyStationPlugin::getInstance();
}

mqusdAPI void mqusdDummy()
{
    // dummy to prevent mqsdk stripped by linker
    DWORD a, b;
    MQGetPlugInID(&a, &b);
}
