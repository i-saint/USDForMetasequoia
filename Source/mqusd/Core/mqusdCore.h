#pragma once

#define mqusdAttrUV             "primvars:st"
#define mqusdAttrUVIndices      "primvars:st:indices"
#define mqusdAttrBindTransform  "primvars:skel:geomBindTransform"
#define mqusdAttrJoints         "primvars:skel:joints"
#define mqusdAttrJointIndices   "primvars:skel:jointIndices"
#define mqusdAttrJointWeights   "primvars:skel:jointWeights"
#define mqusdAttrMaterialIDs    "materialIds"

#define mqusdKeyElementSize     TfToken("elementSize")
#define mqusdKeyInterpolation   TfToken("interpolation")

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

#pragma warning(push)
#pragma warning(disable:4244 267)
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/points.h"
#include "pxr/usd/usdGeom/pointInstancer.h"
#include "pxr/usd/usdSkel/skeleton.h"
#include "pxr/usd/usdSkel/animation.h"
#include "pxr/usd/usdSkel/bindingAPI.h"
#include "pxr/usd/usdSkel/blendShape.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/shader.h"
#pragma warning(pop)
PXR_NAMESPACE_USING_DIRECTIVE;
