#include "pch.h"
#include "sgusd.h"
#include "sgusdSceneGraph.h"
#include "sgusdUtils.h"

#ifdef _WIN32
    #pragma comment(lib, "usd_ms.lib")
#endif

namespace sg {

USDNode::USDNode(USDNode* parent, UsdPrim prim, bool create_node)
    : m_prim(prim)
    , m_scene(USDScene::getCurrent())
    , m_parent(parent)
{
    if (m_parent)
        m_parent->m_children.push_back(this);
    if (create_node)
        setNode(CreateNode<Node>(parent, prim));
}

USDNode::USDNode(Node* n, UsdPrim prim)
    : m_prim(prim)
    , m_scene(USDScene::getCurrent())
{
    setNode(n);
    if (m_node->parent) {
        m_parent = (USDNode*)m_node->parent->impl;
        m_parent->m_children.push_back(this);
    }
}

USDNode::~USDNode()
{
}

template<class T>
void USDNode::padSample(UsdAttribute& attr, UsdTimeCode t, const T default_sample)
{
    if (t.IsDefault())
        return;
    auto prev = m_scene->getPrevTime();
    if (t != prev)
        attr.Set(default_sample, prev);
}

void USDNode::beforeRead()
{
    m_attr_display_name = m_prim.GetAttribute(sgusdAttrDisplayName);
}

void USDNode::read(UsdTimeCode t)
{
    auto& dst = *getNode();
    if (m_attr_display_name)
        GetBinary(m_attr_display_name, dst.display_name, t);
}

void USDNode::beforeWrite()
{
}

void USDNode::write(UsdTimeCode t)
{
    const auto& src = *getNode();
    if (!m_attr_display_name && !src.display_name.empty() && src.display_name != m_prim.GetName().GetText())
        m_attr_display_name = m_prim.CreateAttribute(sgusdAttrDisplayName, SdfValueTypeNames->String, false);
    if (m_attr_display_name)
        SetBinary(m_attr_display_name, src.display_name, t);
}

void USDNode::setNode(Node* node)
{
    m_node = node;
    m_node->impl = this;
}

std::string USDNode::getName() const
{
    return m_prim.GetName().GetString();
}

std::string USDNode::getPath() const
{
    return m_prim.GetPath().GetString();
}



USDRootNode::USDRootNode(UsdPrim prim)
    : super(nullptr, prim, false)
{
    setNode(new RootNode());
}

USDRootNode::USDRootNode(Node* n, UsdPrim prim)
    : super(n, prim)
{
}

USDXformNode::USDXformNode(USDNode* parent, UsdPrim prim, bool create_node)
    : super(parent, prim, false)
{
    m_xform = UsdGeomXformable(prim);
    if (create_node)
        setNode(CreateNode<XformNode>(parent, prim));
}

USDXformNode::USDXformNode(Node* n, UsdPrim prim)
    : super(n, prim)
{
    m_xform = UsdGeomXformable(prim);
}

void USDXformNode::read(UsdTimeCode t)
{
    super::read(t);
    auto& dst = *getNode<XformNode>();

    // visibility
    TfToken vis;
    if (m_xform.GetVisibilityAttr().Get(&vis, t)) {
        if (vis == UsdGeomTokens->inherited) {
            if (auto px = dst.parent->cast<XformNode>())
                dst.visibility = px->visibility;
            else
                dst.visibility = true;
        }
        else
            dst.visibility = vis != UsdGeomTokens->invisible;
    }

    // transform
    GfMatrix4d mat;
    bool reset_stack = false;
    m_xform.GetLocalTransformation(&mat, &reset_stack, t);
    dst.local_matrix.assign((double4x4&)mat);

    if (dst.parent_xform)
        dst.global_matrix = dst.local_matrix * dst.parent_xform->global_matrix;
    else
        dst.global_matrix = dst.local_matrix;
}

void USDXformNode::write(UsdTimeCode t)
{
    super::write(t);
    const auto& src = *getNode<XformNode>();

    // visibility
    m_xform.GetVisibilityAttr().Set(src.visibility ? TfToken() : UsdGeomTokens->invisible, t);

    // transform
    if (src.local_matrix != float4x4::identity()) {
        if (m_xf_ops.empty()) {
            m_xf_ops.push_back(m_xform.AddTransformOp());
        }

        auto& op = m_xf_ops[0];
        if (op.GetOpType() == UsdGeomXformOp::TypeTransform) {
            double4x4 data;
            data.assign(src.local_matrix);
            op.Set((const GfMatrix4d&)data, t);
        }
    }
}


USDMeshNode::USDMeshNode(USDNode* parent, UsdPrim prim)
    : super(parent, prim, false)
{
    m_mesh = UsdGeomMesh(prim);
    setNode(CreateNode<MeshNode>(parent, prim));

//#ifdef mqusdDebug
//    PrintPrim(m_prim, PF_Full);
//#endif
}

USDMeshNode::USDMeshNode(Node* n, UsdPrim prim)
    : super(n, prim)
{
    m_mesh = UsdGeomMesh(prim);
}

void USDMeshNode::beforeRead()
{
    super::beforeRead();
    auto& dst = *getNode<MeshNode>();

    // find attributes
    auto pvapi = UsdGeomPrimvarsAPI(m_prim);
    m_pv_st     = pvapi.GetPrimvar(sgusdAttrST);
    m_pv_colors = pvapi.GetPrimvar(sgusdAttrColors);
    m_attr_bs_ids   = m_prim.GetAttribute(UsdSkelTokens->skelBlendShapes);
    m_attr_joints   = m_prim.GetAttribute(UsdSkelTokens->skelJoints);
    m_attr_joint_indices    = m_prim.GetAttribute(UsdSkelTokens->primvarsSkelJointIndices);
    m_attr_joint_weights    = m_prim.GetAttribute(UsdSkelTokens->primvarsSkelJointWeights);
    m_attr_bind_transform   = m_prim.GetAttribute(UsdSkelTokens->primvarsSkelGeomBindTransform);

    // resolve blendshape/targets
    if (m_attr_bs_ids) {
        VtArray<TfToken> bs_ids;
        m_attr_bs_ids.Get(&bs_ids);

        transform_container(m_blendshape_ids, bs_ids, [](auto& d, auto& s) {
            d = s.GetString();
        });
    }
    if (auto rel_bs_targets = m_prim.GetRelationship(UsdSkelTokens->skelBlendShapeTargets)) {
        SdfPathVector paths;
        rel_bs_targets.GetTargets(&paths);

        transform_container(m_blendshapes, paths, [this, &dst](USDBlendshapeNode*& d, auto& s) {
            d = m_scene->findNode<USDBlendshapeNode>(s.GetString());
            if (d) {
                dst.blendshapes.push_back(d->getNode<BlendshapeNode>());
            }
            else {
                sgDbgPrint("not found %s\n", s.GetText());
            }
        });
    }

    // resolve blendshape animation
    // note: skelanimation may be related to SkelRoot and be resolved by USDSkelRoot::beforeRead()
    if (auto rel_anim = m_prim.GetRelationship(UsdSkelTokens->skelAnimationSource)) {
        SdfPathVector paths;
        rel_anim.GetTargets(&paths);
        if (!paths.empty())
            m_animation = m_scene->findNode<USDSkelAnimationNode>(paths.front().GetString());
    }

    // resolve skeleton
    // note: skeleton may be related to SkelRoot and be resolved by USDSkelRoot::beforeRead()
    if (auto rel_skel = m_prim.GetRelationship(UsdSkelTokens->skelSkeleton)) {
        SdfPathVector paths;
        rel_skel.GetTargets(&paths);
        if (!paths.empty()) {
            m_skeleton = m_scene->findNode<USDSkeletonNode>(paths.front().GetString());
            if (m_skeleton)
                dst.skeleton = m_skeleton->getNode<SkeletonNode>();
        }
    }
    if (dst.skeleton) {
        // resolve joints
        if (m_attr_joints) {
            VtArray<TfToken> data;
            m_attr_joints.Get(&data);
            transform_container(dst.joints, data, [&dst](auto*& d, auto& s) {
                d = dst.skeleton->findJointByPath(s.GetString());
                if (!d) {
                    sgDbgPrint("not found %s\n", s.GetText());
                }
            });
        }
        else {
            transform_container(dst.joints, dst.skeleton->joints, [](auto*& d, auto& s) {
                d = s.get();
            });
        }
    }

    // prepare subsets
    auto mbapi = UsdShadeMaterialBindingAPI(m_prim);
    if (auto mat = mbapi.ComputeBoundMaterial()) {
        // bound single material
        m_isubsets.resize(1);
        auto& data = m_isubsets.front();
        data.dst = std::make_shared<FaceSet>();
        data.dst->material = m_scene->findNode<MaterialNode>(mat.GetPath().GetString());
    }
    else {
        auto subsets = mbapi.GetMaterialBindSubsets();
        transform_container(m_isubsets, subsets, [this, &mbapi](SubsetData& data, UsdGeomSubset& subset) {
            data.subset = subset;
            data.dst = std::make_shared<FaceSet>();
            if (auto mat = UsdShadeMaterialBindingAPI(subset).ComputeBoundMaterial())
                data.dst->material = m_scene->findNode<MaterialNode>(mat.GetPath().GetString());
        });
    }
}

void USDMeshNode::read(UsdTimeCode t)
{
    super::read(t);
    auto& dst = *getNode<MeshNode>();

    // counts, indices, points
    m_mesh.GetFaceVertexCountsAttr().Get(&m_counts, t);
    dst.counts.share(m_counts.cdata(), m_counts.size());

    m_mesh.GetFaceVertexIndicesAttr().Get(&m_indices, t);
    dst.indices.share(m_indices.cdata(), m_indices.size());

    m_mesh.GetPointsAttr().Get(&m_points, t);
    dst.points.share((float3*)m_points.cdata(), m_points.size());


    auto flatten_primvar = [this, &dst](auto& d, auto& s) {
        using src_t = typename std::remove_reference_t<decltype(s)>::value_type;
        using dst_t = typename std::remove_reference_t<decltype(d)>::value_type;
        static_assert(sizeof(src_t) == sizeof(dst_t), "data size mismatch");

        if (s.size() == dst.indices.size()) {
            d.share((dst_t*)s.cdata(), s.size());
        }
        else if (s.size() == dst.points.size()) {
            d.resize_discard(dst.indices.size());
            mu::CopyWithIndices(d.data(), (dst_t*)s.cdata(), dst.indices);
        }
        else {
            d.clear();
        }
    };

    // normals
    m_mesh.GetNormalsAttr().Get(&m_normals, t);
    flatten_primvar(dst.normals, m_normals);

    // uv
    if (m_pv_st) {
        m_pv_st.ComputeFlattened(&m_uvs, t);
        flatten_primvar(dst.uvs, m_uvs);
    }

    // colors
    if (m_pv_colors) {
        m_pv_colors.ComputeFlattened(&m_colors, t);
        flatten_primvar(dst.colors, m_colors);
    }

    // blendshape weights
    if (m_animation && m_blendshapes.size() == m_blendshape_ids.size()) {
        auto& data = m_animation->getBlendshapeData(t);
        auto get_weight = [&data](const std::string& id) {
            auto it = std::lower_bound(data.begin(), data.end(), id, [](auto* a, auto& b) {
                return a->id < b;
            });
            return it != data.end() && (*it)->id == id ? (*it)->weight : 0.0f;
        };
        transform_container(dst.blendshape_weights, m_blendshape_ids, [&get_weight](auto& d, auto& s) {
            d = get_weight(s);
        });
    }

    // skel
    if (m_attr_joint_indices && m_attr_joint_weights) {
        TfToken interpolation;
        m_attr_joint_indices.GetMetadata(UsdGeomTokens->elementSize, &dst.joints_per_vertex);
        m_attr_joint_indices.GetMetadata(UsdGeomTokens->interpolation, &interpolation);

        m_attr_joint_indices.Get(&m_joint_indices);
        m_attr_joint_weights.Get(&m_joint_weights);
        if (m_joint_indices.size() == dst.joints_per_vertex) {
            dst.joint_indices.resize_discard(dst.points.size() * dst.joints_per_vertex);
            dst.joint_weights.resize_discard(dst.points.size() * dst.joints_per_vertex);
            expand_uniform(dst.joint_indices.data(), dst.joint_indices.size(), m_joint_indices.cdata(), m_joint_indices.size());
            expand_uniform(dst.joint_weights.data(), dst.joint_weights.size(), m_joint_weights.cdata(), m_joint_weights.size());
        }
        else {
            dst.joint_indices.share(m_joint_indices.cdata(), m_joint_indices.size());
            dst.joint_weights.share(m_joint_weights.cdata(), m_joint_weights.size());
        }


        if (m_attr_bind_transform) {
            GfMatrix4d data;
            m_attr_bind_transform.Get(&data);
            dst.bind_transform.assign((double4x4&)data);
        }
    }

    // subsets
    transform_container(dst.facesets, m_isubsets, [&dst, &t](FaceSetPtr& dfs, SubsetData& data) {
        auto& faces = data.dst->faces;
        if (data.subset) {
            data.subset.GetIndicesAttr().Get(&data.sample, t);
            faces.share(data.sample.cdata(), data.sample.size());
        }
        else {
            // bound single material
            faces.resize_discard(dst.counts.size());
            std::iota(faces.begin(), faces.end(), 0);
        }
        dfs = data.dst;
    });

    // validate
    dst.validate();
}


void USDMeshNode::beforeWrite()
{
    super::beforeWrite();

    // create attributes
    auto pvapi = UsdGeomPrimvarsAPI(m_prim);
    m_pv_st = pvapi.CreatePrimvar(sgusdAttrST, SdfValueTypeNames->TexCoord2fArray);
    m_pv_colors = pvapi.CreatePrimvar(sgusdAttrColors, SdfValueTypeNames->Color4fArray);

    auto& src = *getNode<MeshNode>();

    // skinning attributes
    if (src.joints_per_vertex > 0) {
        m_attr_joint_indices = m_prim.CreateAttribute(UsdSkelTokens->primvarsSkelJointIndices, SdfValueTypeNames->IntArray, false);
        m_attr_joint_weights = m_prim.CreateAttribute(UsdSkelTokens->primvarsSkelJointWeights, SdfValueTypeNames->FloatArray, false);

        m_attr_joint_indices.SetMetadata(UsdGeomTokens->elementSize, src.joints_per_vertex);
        m_attr_joint_weights.SetMetadata(UsdGeomTokens->elementSize, src.joints_per_vertex);

        // skinning data can be assumed not time sampled. so, read it at this point.

        if (is_uniform(src.joint_indices.cdata(), src.joint_indices.size(), src.joints_per_vertex) &&
            is_uniform(src.joint_weights.cdata(), src.joint_weights.size(), src.joints_per_vertex))
        {
            m_attr_joint_indices.SetMetadata(UsdGeomTokens->interpolation, UsdGeomTokens->constant);
            m_attr_joint_weights.SetMetadata(UsdGeomTokens->interpolation, UsdGeomTokens->constant);
            m_joint_indices.assign(src.joint_indices.begin(), src.joint_indices.begin() + src.joints_per_vertex);
            m_joint_weights.assign(src.joint_weights.begin(), src.joint_weights.begin() + src.joints_per_vertex);
        }
        else {
            m_attr_joint_indices.SetMetadata(UsdGeomTokens->interpolation, UsdGeomTokens->vertex);
            m_attr_joint_weights.SetMetadata(UsdGeomTokens->interpolation, UsdGeomTokens->vertex);
            m_joint_indices.assign(src.joint_indices.begin(), src.joint_indices.end());
            m_joint_weights.assign(src.joint_weights.begin(), src.joint_weights.end());
        }
        m_attr_joint_indices.Set(m_joint_indices);
        m_attr_joint_weights.Set(m_joint_weights);

        if (src.bind_transform != float4x4::identity()) {
            GfMatrix4d data;
            ((double4x4&)data).assign(src.bind_transform);
            m_attr_bind_transform = m_prim.CreateAttribute(UsdSkelTokens->primvarsSkelGeomBindTransform, SdfValueTypeNames->Matrix4d, false);
            m_attr_bind_transform.Set(data);
        }
    }

    // blendshapes
    if (!src.blendshapes.empty()) {
        auto rel_blendshapes = m_prim.CreateRelationship(UsdSkelTokens->skelBlendShapeTargets, false);
        SdfPathVector targets;
        for(auto* blendshape : src.blendshapes)
            targets.push_back(SdfPath(blendshape->getPath()));
        rel_blendshapes.SetTargets(targets);
    }
}

void USDMeshNode::write(UsdTimeCode t)
{
    super::write(t);
    auto& src = *getNode<MeshNode>();

    {
        m_counts.assign(src.counts.begin(), src.counts.end());
        m_mesh.GetFaceVertexCountsAttr().Set(m_counts, t);

        m_indices.assign(src.indices.begin(), src.indices.end());
        m_mesh.GetFaceVertexIndicesAttr().Set(m_indices, t);

        m_points.assign((GfVec3f*)src.points.begin(), (GfVec3f*)src.points.end());
        m_mesh.GetPointsAttr().Set(m_points, t);
    }
    if (!src.normals.empty()) {
        m_normals.assign((GfVec3f*)src.normals.begin(), (GfVec3f*)src.normals.end());
        m_mesh.GetNormalsAttr().Set(m_normals, t);
    }
    if (m_pv_st && !src.uvs.empty()) {
        m_uvs.assign((GfVec2f*)src.uvs.begin(), (GfVec2f*)src.uvs.end());
        m_pv_st.Set(m_uvs, t);
    }
    if (m_pv_colors && !src.colors.empty()) {
        m_colors.assign((GfVec4f*)src.colors.begin(), (GfVec4f*)src.colors.end());
        m_pv_colors.Set(m_colors, t);
    }

    // subsets
    for (auto& fs : src.facesets) {
        auto mat = fs->material;
        if (!mat)
            continue;

        auto& data = m_osubsets[mat->id];
        if (!data.subset) {
            auto subset_name = GetUSDNodeName(mat);
            data.subset = UsdShadeMaterialBindingAPI(m_prim).CreateMaterialBindSubset(TfToken(subset_name), VtArray<int>());
            if (auto mat_node = static_cast<USDMaterialNode*>(mat->impl))
                UsdShadeMaterialBindingAPI(data.subset.GetPrim()).Bind(mat_node->m_material);

            // pad empty sample as default value
            padSample(data.subset.GetIndicesAttr(), t, VtArray<int>());
        }
        data.sample.assign(fs->faces.begin(), fs->faces.end());
    }
    for (auto& kvp : m_osubsets) {
        auto& data = kvp.second;
        data.subset.GetIndicesAttr().Set(data.sample, t);
        data.sample = {};
    }
}



USDBlendshapeNode::USDBlendshapeNode(USDNode* parent, UsdPrim prim)
    : super(parent, prim, false)
{
    m_blendshape = UsdSkelBlendShape(prim);

    setNode(CreateNode<BlendshapeNode>(parent, prim));
}

USDBlendshapeNode::USDBlendshapeNode(Node* n, UsdPrim prim)
    : super(n, prim)
{
    m_blendshape = UsdSkelBlendShape(prim);
}

void USDBlendshapeNode::read(UsdTimeCode t)
{
    super::read(t);
    auto& dst = *getNode<BlendshapeNode>();

    m_blendshape.GetPointIndicesAttr().Get(&m_point_indices);
    dst.indices.share(m_point_indices.cdata(), m_point_indices.size());

    auto ibs = m_blendshape.GetInbetweens();
    m_targets.resize(ibs.size() + 1);
    auto* ibp = m_targets.data();
    for (auto& ib : ibs) {
        float w = 0.0f;
        if (ib.GetWeight(&w)) {
            auto t = dst.addTarget(w);
            ib.GetOffsets(&ibp->point_offsets);
            ib.GetNormalOffsets(&ibp->normal_offsets);
            t->point_offsets.share((float3*)ibp->point_offsets.cdata(), ibp->point_offsets.size());
            t->normal_offsets.share((float3*)ibp->normal_offsets.cdata(), ibp->normal_offsets.size());
        }
        ++ibp;
    }
    {
        auto t = dst.addTarget(1.0f);
        m_blendshape.GetOffsetsAttr().Get(&ibp->point_offsets);
        m_blendshape.GetNormalOffsetsAttr().Get(&ibp->normal_offsets);
        t->point_offsets.share((float3*)ibp->point_offsets.cdata(), ibp->point_offsets.size());
        t->normal_offsets.share((float3*)ibp->normal_offsets.cdata(), ibp->normal_offsets.size());
    }
}

void USDBlendshapeNode::beforeWrite()
{
    super::beforeWrite();
    const auto& src = *getNode<BlendshapeNode>();

    if (src.targets.empty())
        return;

    // assume not time sampled. write once.
    if (!src.indices.empty()) {
        m_point_indices.assign(src.indices.begin(), src.indices.end());
        m_blendshape.GetPointIndicesAttr().Set(m_point_indices);
    }
    {
        auto t = src.targets.back().get();
        m_targets.resize(src.targets.size());
        auto* ibp = &m_targets.back();
        if (!t->point_offsets.empty()) {
            ibp->point_offsets.assign((GfVec3f*)t->point_offsets.begin(), (GfVec3f*)t->point_offsets.end());
            m_blendshape.GetOffsetsAttr().Set(ibp->point_offsets);
        }
        if (!t->normal_offsets.empty()) {
            ibp->normal_offsets.assign((GfVec3f*)t->normal_offsets.begin(), (GfVec3f*)t->normal_offsets.end());
            m_blendshape.GetNormalOffsetsAttr().Set(ibp->normal_offsets);
        }
    }
}


USDSkelRootNode::USDSkelRootNode(USDNode* parent, UsdPrim prim)
    : super(parent, prim, false)
{
    setNode(CreateNode<SkelRootNode>(parent, prim));
}

USDSkelRootNode::USDSkelRootNode(Node* n, UsdPrim prim)
    : super(n, prim)
{
}

void USDSkelRootNode::beforeRead()
{
    super::beforeRead();
    auto& dst = *getNode<SkelRootNode>();

    // resolve animation
    if (auto rel_anim = m_prim.GetRelationship(UsdSkelTokens->skelAnimationSource)) {
        SdfPathVector paths;
        rel_anim.GetTargets(&paths);
        if (!paths.empty()) {
            if (auto usd_anim = m_scene->findNode<USDSkelAnimationNode>(paths.front().GetString())) {
                // update child meshes
                eachChildR([usd_anim](USDNode* n) {
                    if (auto usd_mesh = n->cast<USDMeshNode>())
                        usd_mesh->m_animation = usd_anim;
                });
            }
        }
    }

    // resolve skeleton
    if (auto rel_skel = m_prim.GetRelationship(UsdSkelTokens->skelSkeleton)) {
        SdfPathVector paths;
        rel_skel.GetTargets(&paths);
        if (!paths.empty()) {
            if (auto usd_skel = m_scene->findNode<USDSkeletonNode>(paths.front().GetString())) {
                auto skel = usd_skel->getNode<SkeletonNode>();
                dst.skeleton = skel;

                // update child meshes
                eachChildR([usd_skel, skel](USDNode* n) {
                    if (auto usd_mesh = n->cast<USDMeshNode>()) {
                        usd_mesh->m_skeleton = usd_skel;
                        usd_mesh->getNode<MeshNode>()->skeleton = skel;
                    }
                });
            }
        }
    }
}

void USDSkelRootNode::beforeWrite()
{
    super::beforeWrite();

    const auto& src = *getNode<SkelRootNode>();
    if (src.skeleton) {
        auto rel_skel = m_prim.CreateRelationship(UsdSkelTokens->skelSkeleton, false);

        SdfPathVector targets;
        targets.push_back(SdfPath(src.skeleton->getPath()));
        rel_skel.SetTargets(targets);
    }
}


USDSkeletonNode::USDSkeletonNode(USDNode* parent, UsdPrim prim)
    : super(parent, prim, false)
{
    m_skel = UsdSkelSkeleton(prim);
    setNode(CreateNode<SkeletonNode>(parent, prim));

    // build joints
    auto& dst = *getNode<SkeletonNode>();
    VtArray<TfToken> data;
    m_skel.GetJointsAttr().Get(&data);
    for (auto& token : data)
        dst.addJoint(token.GetString());
}

USDSkeletonNode::USDSkeletonNode(Node* n, UsdPrim prim)
    : super(n, prim)
{
    m_skel = UsdSkelSkeleton(prim);
}

void USDSkeletonNode::beforeRead()
{
    super::beforeRead();
}

void USDSkeletonNode::read(UsdTimeCode t)
{
    super::read(t);
    auto& dst = *getNode<SkeletonNode>();

    // bindpose
    {
        VtArray<GfMatrix4d> data;
        m_skel.GetBindTransformsAttr().Get(&data);
        size_t n = data.size();
        if (dst.joints.size() == n) {
            for (size_t i = 0; i < n; ++i)
                dst.joints[i]->bindpose.assign((double4x4&)data[i]);
        }
    }
    {
        VtArray<GfMatrix4d> data;
        m_skel.GetRestTransformsAttr().Get(&data);
        size_t n = data.size();
        if (dst.joints.size() == n) {
            for (size_t i = 0; i < n; ++i)
                dst.joints[i]->restpose.assign((double4x4&)data[i]);
        }
    }

    // update joint matrices
    if (auto query = m_cache.GetSkelQuery(m_skel)) {
        VtArray<GfMatrix4d> data;
        query.ComputeJointLocalTransforms(&data, t);
        size_t n = data.size();
        if (dst.joints.size() == n) {
            for (size_t i = 0; i < n; ++i)
                dst.joints[i]->local_matrix.assign((double4x4&)data[i]);
        }
        dst.updateGlobalMatrices(dst.global_matrix);
    }
}

void USDSkeletonNode::beforeWrite()
{
    super::beforeWrite();
    const auto& src = *getNode<SkeletonNode>();
    size_t njoints = src.joints.size();

    {
        VtArray<TfToken> data;
        transform_container(data, src.joints, [](TfToken& dst, auto& joint) {
            dst = TfToken(joint->path);
        });
        m_skel.GetJointsAttr().Set(data);
    }

    {
        VtArray<GfMatrix4d> data;
        transform_container(data, src.joints, [](GfMatrix4d& dst, auto& joint) {
            ((double4x4&)dst).assign(joint->bindpose);
        });
        m_skel.GetBindTransformsAttr().Set(data);
    }
    if (!is_uniform(src.joints, [](auto& joint) { return joint->restpose == float4x4::identity(); })) {
        VtArray<GfMatrix4d> data;
        transform_container(data, src.joints, [](GfMatrix4d& dst, auto& joint) {
            ((double4x4&)dst).assign(joint->restpose);
        });
        m_skel.GetRestTransformsAttr().Set(data);
    }
}

void USDSkeletonNode::write(UsdTimeCode t)
{
    super::write(t);
    // todo
}



USDSkelAnimationNode::USDSkelAnimationNode(USDNode* parent, UsdPrim prim)
    : super(parent, prim, false)
{
    m_anim = UsdSkelAnimation(prim);
    setNode(CreateNode<Node>(parent, prim));
}

USDSkelAnimationNode::USDSkelAnimationNode(Node* n, UsdPrim prim)
    : super(n, prim)
{
    m_anim = UsdSkelAnimation(prim);
}

void USDSkelAnimationNode::beforeRead()
{
    super::beforeRead();

    VtArray<TfToken> bs_ids;
    if (m_anim.GetBlendShapesAttr().Get(&bs_ids)) {
        transform_container(m_blendshapes, bs_ids, [](auto& d, auto& s) {
            d = { s.GetString(), 0.0f };
        });

        // make sorted
        transform_container(m_blendshapes_sorted, m_blendshapes, [](auto*& d, auto& s) {
            d = &s;
        });
        std::sort(m_blendshapes_sorted.begin(), m_blendshapes_sorted.end(), [](auto* a, auto* b) { return a->id < b->id; });
    }
}

std::vector<const USDSkelAnimationNode::BlendshapeData*>& USDSkelAnimationNode::getBlendshapeData(UsdTimeCode t)
{
    if (t != m_prev_read) {
        m_prev_read = t;
        m_anim.GetBlendShapeWeightsAttr().Get(&m_bs_weights, t);
        if (m_bs_weights.size() == m_blendshapes.size()) {
            transform_container(m_blendshapes, m_bs_weights, [](auto& d, float s) {
                d.weight = s;
            });
        }
    }
    return m_blendshapes_sorted;
}



USDInstancerNode::USDInstancerNode(USDNode* parent, UsdPrim prim)
    : super(parent, prim)
{
    m_instancer = UsdGeomPointInstancer(prim);
    setNode(CreateNode<InstancerNode>(parent, prim));
}

USDInstancerNode::USDInstancerNode(Node* n, UsdPrim prim)
    : super(n, prim)
{
    m_instancer = UsdGeomPointInstancer(prim);
}

void USDInstancerNode::beforeRead()
{
    super::beforeRead();
    auto& dst = *getNode<InstancerNode>();

    auto rel = m_instancer.GetPrototypesRel();
    if (rel) {
        SdfPathVector paths;
        rel.GetTargets(&paths);
        for (auto& path : paths) {
            if (auto proto = m_scene->findNode<USDNode>(path.GetString()))
                dst.protos.push_back(proto->m_node);
        }
    }
}

void USDInstancerNode::read(UsdTimeCode t)
{
    super::read(t);
    auto& dst = *getNode<InstancerNode>();

    m_instancer.GetProtoIndicesAttr().Get(&m_proto_indices, t);
    dst.proto_indices.share(m_proto_indices.cdata(), m_proto_indices.size());

    m_instancer.ComputeInstanceTransformsAtTime(&m_matrices, t, t);
    transform_container(dst.matrices, m_matrices, [](float4x4& d, const GfMatrix4d& s) {
        d.assign((double4x4&)s);
    });
}

void USDInstancerNode::beforeWrite()
{
    super::beforeWrite();
    const auto& src = *getNode<InstancerNode>();

    auto rel = m_instancer.CreatePrototypesRel();
    if (rel) {
        SdfPathVector paths;
        for (auto& proto : src.protos)
            paths.push_back(SdfPath(proto->getPath()));
        rel.SetTargets(paths);
    }
}

void USDInstancerNode::write(UsdTimeCode t)
{
    super::write(t);
    const auto& src = *getNode<InstancerNode>();

    m_proto_indices.assign(src.proto_indices.begin(), src.proto_indices.end());
    m_instancer.GetProtoIndicesAttr().Set(m_proto_indices, t);

    size_t n = src.matrices.size();
    m_positions.resize(n);
    m_orientations.resize(n);
    m_scales.resize(n);
    for (size_t i = 0; i < n; ++i) {
        float3 t; quatf r; float3 s;
        mu::extract_trs(src.matrices[i], t, r, s);
        m_positions[i] = { t.x, t.y, t.z };
        m_orientations[i] = { r.w, r.x, r.y, r.z };
        m_positions[i] = { s.x, s.y, s.z };
    }

    m_instancer.GetPositionsAttr().Set(m_positions, t);
    m_instancer.GetOrientationsAttr().Set(m_orientations, t);
    m_instancer.GetScalesAttr().Set(m_scales, t);
}



USDMaterialNode::USDMaterialNode(USDNode* parent, UsdPrim prim)
    : super(parent, prim, false)
{
    m_material = UsdShadeMaterial(prim);
    setNode(CreateNode<MaterialNode>(parent, prim));

//#ifdef mqusdDebug
//    PrintPrim(m_prim, PF_Full);
//#endif
}

USDMaterialNode::USDMaterialNode(Node* n, UsdPrim prim)
    : super(n, prim)
{
    m_material = UsdShadeMaterial(prim);
}

void USDMaterialNode::beforeRead()
{
    super::beforeRead();

    EachChild(m_prim, [this](UsdPrim& c) {
        UsdShadeShader sh(c);
        if (!sh)
            return;

        TfToken id;
        if (sh.GetIdAttr().Get(&id)) {
            if (id == sgusdUsdPreviewSurface) {
                m_surface = sh;
                m_in_use_vertex_color = m_surface.GetInput(sgusdAttrUseVertexColor);
                m_in_double_sided   = m_surface.GetInput(sgusdAttrDoubleSided);
                m_in_diffuse_color  = m_surface.GetInput(sgusdAttrDiffuseColor);
                m_in_diffuse        = m_surface.GetInput(sgusdAttrDiffuse);
                m_in_opacity        = m_surface.GetInput(sgusdAttrOpacity);
                m_in_roughness      = m_surface.GetInput(sgusdAttrRoughness);
                m_in_ambient_color  = m_surface.GetInput(sgusdAttrAmbientColor);
                m_in_specular_color = m_surface.GetInput(sgusdAttrSpecularColor);
                m_in_emissive_color = m_surface.GetInput(sgusdAttrEmissiveColor);

                auto& dst = *getNode<MaterialNode>();
                GetValue(m_surface.GetInput(sgusdAttrShaderType), dst.shader_type);
            }
            else if (id == sgusdUsdUVTexture) {
                if (c.GetName() == sgusdAttrDiffuseTexture)
                    m_tex_diffuse = sh;
                else if (c.GetName() == sgusdAttrOpacityTexture)
                    m_tex_opacity = sh;
                else if (c.GetName() == sgusdAttrBumpTexture)
                    m_tex_bump = sh;
            }
        }
    });
}

void USDMaterialNode::read(UsdTimeCode t)
{
    super::read(t);
    auto& dst = *getNode<MaterialNode>();

    if (m_surface) {
        GetValue(m_in_use_vertex_color, dst.use_vertex_color, t);
        GetValue(m_in_double_sided, dst.double_sided, t);
        GetValue(m_in_diffuse_color, dst.diffuse_color, t);
        GetValue(m_in_diffuse, dst.diffuse, t);
        GetValue(m_in_opacity, dst.opacity, t);
        GetValue(m_in_roughness, dst.roughness, t);
        GetValue(m_in_ambient_color, dst.ambient_color, t);
        GetValue(m_in_specular_color, dst.specular_color, t);
        GetValue(m_in_emissive_color, dst.emissive_color, t);
    }

    auto get_texture = [](TexturePtr& dst, UsdShadeShader& src) {
        if (!src)
            return;
        dst = std::make_shared<Texture>();
        if (auto file = src.GetInput(sgusdAttrFile))
            GetValue(file, dst->file_path);
        if (auto st = src.GetInput(sgusdAttrST))
            GetValue(st, dst->st);
        if (auto ws = src.GetInput(sgusdAttrWrapS))
            GetValue(ws, dst->wrap_s);
        if (auto wt = src.GetInput(sgusdAttrWrapT))
            GetValue(wt, dst->wrap_t);
        if (auto fallback = src.GetInput(sgusdAttrFallback))
            GetValue(fallback, dst->fallback);

    };
    get_texture(dst.diffuse_texture, m_tex_diffuse);
    get_texture(dst.opacity_texture, m_tex_opacity);
    get_texture(dst.bump_texture, m_tex_bump);
}

void USDMaterialNode::beforeWrite()
{
    super::beforeWrite();
    const auto& src = *getNode<MaterialNode>();

    // setup
    if (!m_surface) {
        auto surf_path = m_prim.GetPath().GetString() + "/Surface";
        auto surf_prim = m_scene->getStage()->DefinePrim(SdfPath(surf_path), TfToken(USDShaderNode::getUsdTypeName()));
        m_surface = UsdShadeShader(surf_prim);

        // based on UsdPreviewSurface Proposal
        // https://graphics.pixar.com/usd/docs/UsdPreviewSurface-Proposal.html
        m_surface.SetShaderId(sgusdUsdPreviewSurface);

        m_in_use_vertex_color = m_surface.CreateInput(sgusdAttrUseVertexColor, SdfValueTypeNames->Int);
        m_in_double_sided   = m_surface.CreateInput(sgusdAttrDoubleSided, SdfValueTypeNames->Int);
        m_in_diffuse_color  = m_surface.CreateInput(sgusdAttrDiffuseColor, SdfValueTypeNames->Float3);
        m_in_diffuse        = m_surface.CreateInput(sgusdAttrDiffuse, SdfValueTypeNames->Float);
        m_in_opacity        = m_surface.CreateInput(sgusdAttrOpacity, SdfValueTypeNames->Float);
        m_in_roughness      = m_surface.CreateInput(sgusdAttrRoughness, SdfValueTypeNames->Float);
        m_in_ambient_color  = m_surface.CreateInput(sgusdAttrAmbientColor, SdfValueTypeNames->Float3);
        m_in_specular_color = m_surface.CreateInput(sgusdAttrSpecularColor, SdfValueTypeNames->Float3);
        m_in_emissive_color = m_surface.CreateInput(sgusdAttrEmissiveColor, SdfValueTypeNames->Float3);

        auto sh_surf    = m_surface.CreateOutput(sgusdAttrSurface, SdfValueTypeNames->Token);
        auto mat_surf   = m_material.CreateOutput(sgusdAttrSurface, SdfValueTypeNames->Token);
        auto st         = m_material.CreateInput(sgusdAttrSTName, SdfValueTypeNames->Token);
        auto tangents   = m_material.CreateInput(sgusdAttrTangentsName, SdfValueTypeNames->Token);
        auto binormals  = m_material.CreateInput(sgusdAttrBinormalsName, SdfValueTypeNames->Token);
        auto colors     = m_material.CreateInput(sgusdAttrColorsName, SdfValueTypeNames->Token);
        mat_surf.ConnectToSource(sh_surf);
        SetValue(st, sgusdAttrST);
        SetValue(tangents, sgusdAttrTangents);
        SetValue(binormals, sgusdAttrBinormals);
        SetValue(colors, sgusdAttrColors);

        if (src.shader_type != ShaderType::Unknown)
            SetValue(m_surface.CreateInput(sgusdAttrShaderType, SdfValueTypeNames->Token), src.shader_type);
    }

    // base parameters
    if (m_surface) {
        SetValue(m_in_use_vertex_color, src.use_vertex_color);
        SetValue(m_in_double_sided, src.double_sided);
        SetValue(m_in_diffuse_color, src.diffuse_color);
        SetValue(m_in_diffuse, src.diffuse);
        SetValue(m_in_opacity, src.opacity);
        SetValue(m_in_roughness, src.roughness);
        SetValue(m_in_ambient_color, src.ambient_color);
        SetValue(m_in_specular_color, src.specular_color);
        SetValue(m_in_emissive_color, src.emissive_color);
    }

    // textures
    auto add_texture = [this](const char* name, const Texture& v) {
        auto tex_path = m_prim.GetPath().GetString() + "/" + name;
        auto tex_prim = m_scene->getStage()->DefinePrim(SdfPath(tex_path), TfToken(USDShaderNode::getUsdTypeName()));
        auto tex = UsdShadeShader(tex_prim);
        tex.SetShaderId(sgusdUsdUVTexture);
        SetValue(tex.CreateInput(sgusdAttrFile, SdfValueTypeNames->Asset), v.file_path);
        if (v.st != float2::zero())
            SetValue(tex.CreateInput(sgusdAttrST, SdfValueTypeNames->Float2), v.st);
        if (v.wrap_s != WrapMode::Unknown)
            SetValue(tex.CreateInput(sgusdAttrWrapS, SdfValueTypeNames->Token), v.wrap_s);
        if (v.wrap_t != WrapMode::Unknown)
            SetValue(tex.CreateInput(sgusdAttrWrapT, SdfValueTypeNames->Token), v.wrap_t);
        if (v.fallback != Texture::default_fallback)
            SetValue(tex.CreateInput(sgusdAttrFallback, SdfValueTypeNames->Float4), v.fallback);
        return tex;
    };
    if (!m_tex_diffuse && src.diffuse_texture) {
        m_tex_diffuse = add_texture(sgusdAttrDiffuseTexture, *src.diffuse_texture);
        auto o = m_tex_diffuse.CreateOutput(sgusdAttrRGB, SdfValueTypeNames->Float3);
        m_in_diffuse_color.ConnectToSource(o);
    }
    if (!m_tex_opacity && src.opacity_texture) {
        m_tex_opacity = add_texture(sgusdAttrOpacityTexture, *src.opacity_texture);
        auto o = m_tex_opacity.CreateOutput(sgusdAttrA, SdfValueTypeNames->Float);
        m_in_opacity.ConnectToSource(o);
    }
    if (!m_tex_bump && src.bump_texture) {
        m_tex_bump = add_texture(sgusdAttrBumpTexture, *src.bump_texture);
        auto o = m_tex_bump.CreateOutput(sgusdAttrR, SdfValueTypeNames->Float);
    }
}


USDShaderNode::USDShaderNode(USDNode* parent, UsdPrim prim)
    : super(parent, prim, true)
{
    m_shader = UsdShadeShader(prim);

//#ifdef mqusdDebug
//    PrintPrim(m_prim, PF_Full);
//#endif
}

USDShaderNode::USDShaderNode(Node* n, UsdPrim prim)
    : super(n, prim)
{
    m_shader = UsdShadeShader(prim);
}



#define EachNodeType(Body)\
    Body(Mesh, USDMeshNode)\
    Body(Blendshape, USDBlendshapeNode)\
    Body(SkelRoot, USDSkelRootNode)\
    Body(Skeleton, USDSkeletonNode)\
    Body(Instancer, USDInstancerNode)\
    Body(Xform, USDXformNode)\
    Body(Material, USDMaterialNode)\
    Body(Unknown, USDNode)


static thread_local USDScene* g_current_scene;

USDScene* USDScene::getCurrent()
{
    return g_current_scene;
}

USDScene::USDScene(Scene* scene)
    : m_scene(scene)
{
}

USDScene::~USDScene()
{
    close();
}

bool USDScene::open(const char* path_)
{
    m_scene->path = path_;
    m_stage = UsdStage::Open(path_);
    if (!m_stage)
        return false;

    g_current_scene = this;

    m_scene->frame_rate = m_frame_rate = m_stage->GetFramesPerSecond();
    m_scene->time_start = m_stage->GetStartTimeCode() / m_frame_rate;
    m_scene->time_end = m_stage->GetEndTimeCode() / m_frame_rate;

    //    auto masters = m_stage->GetMasters();
    //    for (auto& m : masters) {
    //        m_masters.push_back(createSchemaRecursive(nullptr, m));
    //}
    auto root = m_stage->GetPseudoRoot();
    if (root.IsValid()) {
        m_root = new USDRootNode(root);
        constructTree(m_root);
    }
    for (auto& node : m_nodes)
        node->beforeRead();
    return true;
}

bool USDScene::create(const char* path_)
{
    // UsdStage::CreateNew() will fail if the file already exists. try to delete existing one.
    if (FILE* f = fopen(path_, "rb")) {
        fclose(f);
        std::remove(path_);
    }

    m_scene->path = path_;
    m_stage = UsdStage::CreateNew(m_scene->path);
    if (m_stage) {
        g_current_scene = this;

        m_frame_rate = m_scene->frame_rate;
        m_stage->SetFramesPerSecond(m_scene->frame_rate);

        auto root = m_stage->GetPseudoRoot();
        if (root.IsValid()) {
            m_root = new USDRootNode(root);
            constructTree(m_root);
        }
    }
    return m_stage;
}

bool USDScene::save()
{
    if (m_stage) {
        return m_stage->GetRootLayer()->Save();
    }
    return false;
}

void USDScene::registerNode(USDNode* n)
{
    if (n) {
        m_nodes.push_back(USDNodePtr(n));
        m_node_table[n->getPath()] = n;
        m_scene->registerNode(n->m_node);
    }
}

void USDScene::constructTree(USDNode* n)
{
#ifdef mqusdDebug
    PrintPrim(n->m_prim);
#endif
    registerNode(n);

    auto& prim = n->m_prim;
    auto children = prim.GetChildren();
    for (auto cprim : children) {
        USDNode* c = nullptr;

        // note: SkelAnimation is hidden on mqusdSceneGraph.h side
        if (!c) {
            UsdSkelAnimation schema(cprim);
            if (schema)
                c = new USDSkelAnimationNode(n, cprim);
        }
        if (!c) {
            UsdShadeShader schema(cprim);
            if (schema)
                c = new USDShaderNode(n, cprim);
        }

#define Case(E, T)\
        if (!c) {\
            T::UsdType schema(cprim);\
            if (schema)\
                c = new T(n, cprim);\
        }
        EachNodeType(Case)
#undef Case

        if (!c) {
            c = new USDNode(n, cprim, true);
        }

        constructTree(c);
    }
}

void USDScene::close()
{
    m_stage = {};
}

void USDScene::read()
{
    g_current_scene = this;
    double time = m_scene->time_current;
    UsdTimeCode t = toTimeCode(time);

    for (auto& n : m_nodes)
        n->read(t);
    ++m_read_count;
}

void USDScene::write()
{
    g_current_scene = this;
    double time = m_scene->time_current;
    UsdTimeCode t = toTimeCode(time);
    if (m_write_count == 0)
        m_prev_time = t;

    for (auto& n : m_nodes) {
        if (n->m_write_count++ == 0)
            n->beforeWrite();
        n->write(t);
    }
    ++m_write_count;
    m_prev_time = t;

    if (!std::isnan(time) && time > m_max_time) {
        m_stage->SetEndTimeCode(time * m_frame_rate);
        m_max_time = time;
    }
}

template<class NodeT>
USDNode* USDScene::createNodeImpl(USDNode* parent, std::string path)
{
    auto prim = m_stage->DefinePrim(SdfPath(path), TfToken(NodeT::getUsdTypeName()));
    if (prim) {
#ifdef mqusdDebug
        PrintPrim(prim);
#endif
        auto ret = new NodeT(parent, prim);
        return ret;
    }
    return nullptr;
}

Node* USDScene::createNode(Node* parent, const char* name, Node::Type type)
{
    g_current_scene = this;

    std::string path;
    if (parent) {
        path = parent->getPath();
        if (path != "/")
            path += '/';
    }
    path += EncodeNodeName(name);

    USDNode* ret = nullptr;
    USDNode* usd_parent = parent ? (USDNode*)parent->impl : nullptr;
    switch (type) {
#define Case(E, T) case Node::Type::E: ret = createNodeImpl<T>(usd_parent, path); break;
        EachNodeType(Case);
#undef Case
    default: break;
    }

    registerNode(ret);
    return ret ? ret->m_node : nullptr;
}

template<class NodeT>
USDNode* USDScene::wrapNodeImpl(Node* node)
{
    USDNode* usd_parent = nullptr;
    std::string path = EncodeNodePath(node->getPath());

    UsdPrim prim;
    if (path == "/")
        prim = m_stage->GetPseudoRoot();
    else
        prim = m_stage->DefinePrim(SdfPath(path), TfToken(NodeT::getUsdTypeName()));

    if (prim) {
        auto ret = new NodeT(node, prim);
        m_node_table[path] = ret;
        return ret;
    }
    else {
        sgDbgPrint("USDScene::wrapNodeImpl(): failed to create prim %s\n", path.c_str());
    }
    return nullptr;
}

bool USDScene::wrapNode(Node* node)
{
    USDNode* ret = nullptr;
    switch (node->getType()) {
#define Case(E, T) case Node::Type::E: ret = wrapNodeImpl<T>(node); break;
        EachNodeType(Case);
#undef Case
    default: break;
    }

    if (ret) {
        m_nodes.push_back(USDNodePtr(ret));
    }
    return ret;
}

UsdStageRefPtr& USDScene::getStage()
{
    return m_stage;
}

UsdTimeCode USDScene::toTimeCode(double time) const
{
    return UsdTimeCode(time * m_frame_rate);
}

UsdTimeCode USDScene::getPrevTime() const
{
    return m_prev_time;
}

USDNode* USDScene::findUSDNodeImpl(const std::string& path)
{
    auto it = m_node_table.find(path);
    return it == m_node_table.end() ? nullptr : it->second;
}

Node* USDScene::findNodeImpl(const std::string& path)
{
    if (USDNode* n = findUSDNodeImpl(path))
        return n->m_node;
    return nullptr;
}

} // namespace sg

sgusdAPI sg::SceneInterface* sgusdCreateSceneInterface(sg::Scene *scene)
{
    static std::once_flag s_once;
    std::call_once(s_once, []() {
        // register plugins
        std::string pluginfo_path = mu::GetCurrentModuleDirectory();
        pluginfo_path += muPathSep "usd" muPathSep "plugInfo.json";
        PlugRegistry::GetInstance().RegisterPlugins(pluginfo_path);
    });

    return new sg::USDScene(scene);
}
