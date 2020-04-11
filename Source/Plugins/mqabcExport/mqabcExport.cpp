#include "mqusd.h"
#include "mqsdk/sdk_Include.h"
#include "MeshUtils/MeshUtils.h"

MQBasePlugin* (*_mqabcGetExportPlugin)();

MQBasePlugin* GetPluginClass()
{
    if (!_mqabcGetExportPlugin) {
        auto mod = mu::GetModule(mqabcPluginFile);
        if (!mod) {
            std::string path = mqusd::GetPluginsDir();
            path += "Misc" muPathSep mqabcPluginFile;
            mod = mu::LoadModule(path.c_str());
        }
        if (mod) {
            (void*&)_mqabcGetExportPlugin = mu::GetSymbol(mod, "mqabcGetExportPlugin");
        }
    }
    if (_mqabcGetExportPlugin)
        return _mqabcGetExportPlugin();
    return nullptr;
}

mqabcAPI void mqabcDummy()
{
    // dummy to prevent mqsdk stripped by linker
    DWORD a, b;
    MQGetPlugInID(&a, &b);
}
