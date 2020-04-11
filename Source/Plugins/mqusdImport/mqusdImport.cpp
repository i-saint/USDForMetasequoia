#include "mqusd.h"
#include "mqCommon/mqDummyPlugins.h"

MQBasePlugin* (*_mqusdGetImportPlugin)();

MQBasePlugin* GetPluginClass()
{
    if (!_mqusdGetImportPlugin) {
        auto mod = mu::GetModule(mqusdModuleFile);
        if (!mod) {
            std::string path = mqusd::GetMiscDir();
            path += mqusdModuleFile;
            mod = mu::LoadModule(path.c_str());
        }
        if (mod) {
            (void*&)_mqusdGetImportPlugin = mu::GetSymbol(mod, "mqusdGetImportPlugin");
        }
    }
    if (_mqusdGetImportPlugin)
        return _mqusdGetImportPlugin();
    return &mqusd::DummyImportPlugin::getInstance();
}

mqusdAPI void mqusdDummy()
{
    // dummy to prevent mqsdk stripped by linker
    DWORD a, b;
    MQGetPlugInID(&a, &b);
}
