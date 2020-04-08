#pragma once

#define sgusdAttrST             TfToken("st")

// Metasequoia-specific parameters
#define sgusdAttrShaderType      TfToken("shaderType")
#define sgusdAttrUseVertexColor  TfToken("useVertexColor")
#define sgusdAttrDoubleSided     TfToken("doubleSided")
#define sgusdAttrAmbientColor    TfToken("ambientColor")

#define sgusdAttrDiffuseTexture  "diffuseColorTexture"
#define sgusdAttrOpacityTexture  "opacityTexture"
#define sgusdAttrBumpTexture     "bumpTexture"
#define sgusdAttrSpecularTexture "specularColorTexture"
#define sgusdAttrEmissiveTexture "emissionColorTexture"

// parameters based on UsdPreviewSurface Proposal
// https://graphics.pixar.com/usd/docs/UsdPreviewSurface-Proposal.html
#define sgusdUsdPreviewSurface  TfToken("UsdPreviewSurface")
#define sgusdUsdUVTexture       TfToken("UsdUVTexture")
#define sgusdAttrDiffuseColor   TfToken("diffuseColor")
#define sgusdAttrDiffuse        TfToken("diffuse")
#define sgusdAttrEmissiveColor  TfToken("emissiveColor")
#define sgusdAttrSpecularColor  TfToken("specularColor")
#define sgusdAttrRoughness      TfToken("roughness")
#define sgusdAttrOpacity        TfToken("opacity")
#define sgusdAttrFile           TfToken("file")
#define sgusdAttrR              TfToken("r")
#define sgusdAttrRGB            TfToken("rgb")
#define sgusdAttrA              TfToken("a")
#define sgusdAttrSurface        TfToken("surface")
#define sgusdAttrWrapS          TfToken("wrapS")
#define sgusdAttrWrapT          TfToken("wrapT")
#define sgusdAttrFallback       TfToken("fallback")
#define sgusdAttrST             TfToken("st")
#define sgusdAttrSTName         TfToken("frame:stPrimvarName")
#define sgusdAttrTangents       TfToken("tangents")
#define sgusdAttrTangentsName   TfToken("frame:tangentsPrimvarName")
#define sgusdAttrBinormals      TfToken("binormals")
#define sgusdAttrBinormalsName  TfToken("frame:binormalsPrimvarName")
#define sgusdAttrColors         TfToken("colors")
#define sgusdAttrColorsName     TfToken("frame:colorsPrimvarName")

#include "../SceneGraph.h"
#include "../sgUtils.h"
