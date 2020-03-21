#pragma once

#define mqusdAttrMaterialIDs    TfToken("materialIds")
#define mqusdAttrUV             TfToken("primvars:st")
#define mqusdAttrUVIndices      TfToken("primvars:st:indices")
#define mqusdAttrBindTransform  TfToken("primvars:skel:geomBindTransform")
#define mqusdAttrJoints         TfToken("primvars:skel:joints")
#define mqusdAttrJointIndices   TfToken("primvars:skel:jointIndices")
#define mqusdAttrJointWeights   TfToken("primvars:skel:jointWeights")

#define mqusdRelBlendshapeTargets   TfToken("skel:blendShapeTargets")
#define mqusdRelSkeleton            TfToken("skel:skeleton")

#define mqusdMetaElementSize        TfToken("elementSize")
#define mqusdMetaInterpolation      TfToken("interpolation")

#define mqusdInterpolationVertex    TfToken("vertex")
#define mqusdInterpolationConstant  TfToken("constant")

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
    #define mqusdCoreAPI extern "C" __declspec(dllexport)
#else
    #define mqusdCoreAPI extern "C" __attribute__((visibility("default")))
#endif


#pragma warning(push)
#pragma warning(disable:4244 267)
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/points.h"
#include "pxr/usd/usdGeom/pointInstancer.h"
#include "pxr/usd/usdSkel/root.h"
#include "pxr/usd/usdSkel/skeleton.h"
#include "pxr/usd/usdSkel/skeletonQuery.h"
#include "pxr/usd/usdSkel/cache.h"
#include "pxr/usd/usdSkel/animation.h"
#include "pxr/usd/usdSkel/blendShape.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/shader.h"
#pragma warning(pop)
PXR_NAMESPACE_USING_DIRECTIVE;

#include "Foundation/mqusdSceneGraph.h"
