#pragma once

#define mqusdAttrMaterialIDs    TfToken("materialIds")
#define mqusdAttrUV             TfToken("primvars:st")
#define mqusdAttrUVIndices      TfToken("primvars:st:indices")

#define mqusdMtlTarget          "metasequoia"
#define mqusdShaderClassic      "classic"
#define mqusdShaderConstant     "constant"
#define mqusdShaderLambert      "lambert"
#define mqusdShaderPhong        "phong"
#define mqusdShaderBlinn        "blinn"
#define mqusdShaderHLSL         "hlsl"

// Metasequoia-specific parameters
#define mqusdMtlUseVertexColor  TfToken("useVertexColor")
#define mqusdMtlDoubleSided     TfToken("doubleSided")
#define mqusdMtlAmbientColor    TfToken("ambientColor")

// parameters based on UsdPreviewSurface Proposal
// https://graphics.pixar.com/usd/docs/UsdPreviewSurface-Proposal.html
#define mqusdUsdPreviewSurface  TfToken("UsdPreviewSurface")
#define mqusdUsdUVTexture       TfToken("UsdUVTexture")
#define mqusdMtlDiffuseColor    TfToken("diffuseColor")
#define mqusdMtlEmissiveColor   TfToken("emissiveColor")
#define mqusdMtlSpecularColor   TfToken("specularColor")
#define mqusdMtlRoughness       TfToken("roughness")
#define mqusdMtlOpacity         TfToken("opacity")

#define mqusdMtlDiffuseColorTexture     "diffuseColorTexture"
#define mqusdMtlAmbientColorTexture     "ambientColorTexture"
#define mqusdMtlSpecularColorTexture    "specularColorTexture"
#define mqusdMtlEmissionColorTexture    "emissionColorTexture"

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
#include <pxr/usd/usdSkel/root.h>
#include <pxr/usd/usdSkel/skeleton.h>
#include <pxr/usd/usdSkel/skeletonQuery.h>
#include <pxr/usd/usdSkel/cache.h>
#include <pxr/usd/usdSkel/animation.h>
#include <pxr/usd/usdSkel/blendShape.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/shader.h>
#pragma warning(pop)
PXR_NAMESPACE_USING_DIRECTIVE;

#include "Foundation/mqusdSceneGraph.h"
