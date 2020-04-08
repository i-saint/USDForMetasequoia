#pragma once

#define sgabcAttrVertexColor        "C"
#define sgabcAttrMaterialID         "MID"
#define sgabcAttrMaterialBinding    "material:binding"

#define mqabcMaterialTarget         "metasequoia"
#define sgabcAttrUseVertexColor     "useVertexColor"
#define sgabcAttrDoubleSided        "doubleSided"
#define sgabcAttrDiffuseColor       "diffuseColor"
#define sgabcAttrDiffuse            "diffuse"
#define sgabcAttrOpacity            "opacity"
#define sgabcAttrRoughness          "roughness"
#define sgabcAttrAmbientColor       "ambientColor"
#define sgabcAttrEmissiveColor      "emissiveColor"
#define sgabcAttrSpecularColor      "specularColor"

#define sgabcAttrDiffuseTexture     "diffuseColorTexture"
#define sgabcAttrOpacityTexture     "opacityTexture"
#define sgabcAttrBumpTexture        "bumpTexture"

#define sgabcAttrWrapS              "wrapS"
#define sgabcAttrWrapT              "wrapT"


#include "../SceneGraph.h"
#include "../sgUtils.h"

namespace sg {

using namespace Alembic;
using abcV2 = Imath::V2f;
using abcV3 = Imath::V3f;
using abcV3i = Imath::V3i;
using abcV3d = Imath::V3d;
using abcV4 = Imath::V4f;
using abcC3 = Imath::C3f;
using abcC4 = Imath::C4f;
using abcM44 = Imath::M44f;
using abcM44d = Imath::M44d;
using abcBox = Imath::Box3f;
using abcBoxd = Imath::Box3d;
using abcChrono = Abc::chrono_t;

} // namespace sg
