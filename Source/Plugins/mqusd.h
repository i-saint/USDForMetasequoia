#pragma once

#define mqusdVersion        1.0.0
#define mqusdVersionString  "1.0.0"
#define mqusdCopyRight      "Copyright(C) 2020, i-saint"

#define mqusdPluginProduct      0x493ADF11
#define mqusdPluginID           0xB1CC99A0
#define mqusdImportPluginID     0xB1CC99A1
#define mqusdExportPluginID     0xB1CC99A2

#define mqabcPluginProduct      mqusdPluginProduct
#define mqabcPluginID           0xB1CC99B0
#define mqabcImportPluginID     0xB1CC99B1
#define mqabcExportPluginID     0xB1CC99B2

#ifdef _WIN32
    #define mqusdModuleFile "mqusd.dll"
    #define mqabcModuleFile "mqabc.dll"
#else
    #define mqusdModuleFile "mqusd.plugin/Contents/MacOS/mqusd"
    #define mqabcModuleFile "mqabc.plugin/Contents/MacOS/mqabc"
#endif


#ifdef _WIN32
    #define mqusdAPI extern "C" __declspec(dllexport)
#else
    #define mqusdAPI extern "C" __attribute__((visibility("default")))
#endif
#define mqabcAPI mqusdAPI

#ifdef mqusdDebug
    #define mqusdDbgPrint(...) mu::Print(__VA_ARGS__)
    #define mqusdDbgFatal(...) mu::Print(__VA_ARGS__)
#else
    #define mqusdDbgPrint(...) 
    #define mqusdDbgFatal(...) 
#endif

#include "MeshUtils/MeshUtils.h"
#include "SceneGraph/SceneGraph.h"
#include "SceneGraph/SceneGraphRemote.h"
#include "SceneGraph/sgUtils.h"
#include "mqsdk/sdk_Include.h"

namespace mqusd {

using namespace sg;
using mu::float2;
using mu::float3;
using mu::float4;
using mu::quatf;
using mu::float2x2;
using mu::float3x3;
using mu::float4x4;
using mu::double4x4;

inline std::string GetPluginsDir()
{
    std::string path = mu::GetCurrentModuleDirectory();
    size_t pos = path.find("Plugins");
    if (pos == std::string::npos)
        return "";
    return std::string(path.data(), pos + 8);
}

inline std::string GetMiscDir()
{
    std::string path = GetPluginsDir();
    path += "Misc" muPathSep;
    return path;
}

} // namespace mqusd
