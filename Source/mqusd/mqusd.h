#pragma once

#define mqusdVersion        0.9.0
#define mqusdVersionString  "0.9.0"
#define mqusdCopyRight      "Copyright(C) 2020, i-saint"

#define mqusdPluginProduct      0x493ADF11
#define mqusdRecorderPluginID   0xB1CC99A0
#define mqusdPlayerPluginID     0xB1CC99A1

#define mqusdVertexColorPropName    "C"
#define mqusdMaterialIDPropName     "MID"

#define mqusdMtlTarget          "metasequoia"
#define mqusdMtlShaderClassic   "classic"
#define mqusdMtlShaderConstant  "constant"
#define mqusdMtlShaderLambert   "lambert"
#define mqusdMtlShaderPhong     "phong"
#define mqusdMtlShaderBlinn     "blinn"
#define mqusdMtlShaderHLSL      "hlsl"

#define mqusdMtlUseVertexColor  ".useVertexColor"
#define mqusdMtlDoubleSided     ".doubleSided"
#define mqusdMtlDiffuseColor    ".diffuseColor"
#define mqusdMtlDiffuse         ".diffuse"
#define mqusdMtlAlpha           ".alpha"
#define mqusdMtlAmbientColor    ".ambientColor"
#define mqusdMtlSpecularColor   ".specularColor"
#define mqusdMtlEmissionColor   ".emissionColor"

#ifdef _WIN32
    #define mqusdAPI extern "C" __declspec(dllexport)
#else
    #define mqusdAPI extern "C" 
#endif

#include "MeshUtils/MeshUtils.h"
using mu::float2;
using mu::float3;
using mu::float4;
using mu::float2x2;
using mu::float3x3;
using mu::float4x4;
using mu::double4x4;
