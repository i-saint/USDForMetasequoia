#pragma once

#define mqusdAttrMaterialIDs    TfToken("materialIds")
#define mqusdAttrST             TfToken("st")

#define mqusdShaderMQClassic    "mq:classic"
#define mqusdShaderMQConstant   "mq:constant"
#define mqusdShaderMQLambert    "mq:lambert"
#define mqusdShaderMQPhong      "mq:phong"
#define mqusdShaderMQBlinn      "mq:blinn"
#define mqusdShaderMQHLSL       "mq:hlsl"

#define mqusdWrapBlack  "black"
#define mqusdWrapClamp  "clamp"
#define mqusdWrapRepeat "repeat"
#define mqusdWrapMirror "mirror"

// Metasequoia-specific parameters
#define mqusdAttrShaderType      TfToken("shaderType")
#define mqusdAttrUseVertexColor  TfToken("useVertexColor")
#define mqusdAttrDoubleSided     TfToken("doubleSided")
#define mqusdAttrAmbientColor    TfToken("ambientColor")

#define mqusdAttrDiffuseTexture  "diffuseColorTexture"
#define mqusdAttrOpacityTexture  "opacityTexture"
#define mqusdAttrBumpTexture     "bumpTexture"
#define mqusdAttrSpecularTexture "specularColorTexture"
#define mqusdAttrEmissiveTexture "emissionColorTexture"

// parameters based on UsdPreviewSurface Proposal
// https://graphics.pixar.com/usd/docs/UsdPreviewSurface-Proposal.html
#define mqusdUsdPreviewSurface  TfToken("UsdPreviewSurface")
#define mqusdUsdUVTexture       TfToken("UsdUVTexture")
#define mqusdAttrDiffuseColor   TfToken("diffuseColor")
#define mqusdAttrDiffuse        TfToken("diffuse")
#define mqusdAttrEmissiveColor  TfToken("emissiveColor")
#define mqusdAttrSpecularColor  TfToken("specularColor")
#define mqusdAttrRoughness      TfToken("roughness")
#define mqusdAttrOpacity        TfToken("opacity")
#define mqusdAttrFile           TfToken("file")
#define mqusdAttrR              TfToken("r")
#define mqusdAttrRGB            TfToken("rgb")
#define mqusdAttrA              TfToken("a")
#define mqusdAttrSurface        TfToken("surface")
#define mqusdAttrWrapS          TfToken("wrapS")
#define mqusdAttrWrapT          TfToken("wrapT")
#define mqusdAttrFallback       TfToken("fallback")
#define mqusdAttrST             TfToken("st")
#define mqusdAttrSTName         TfToken("frame:stPrimvarName")
#define mqusdAttrTangents       TfToken("tangents")
#define mqusdAttrTangentsName   TfToken("frame:tangentsPrimvarName")
#define mqusdAttrBinormals      TfToken("binormals")
#define mqusdAttrBinormalsName  TfToken("frame:binormalsPrimvarName")
#define mqusdAttrColors         TfToken("colors")
#define mqusdAttrColorsName     TfToken("frame:colorsPrimvarName")

#ifdef _WIN32
    #define sgusdAPI extern "C" __declspec(dllexport)
#else
    #define sgusdAPI extern "C" __attribute__((visibility("default")))
#endif

#include "../SceneGraph.h"
#include "../sgUtils.h"
