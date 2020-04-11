#include "mqusd.h"
#include "mqCommon/mqDummyPlugins.h"

MQBasePlugin* (*entrypoint)();

MQBasePlugin* GetPluginClass()
{
    if (!entrypoint) {
        auto mod = mu::GetModule(mqusdModuleFile);
        if (!mod) {
            std::string path = mqusd::GetMiscDir();
            path += mqusdModuleFile;
            mod = mu::LoadModule(path.c_str());
        }
        if (mod) {
            (void*&)entrypoint = mu::GetSymbol(mod, "mqusdGetExporterPlugin");
        }
    }
    if (entrypoint)
        return entrypoint();
    return &mqusd::DummyExportPlugin::getInstance();
}

mqusdAPI void mqusdDummy()
{
    // dummy to prevent mqsdk stripped by linker
    DWORD a, b;
    MQGetPlugInID(&a, &b);
}
