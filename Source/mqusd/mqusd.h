#pragma once

#define mqusdVersion        0.9.0
#define mqusdVersionString  "0.9.0"
#define mqusdCopyRight      "Copyright(C) 2020, i-saint"

#define mqusdPluginProduct      0x493ADF11
#define mqusdRecorderPluginID   0xB1CC99A0
#define mqusdPlayerPluginID     0xB1CC99A1

#ifdef _WIN32
    #define mqusdAPI extern "C" __declspec(dllexport)
#else
    #define mqusdAPI extern "C" 
#endif

#ifdef mqusdDebug
    #define mqusdDbgPrint(...) mu::Print(__VA_ARGS__)
#else
    #define mqusdDbgPrint(...) 
#endif

#include "MeshUtils/MeshUtils.h"

namespace mqusd {
using mu::float2;
using mu::float3;
using mu::float4;
using mu::quatf;
using mu::float2x2;
using mu::float3x3;
using mu::float4x4;
using mu::double4x4;

void mqusdLog(const char* fmt, ...);
} // namespace mqusd
