#include "mqusd.h"
#include "mqsdk/sdk_Include.h"
#include "MeshUtils/MeshUtils.h"

MQBasePlugin* (*_mqabcGetImportPlugin)();

MQBasePlugin* GetPluginClass()
{
    if (!_mqabcGetImportPlugin) {
        auto mod = mu::GetModule(mqabcPluginFile);
        if (!mod) {
            std::string path = mqusd::GetPluginsDir();
            path += "Misc" muPathSep mqabcPluginFile;
            mod = mu::LoadModule(path.c_str());
        }
        if (mod) {
            (void*&)_mqabcGetImportPlugin = mu::GetSymbol(mod, "mqabcGetImportPlugin");
        }
    }
    if (_mqabcGetImportPlugin)
        return _mqabcGetImportPlugin();
    return nullptr;
}

mqabcAPI void mqabcDummy()
{
    // dummy to prevent mqsdk stripped by linker
    DWORD a, b;
    MQGetPlugInID(&a, &b);
}
