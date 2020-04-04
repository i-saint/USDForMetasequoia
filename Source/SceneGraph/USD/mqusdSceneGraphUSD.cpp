#include "pch.h"
#include "mqusdSceneGraphUSD.h"
#include "mqusdUtilsUSD.h"

#ifdef _WIN32
    #pragma comment(lib, "usd_ms.lib")
#endif

namespace mqusd {

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

void USDNode::beforeRead()
{
}

void USDNode::read(double time)
{
}

void USDNode::beforeWrite()
{
}

void USDNode::write(double time)
{

}

void USDNode::setNode(Node* node)
{
    m_node = node;
    m_node->impl = this;
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

void USDXformNode::read(double time)
{
    super::read(time);

    auto t = m_scene->toTimeCode(time);
    auto& dst = static_cast<XformNode&>(*m_node);

    GfMatrix4d mat;
    bool reset_stack = false;
    m_xform.GetLocalTransformation(&mat, &reset_stack, t);
    dst.local_matrix.assign((double4x4&)mat);

    if (dst.parent_xform)
        dst.global_matrix = dst.local_matrix * dst.parent_xform->global_matrix;
    else
        dst.global_matrix = dst.local_matrix;
}

void USDXformNode::write(double time)
{
    super::write(time);

    auto t = m_scene->toTimeCode(time);
    auto& src = static_cast<XformNode&>(*m_node);

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
    auto& dst = static_cast<MeshNode&>(*m_node);

    // find attributes
    auto pvapi = UsdGeomPrimvarsAPI(m_prim);
    m_pv_st     = pvapi.GetPrimvar(mqusdAttrST);
    m_pv_colors = pvapi.GetPrimvar(mqusdAttrColors);
    m_attr_mids     = m_prim.GetAttribute(mqusdAttrMaterialIDs);
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

        transform_container(m_blendshapes, paths, [this, &dst](auto*& d, auto& s) {
            d = m_scene->findNode<USDBlendshapeNode>(s.GetString());
            if (d) {
                dst.blendshapes.push_back(d->getNode<BlendshapeNode>());
            }
            else {
                mqusdDbgPrint("not found %s\n", s.GetText());
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
                    mqusdDbgPrint("not found %s\n", s.GetText());
                }
            });
        }
        else {
            transform_container(dst.joints, dst.skeleton->joints, [](auto*& d, auto& s) {
                d = s.get();
            });
        }
    }
}

