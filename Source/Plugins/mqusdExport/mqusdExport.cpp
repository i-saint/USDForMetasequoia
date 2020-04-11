#include "mqusd.h"
#include "mqCommon/mqDummyPlugins.h"

MQBasePlugin* (*_mqusdGetExportPlugin)();

MQBasePlugin* GetPluginClass()
{
    if (!_mqusdGetExportPlugin) {
        auto mod = mu::GetModule(mqusdModuleFile);
        if (!mod) {
            std::string path = mqusd::GetMiscDir();
            path += mqusdModuleFile;
            mod = mu::LoadModule(path.c_str());
        }
        if (mod) {
            (void*&)_mqusdGetExportPlugin = mu::GetSymbol(mod, "mqusdGetExportPlugin");
        }
    }
    if (_mqusdGetExportPlugin)
        return _mqusdGetExportPlugin();
    return &mqusd::DummyExportPlugin::getInstance();
}

mqusdAPI void mqusdDummy()
{
    // dummy to prevent mqsdk stripped by linker
    DWORD a, b;
    MQGetPlugInID(&a, &b);
}
