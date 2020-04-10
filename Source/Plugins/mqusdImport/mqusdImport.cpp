#include "mqusd.h"
#include "mqsdk/sdk_Include.h"
#include "MeshUtils/MeshUtils.h"

MQBasePlugin* (*_mqusdGetImportPlugin)();

MQBasePlugin* GetPluginClass()
{
    if (!_mqusdGetImportPlugin) {
        auto mod = mu::GetModule(mqusdPluginFile);
        if (!mod) {
            std::string path = mqusd::GetPluginsDir();
            path += "Station" muPathSep mqusdPluginFile;
            mod = mu::LoadModule(path.c_str());
        }
        if (mod) {
            (void*&)_mqusdGetImportPlugin = mu::GetSymbol(mod, "mqusdGetImportPlugin");
        }
    }
    if (_mqusdGetImportPlugin)
        return _mqusdGetImportPlugin();
    return nullptr;
}

mqusdAPI void mqusdDummy()
{
    // dummy to prevent mqsdk stripped by linker
    DWORD a, b;
    MQGetPlugInID(&a, &b);
}
