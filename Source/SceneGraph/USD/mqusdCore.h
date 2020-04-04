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
    #define mqusdCoreAPI extern "C" __declspec(dllexport)
#else
    #define mqusdCoreAPI extern "C" __attribute__((visibility("default")))
#endif


#pragma warning(push)
#pragma warning(disable:4244 267)
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/points.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/usd/usdSkel/root.h>
#include <pxr/usd/usdSkel/skeleton.h>
#include <pxr/usd/usdSkel/skeletonQuery.h>
#include <pxr/usd/usdSkel/cache.h>
#include <pxr/usd/usdSkel/animation.h>
#include <pxr/usd/usdSkel/blendShape.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#pragma warning(pop)
PXR_NAMESPACE_USING_DIRECTIVE;

#include "../mqusdSceneGraph.h"
