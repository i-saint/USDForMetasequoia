#include "pch.h"
#include "mqabcInternal.h"

#ifdef _WIN32
    #pragma comment(lib, "Half-2_4.lib")
    #pragma comment(lib, "Iex-2_4.lib")
    #pragma comment(lib, "IexMath-2_4.lib")
    #pragma comment(lib, "Imath-2_4.lib")
    #pragma comment(lib, "Alembic.lib")
    #pragma comment(lib, "libhdf5.lib")
    #pragma comment(lib, "libszip.lib")
    #pragma comment(lib, "zlib.lib")
#endif // _WIN32

namespace mqusd {

static void mqabcInitialize_()
{
    auto main_module_path = mu::GetModuleName(mu::GetMainModule());
    MQInit(main_module_path.data());

    muvgInitialize();
}

static void mqabcFinalize_()
{
    muvgPrintRecords();
}

void mqabcInitialize()
{
    static std::once_flag s_once;
    std::call_once(s_once, []() {
        mqabcInitialize_();
    });
}

void mqabcFinalize()
{
    static std::once_flag s_once;
    std::call_once(s_once, []() {
        mqabcFinalize_();
    });
}

} // namespace mqusd

MQBasePlugin* GetPluginClass()
{
    return &mqusd::DummyExportPlugin::getInstance();
}
