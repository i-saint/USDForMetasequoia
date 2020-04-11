#include "mqusd.h"
#include "mqCommon/mqDummyPlugins.h"

MQBasePlugin* (*_mqabcGetImportPlugin)();

MQBasePlugin* GetPluginClass()
{
    if (!_mqabcGetImportPlugin) {
        auto mod = mu::GetModule(mqabcModuleFile);
        if (!mod) {
            std::string path = mqusd::GetMiscDir();
            path += mqabcModuleFile;
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
