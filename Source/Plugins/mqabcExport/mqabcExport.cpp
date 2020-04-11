#include "mqusd.h"
#include "mqCommon/mqDummyPlugins.h"

MQBasePlugin* (*_mqabcGetExportPlugin)();

MQBasePlugin* GetPluginClass()
{
    if (!_mqabcGetExportPlugin) {
        auto mod = mu::GetModule(mqabcModuleFile);
        if (!mod) {
            std::string path = mqusd::GetMiscDir();
            path += mqabcModuleFile;
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