void USDMeshNode::read(double time)
{
    super::read(time);

    auto t = m_scene->toTimeCode(time);
    auto& dst = static_cast<MeshNode&>(*m_node);

    // counts, indices, points
    {
        m_mesh.GetFaceVertexCountsAttr().Get(&m_counts, t);
        dst.counts.share(m_counts.cdata(), m_counts.size());
    }
    {
        m_mesh.GetFaceVertexIndicesAttr().Get(&m_indices, t);
        dst.indices.share(m_indices.cdata(), m_indices.size());
    }
    {
        m_mesh.GetPointsAttr().Get(&m_points, t);
        dst.points.share((float3*)m_points.cdata(), m_points.size());
    }


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

    // material ids
    if (m_attr_mids) {
        m_attr_mids.Get(&m_material_ids, t);
        dst.material_ids.share(m_material_ids.cdata(), m_material_ids.size());
    }

    // blendshape weights
    if (m_animation && m_blendshapes.size() == m_blendshape_ids.size()) {
        auto& data = m_animation->getBlendshapeData(time);
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

    auto mbapi = UsdShadeMaterialBindingAPI(m_prim);
    // material
    if (auto mat = mbapi.ComputeBoundMaterial()) {
        // bound single material
        dst.facesets.resize(1);
        auto& faceset = dst.facesets.front();
        if (!faceset)
            faceset = std::make_shared<FaceSet>();

        faceset->faces.resize_discard(dst.counts.size());
        std::iota(faceset->faces.begin(), faceset->faces.end(), 0);
        faceset->material = m_scene->findNode<MaterialNode>(mat.GetPath().GetString());
    }
    else {
        // handle subsets
        auto subsets = mbapi.GetMaterialBindSubsets();
        m_subset_faces.resize(subsets.size());
        dst.facesets.resize(subsets.size());
        each_with_index(subsets, [this, &t, &dst](UsdGeomSubset& subset, int i) {
            auto& faces = m_subset_faces[i];
            subset.GetIndicesAttr().Get(&faces, t);

            auto& faceset = dst.facesets[i];
            if (!faceset)
                faceset = std::make_shared<FaceSet>();
            faceset->faces.share(faces.cdata(), faces.size());

            if (auto mat = UsdShadeMaterialBindingAPI(subset).ComputeBoundMaterial())
                faceset->material = m_scene->findNode<MaterialNode>(mat.GetPath().GetString());
        });
    }

    // validate
    dst.validate();
}


void USDMeshNode::beforeWrite()
{
    // create attributes
    auto pvapi = UsdGeomPrimvarsAPI(m_prim);
    m_pv_st = pvapi.CreatePrimvar(mqusdAttrST, SdfValueTypeNames->TexCoord2fArray);
    m_pv_colors = pvapi.CreatePrimvar(mqusdAttrColors, SdfValueTypeNames->Color4fArray);

    auto& src = static_cast<MeshNode&>(*m_node);

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

void USDMeshNode::write(double time)
{
    super::write(time);

    auto t = m_scene->toTimeCode(time);
    auto& src = static_cast<MeshNode&>(*m_node);
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

    if (!src.material_ids.empty()) {
        if (!m_attr_mids)
            m_attr_mids = m_prim.CreateAttribute(mqusdAttrMaterialIDs, SdfValueTypeNames->IntArray, false);
        m_material_ids.assign(src.material_ids.begin(), src.material_ids.end());
        m_attr_mids.Set(m_material_ids, t);
    }
    else {
        auto mbapi = UsdShadeMaterialBindingAPI(m_prim);
        for (auto& fs : src.facesets) {
            auto mat = fs->material;
            if (!mat)
                continue;

            VtArray<int> faces(fs->faces.begin(), fs->faces.end());
            auto subset = mbapi.CreateMaterialBindSubset(TfToken(mat->getName()), faces);
            auto mat_node = static_cast<USDMaterialNode*>(mat->impl);
            UsdShadeMaterialBindingAPI(subset.GetPrim()).Bind(mat_node->m_material);
        }
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

void USDBlendshapeNode::read(double time)
{
    super::read(time);
    auto& dst = *static_cast<BlendshapeNode*>(m_node);

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
    auto& src = *static_cast<const BlendshapeNode*>(m_node);

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
    auto& dst = *static_cast<SkelRootNode*>(m_node);

    // resolve animation
    if (auto rel_anim = m_prim.GetRelationship(UsdSkelTokens->skelAnimationSource)) {
        SdfPathVector paths;
        rel_anim.GetTargets(&paths);
        if (!paths.empty()) {
            if (auto usd_anim = m_scene->findUSDNode<USDSkelAnimationNode>(paths.front().GetString())) {
                // update child meshes
                eachChildR([usd_anim](USDNode* n) {
                    if (auto usd_mesh = dynamic_cast<USDMeshNode*>(n))
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
            if (auto usd_skel = m_scene->findUSDNode<USDSkeletonNode>(paths.front().GetString())) {
                auto skel = usd_skel->getNode<SkeletonNode>();
                dst.skeleton = skel;

                // update child meshes
                eachChildR([usd_skel, skel](USDNode* n) {
                    if (auto usd_mesh = dynamic_cast<USDMeshNode*>(n)) {
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

    auto& src = *static_cast<SkelRootNode*>(m_node);
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
    auto& dst = *static_cast<SkeletonNode*>(m_node);
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

void USDSkeletonNode::read(double time)
{
    super::read(time);

    auto t = m_scene->toTimeCode(time);
    auto& dst = *static_cast<SkeletonNode*>(m_node);

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
    auto& src = *static_cast<SkeletonNode*>(m_node);
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

void USDSkeletonNode::write(double time)
{
    super::write(time);

    auto t = m_scene->toTimeCode(time);
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

std::vector<const USDSkelAnimationNode::BlendshapeData*>& USDSkelAnimationNode::getBlendshapeData(double time)
{
    if (time != m_prev_read) {
        m_prev_read = time;

        auto t = m_scene->toTimeCode(time);
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
    auto& dst = *static_cast<InstancerNode*>(m_node);

    auto rel = m_instancer.GetPrototypesRel();
    if (rel) {
        SdfPathVector paths;
        rel.GetTargets(&paths);
        for (auto& path : paths) {
            if (auto proto = m_scene->findUSDNode(path.GetString()))
                dst.protos.push_back(proto->m_node);
        }
    }
}

void USDInstancerNode::read(double time)
{
    super::read(time);

    auto t = m_scene->toTimeCode(time);
    auto& dst = *static_cast<InstancerNode*>(m_node);

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
    auto& src = *static_cast<InstancerNode*>(m_node);

    auto rel = m_instancer.CreatePrototypesRel();
    if (rel) {
        SdfPathVector paths;
        for (auto& proto : src.protos)
            paths.push_back(SdfPath(proto->getPath()));
        rel.SetTargets(paths);
    }
}

void USDInstancerNode::write(double time)
{
    super::write(time);

    auto t = m_scene->toTimeCode(time);
    auto& src = *static_cast<InstancerNode*>(m_node);

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
    EachChild(m_prim, [this](UsdPrim& c) {
        UsdShadeShader sh(c);
        if (!sh)
            return;

        TfToken id;
        if (sh.GetIdAttr().Get(&id)) {
            if (id == mqusdUsdPreviewSurface) {
                m_surface = sh;
                m_in_use_vertex_color = m_surface.GetInput(mqusdAttrUseVertexColor);
                m_in_double_sided   = m_surface.GetInput(mqusdAttrDoubleSided);
                m_in_diffuse_color  = m_surface.GetInput(mqusdAttrDiffuseColor);
                m_in_diffuse        = m_surface.GetInput(mqusdAttrDiffuse);
                m_in_opacity        = m_surface.GetInput(mqusdAttrOpacity);
                m_in_roughness      = m_surface.GetInput(mqusdAttrRoughness);
                m_in_ambient_color  = m_surface.GetInput(mqusdAttrAmbientColor);
                m_in_specular_color = m_surface.GetInput(mqusdAttrSpecularColor);
                m_in_emissive_color = m_surface.GetInput(mqusdAttrEmissiveColor);

                auto& dst = *static_cast<MaterialNode*>(m_node);
                GetValue(m_surface.GetInput(mqusdAttrShaderType), dst.shader_type);
            }
            else if (id == mqusdUsdUVTexture) {
                if (c.GetName() == mqusdAttrDiffuseTexture)
                    m_tex_diffuse = sh;
                else if (c.GetName() == mqusdAttrOpacityTexture)
                    m_tex_opacity = sh;
                else if (c.GetName() == mqusdAttrBumpTexture)
                    m_tex_bump = sh;
            }
        }
    });

    read(default_time);
}

void USDMaterialNode::read(double time)
{
    super::read(time);

    auto t = m_scene->toTimeCode(time);
    auto& dst = *static_cast<MaterialNode*>(m_node);
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
        if (auto file = src.GetInput(mqusdAttrFile))
            GetValue(file, dst->file_path);
        if (auto st = src.GetInput(mqusdAttrST))
            GetValue(st, dst->st);
        if (auto ws = src.GetInput(mqusdAttrWrapS))
            GetValue(ws, dst->wrap_s);
        if (auto wt = src.GetInput(mqusdAttrWrapT))
            GetValue(wt, dst->wrap_t);
        if (auto fallback = src.GetInput(mqusdAttrFallback))
            GetValue(fallback, dst->fallback);

    };
    get_texture(dst.diffuse_texture, m_tex_diffuse);
    get_texture(dst.opacity_texture, m_tex_opacity);
    get_texture(dst.bump_texture, m_tex_bump);
}

void USDMaterialNode::beforeWrite()
{
    auto& src = *static_cast<const MaterialNode*>(m_node);

    // setup
    if (!m_surface) {
        auto surf_path = m_prim.GetPath().GetString() + "/Surface";
        auto surf_prim = m_scene->getStage()->DefinePrim(SdfPath(surf_path), TfToken(USDShaderNode::getUsdTypeName()));
        m_surface = UsdShadeShader(surf_prim);

        // based on UsdPreviewSurface Proposal
        // https://graphics.pixar.com/usd/docs/UsdPreviewSurface-Proposal.html
        m_surface.SetShaderId(mqusdUsdPreviewSurface);

        m_in_use_vertex_color = m_surface.CreateInput(mqusdAttrUseVertexColor, SdfValueTypeNames->Int);
        m_in_double_sided   = m_surface.CreateInput(mqusdAttrDoubleSided, SdfValueTypeNames->Int);
        m_in_diffuse_color  = m_surface.CreateInput(mqusdAttrDiffuseColor, SdfValueTypeNames->Float3);
        m_in_diffuse        = m_surface.CreateInput(mqusdAttrDiffuse, SdfValueTypeNames->Float);
        m_in_opacity        = m_surface.CreateInput(mqusdAttrOpacity, SdfValueTypeNames->Float);
        m_in_roughness      = m_surface.CreateInput(mqusdAttrRoughness, SdfValueTypeNames->Float);
        m_in_ambient_color  = m_surface.CreateInput(mqusdAttrAmbientColor, SdfValueTypeNames->Float3);
        m_in_specular_color = m_surface.CreateInput(mqusdAttrSpecularColor, SdfValueTypeNames->Float3);
        m_in_emissive_color = m_surface.CreateInput(mqusdAttrEmissiveColor, SdfValueTypeNames->Float3);

        auto sh_surf    = m_surface.CreateOutput(mqusdAttrSurface, SdfValueTypeNames->Token);
        auto mat_surf   = m_material.CreateOutput(mqusdAttrSurface, SdfValueTypeNames->Token);
        auto st         = m_material.CreateInput(mqusdAttrSTName, SdfValueTypeNames->Token);
        auto tangents   = m_material.CreateInput(mqusdAttrTangentsName, SdfValueTypeNames->Token);
        auto binormals  = m_material.CreateInput(mqusdAttrBinormalsName, SdfValueTypeNames->Token);
        auto colors     = m_material.CreateInput(mqusdAttrColorsName, SdfValueTypeNames->Token);
        mat_surf.ConnectToSource(sh_surf);
        SetValue(st, mqusdAttrST);
        SetValue(tangents, mqusdAttrTangents);
        SetValue(binormals, mqusdAttrBinormals);
        SetValue(colors, mqusdAttrColors);

        if (src.shader_type != ShaderType::Unknown)
            SetValue(m_surface.CreateInput(mqusdAttrShaderType, SdfValueTypeNames->Token), src.shader_type);
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
        tex.SetShaderId(mqusdUsdUVTexture);
        SetValue(tex.CreateInput(mqusdAttrFile, SdfValueTypeNames->Asset), v.file_path);
        if (v.st != float2::zero())
            SetValue(tex.CreateInput(mqusdAttrST, SdfValueTypeNames->Float2), v.st);
        if (v.wrap_s != WrapMode::Unknown)
            SetValue(tex.CreateInput(mqusdAttrWrapS, SdfValueTypeNames->Token), v.wrap_s);
        if (v.wrap_t != WrapMode::Unknown)
            SetValue(tex.CreateInput(mqusdAttrWrapT, SdfValueTypeNames->Token), v.wrap_t);
        if (v.fallback != Texture::default_fallback)
            SetValue(tex.CreateInput(mqusdAttrFallback, SdfValueTypeNames->Float4), v.fallback);
        return tex;
    };
    if (!m_tex_diffuse && src.diffuse_texture) {
        m_tex_diffuse = add_texture(mqusdAttrDiffuseTexture, *src.diffuse_texture);
        auto o = m_tex_diffuse.CreateOutput(mqusdAttrRGB, SdfValueTypeNames->Float3);
        m_in_diffuse_color.ConnectToSource(o);
    }
    if (!m_tex_opacity && src.opacity_texture) {
        m_tex_opacity = add_texture(mqusdAttrOpacityTexture, *src.opacity_texture);
        auto o = m_tex_opacity.CreateOutput(mqusdAttrA, SdfValueTypeNames->Float);
        m_in_opacity.ConnectToSource(o);
    }
    if (!m_tex_bump && src.bump_texture) {
        m_tex_bump = add_texture(mqusdAttrBumpTexture, *src.bump_texture);
        auto o = m_tex_bump.CreateOutput(mqusdAttrR, SdfValueTypeNames->Float);
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

    for (auto& n : m_nodes)
        n->read(time);
    ++m_read_count;
}

void USDScene::write()
{
    g_current_scene = this;
    double time = m_scene->time_current;

    if (m_write_count == 0) {
        for (auto& n : m_nodes)
            n->beforeWrite();
    }
    for (auto& n : m_nodes)
        n->write(time);
    ++m_write_count;

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
    path += SanitizeNodeName(name);

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
    std::string path = SanitizeNodePath(node->getPath());

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
        mqusdDbgPrint("USDScene::wrapNodeImpl(): failed to create prim %s\n", path.c_str());
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

} // namespace mqusd

mqusdCoreAPI mqusd::SceneInterface* mqusdCreateUSDSceneInterface(mqusd::Scene *scene)
{
    return new mqusd::USDScene(scene);
}
