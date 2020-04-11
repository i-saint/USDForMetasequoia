#include "mqusd.h"
#include "mqCommon/mqDummyPlugins.h"

MQBasePlugin* (*entrypoint)();

MQBasePlugin* GetPluginClass()
{
    if (!entrypoint) {
        auto mod = mu::GetModule(mqabcModuleFile);
        if (!mod) {
            std::string path = mqusd::GetMiscDir();
            path += mqabcModuleFile;
            mod = mu::LoadModule(path.c_str());
        }
        if (mod) {
            (void*&)entrypoint = mu::GetSymbol(mod, "mqabcGetImporterPlugin");
        }
    }
    if (entrypoint)
        return entrypoint();
    return nullptr;
}

mqabcAPI void mqabcDummy()
{
    // dummy to prevent mqsdk stripped by linker
    DWORD a, b;
    MQGetPlugInID(&a, &b);
}
