#include "pch.h"
#include "mqusdInternal.h"

MQPLUGIN_EXPORT BOOL MQInit(const char* exe_name);

namespace mqusd {

static void mqusdInitialize_()
{
    auto main_module_path = mu::GetModuleName(mu::GetMainModule());
    MQInit(main_module_path.data());

    muvgInitialize();
#ifdef _WIN32
    sg::SetUSDModuleDir(GetMiscDir() + "mqusd");
#endif
}

static void mqusdFinalize_()
{
    muvgPrintRecords();
}

void mqusdInitialize()
{
    static std::once_flag s_once;
    std::call_once(s_once, []() {
        mqusdInitialize_();
    });
}

void mqusdFinalize()
{
    static std::once_flag s_once;
    std::call_once(s_once, []() {
        mqusdFinalize_();
    });
}

} // namespace mqusd

MQBasePlugin* GetPluginClass()
{
    return &mqusd::DummyExportPlugin::getInstance();
}
