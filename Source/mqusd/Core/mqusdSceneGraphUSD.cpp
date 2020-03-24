#include "pch.h"
#include "mqusd.h"
#include "mqusdSceneGraphUSD.h"

#ifdef _WIN32
    #pragma comment(lib, "usd_ms.lib")
#endif

namespace mqusd {

static void PrintPrim(UsdPrim prim)
{
    std::stringstream ss;
    mu::Print("prim %s (%s)\n", prim.GetPath().GetText(), prim.GetTypeName().GetText());
    for (auto& attr : prim.GetAuthoredAttributes()) {
        mu::Print("  attr %s (%s)\n", attr.GetName().GetText(), attr.GetTypeName().GetAsToken().GetText());
        //for (auto& kvp : prim.GetAllAuthoredMetadata()) {
        //    ss << kvp.second;
        //    mu::Print("    meta %s (%s) - %s\n", kvp.first.GetText(), kvp.second.GetTypeName().c_str(), ss.str().c_str());
        //    ss.str({});
        //}
    }
    for (auto& rel : prim.GetAuthoredRelationships()) {
        SdfPathVector paths;
        rel.GetTargets(&paths);
        for (auto& path : paths) {
            mu::Print("  rel %s %s\n", rel.GetName().GetText(), path.GetText());
        }
    }
}


template<class NodeT>
NodeT* CreateNode(USDNode* parent, UsdPrim prim)
{
    auto pnode = parent ? parent->m_node : nullptr;
    return new NodeT(
        parent ? parent->m_node : nullptr,
        prim.GetName().GetText());
}

USDNode::USDNode(USDNode* parent, UsdPrim prim, bool create_node)
    : m_prim(prim)
    , m_parent(parent)
{
    m_scene = USDScene::getCurrent();
    if (m_parent)
        m_parent->m_children.push_back(this);
    if (create_node)
        setNode(CreateNode<Node>(parent, prim));
}

USDNode::USDNode(Node* n, UsdPrim prim)
    : m_prim(prim)
    , m_node(n)
{

    m_scene = USDScene::getCurrent();

    m_node->impl = this;
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

    auto t = UsdTimeCode(time);
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

    auto t = UsdTimeCode(time);
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
    m_attr_mids = m_prim.GetAttribute(mqusdAttrMaterialIDs);
    m_attr_uv = m_prim.GetAttribute(mqusdAttrUV);
    m_attr_uv_indices = m_prim.GetAttribute(mqusdAttrUVIndices);
    m_attr_joints = m_prim.GetAttribute(UsdSkelTokens->skelJoints);
    m_attr_joint_indices = m_prim.GetAttribute(UsdSkelTokens->primvarsSkelJointIndices);
    m_attr_joint_weights = m_prim.GetAttribute(UsdSkelTokens->primvarsSkelJointWeights);
    m_attr_bind_transform = m_prim.GetAttribute(UsdSkelTokens->primvarsSkelGeomBindTransform);

    // resolve blendshapes
    auto rel_blendshapes = m_prim.GetRelationship(UsdSkelTokens->skelBlendShapeTargets);
    if (rel_blendshapes) {
        SdfPathVector paths;
        rel_blendshapes.GetTargets(&paths);
        dst.blendshapes.clear();
        for (auto& path : paths) {
            if (auto n = m_scene->findNode(path.GetString()))
                dst.blendshapes.push_back(static_cast<BlendshapeNode*>(n->m_node));
        }
    }

    // resolve skeleton
    // note: skeleton may be related to SkelRoot and be resolved by USDSkelRoot::beforeRead()
    auto rel_skel = m_prim.GetRelationship(UsdSkelTokens->skelSkeleton);
    if (rel_skel) {
        SdfPathVector paths;
        rel_skel.GetTargets(&paths);
        if (!paths.empty()) {
            if (auto n = m_scene->findNode(paths.front().GetString()))
                dst.skeleton = static_cast<SkeletonNode*>(n->m_node);
        }
    }

    // resolve joints
    if (m_attr_joints) {
        VtArray<TfToken> data;
        m_attr_joints.Get(&data);
        dst.joint_paths.clear();
        for (auto& t : data)
            dst.joint_paths.push_back(t.GetString());
    }
}

void USDMeshNode::read(double time)
{
    super::read(time);

    auto t = UsdTimeCode(time);
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

    // normals
    {
        m_mesh.GetNormalsAttr().Get(&m_normals, t);

        if (m_normals.size() == dst.indices.size()) {
            dst.normals.share((float3*)m_normals.cdata(), m_normals.size());
        }
        else if (m_normals.size() == dst.points.size()) {
            dst.normals.resize_discard(dst.indices.size());
            mu::CopyWithIndices(dst.normals.data(), (float3*)m_normals.cdata(), dst.indices);
        }
        else {
            dst.normals.clear();
        }
    }

    // uv
    if (m_attr_uv) {
        m_attr_uv.Get(&m_uvs, t);

        if (m_attr_uv_indices) {
            m_attr_uv_indices.Get(&m_uv_indices, t);
            if (m_uv_indices.size() == dst.indices.size()) {
                dst.uvs.resize_discard(m_uv_indices.size());
                mu::CopyWithIndices(dst.uvs.data(), (float2*)m_uvs.cdata(), m_uv_indices);
            }
            else {
                dst.uvs.clear();
            }
        }
        else {
            if (m_uvs.size() == dst.indices.size()) {
                dst.uvs.share((float2*)m_uvs.cdata(), m_uvs.size());
            }
            else if (m_uvs.size() == dst.points.size()) {
                dst.uvs.resize_discard(dst.indices.size());
                mu::CopyWithIndices(dst.uvs.data(), (float2*)m_uvs.cdata(), dst.indices);
            }
            else {
                dst.uvs.clear();
            }
        }
    }

    // material ids
    if (m_attr_mids) {
        m_attr_mids.Get(&m_material_ids, t);
        dst.material_ids.share(m_material_ids.cdata(), m_material_ids.size());
    }

    // skel
    if (m_attr_joint_indices && m_attr_joint_weights) {
        TfToken interpolation;
        m_attr_joint_indices.GetMetadata(UsdGeomTokens->elementSize, &dst.joints_per_vertex);
        m_attr_joint_indices.GetMetadata(UsdGeomTokens->interpolation, &interpolation);

        m_attr_joint_indices.Get(&m_joint_indices);
        dst.joint_indices.share(m_joint_indices.cdata(), m_joint_indices.size());

        m_attr_joint_weights.Get(&m_joint_weights);
        dst.joint_weights.share(m_joint_weights.cdata(), m_joint_weights.size());

        if (m_attr_bind_transform) {
            GfMatrix4d data;
            m_attr_bind_transform.Get(&data);
            dst.bind_transform.assign((float4x4&)data);
        }
    }

    // validate
    dst.validate();
}


void USDMeshNode::beforeWrite()
{
    // create attributes
    m_attr_mids = m_prim.CreateAttribute(mqusdAttrMaterialIDs, SdfValueTypeNames->IntArray, false);
    m_attr_uv = m_prim.CreateAttribute(mqusdAttrUV, SdfValueTypeNames->Float2Array, false);

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

    auto t = UsdTimeCode(time);
    auto& src = static_cast<MeshNode&>(*m_node);
    {
        m_counts.assign(src.counts.begin(), src.counts.end());
        m_mesh.GetFaceVertexCountsAttr().Set(m_counts, t);
    }
    {
        m_indices.assign(src.indices.begin(), src.indices.end());
        m_mesh.GetFaceVertexIndicesAttr().Set(m_indices, t);
    }
    {
        m_points.assign((GfVec3f*)src.points.begin(), (GfVec3f*)src.points.end());
        m_mesh.GetPointsAttr().Set(m_points, t);
    }
    if (!src.normals.empty()) {
        m_normals.assign((GfVec3f*)src.normals.begin(), (GfVec3f*)src.normals.end());
        m_mesh.GetNormalsAttr().Set(m_normals, t);
    }
    if (m_attr_uv && !src.uvs.empty()) {
        m_uvs.assign((GfVec2f*)src.uvs.begin(), (GfVec2f*)src.uvs.end());
        m_attr_uv.Set(m_uvs, t);
    }
    if (m_attr_mids && !src.material_ids.empty()) {
        m_material_ids.assign(src.material_ids.begin(), src.material_ids.end());
        m_attr_mids.Set(m_material_ids, t);
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

    {
        m_blendshape.GetPointIndicesAttr().Get(&m_point_indices);
        dst.indices.share(m_point_indices.cdata(), m_point_indices.size());
    }
    {
        m_blendshape.GetOffsetsAttr().Get(&m_point_offsets);
        dst.point_offsets.share((float3*)m_point_offsets.cdata(), m_point_offsets.size());
    }
    {
        m_blendshape.GetNormalOffsetsAttr().Get(&m_normal_offsets);
        dst.normal_offsets.share((float3*)m_normal_offsets.cdata(), m_normal_offsets.size());
    }
}

void USDBlendshapeNode::beforeWrite()
{
    super::beforeWrite();

    auto& src = *static_cast<const BlendshapeNode*>(m_node);

    // assume not time sampled. write once.
    if (!src.indices.empty()) {
        m_point_indices.assign(src.indices.begin(), src.indices.end());
        m_blendshape.GetPointIndicesAttr().Set(m_point_indices);
    }
    if (!src.point_offsets.empty()) {
        m_point_offsets.assign((GfVec3f*)src.point_offsets.begin(), (GfVec3f*)src.point_offsets.end());
        m_blendshape.GetOffsetsAttr().Set(m_point_offsets);
    }
    if (!src.normal_offsets.empty()) {
        m_normal_offsets.assign((GfVec3f*)src.normal_offsets.begin(), (GfVec3f*)src.normal_offsets.end());
        m_blendshape.GetNormalOffsetsAttr().Set(m_normal_offsets);
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

    // resolve skeleton
    auto rel_skel = m_prim.GetRelationship(UsdSkelTokens->skelSkeleton);
    if (rel_skel) {
        SdfPathVector paths;
        rel_skel.GetTargets(&paths);
        if (!paths.empty()) {
            if (auto skel_node = m_scene->findNode(paths.front().GetString())) {
                auto skel = static_cast<SkeletonNode*>(skel_node->m_node);
                dst.skeleton = skel;

                // update child meshes
                eachChildR([skel](USDNode* n) {
                    if (n->m_node->getType() == Node::Type::Mesh) {
                        auto mesh_node = static_cast<MeshNode*>(n->m_node);
                        if (!mesh_node->skeleton)
                            mesh_node->skeleton = skel;
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
}

USDSkeletonNode::USDSkeletonNode(Node* n, UsdPrim prim)
    : super(n, prim)
{
    m_skel = UsdSkelSkeleton(prim);
}

void USDSkeletonNode::beforeRead()
{
    super::beforeRead();
    auto& dst = *static_cast<SkeletonNode*>(m_node);

    // build joints
    VtArray<TfToken> data;
    m_skel.GetJointsAttr().Get(&data);
    for (auto& token : data)
        dst.addJoint(token.GetString());
}

void USDSkeletonNode::read(double time)
{
    super::read(time);

    auto t = UsdTimeCode(time);
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
    if (auto query = m_skel_cache.GetSkelQuery(m_skel)) {
        VtArray<GfMatrix4d> data;
        query.ComputeJointLocalTransforms(&data, time);
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

    auto t = UsdTimeCode(time);
    // todo
}


USDMaterialNode::USDMaterialNode(USDNode* parent, UsdPrim prim)
    : super(parent, prim, false)
{
    m_material = UsdShadeMaterial(prim);

    setNode(CreateNode<MaterialNode>(parent, prim));
}

USDMaterialNode::USDMaterialNode(Node* n, UsdPrim prim)
    : super(n, prim)
{
    m_material = UsdShadeMaterial(prim);
}

void USDMaterialNode::read(double time)
{
    super::read(time);
    // todo
}

void USDMaterialNode::write(double time)
{
    super::write(time);
    // todo
}


static thread_local USDScene* s_current_scene;

USDScene* USDScene::getCurrent()
{
    return s_current_scene;
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

    s_current_scene = this;

    m_scene->time_start = m_stage->GetStartTimeCode();
    m_scene->time_end = m_stage->GetEndTimeCode();

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
        s_current_scene = this;

        auto root = m_stage->GetPseudoRoot();
        if (root.IsValid()) {
            m_root = new USDRootNode(root);
            m_scene->root_node = static_cast<RootNode*>(m_root->m_node);
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

void USDScene::constructTree(USDNode* n)
{
#ifdef mqusdDebug
    PrintPrim(n->m_prim);
#endif

    m_nodes.push_back(USDNodePtr(n));
    m_node_table[n->getPath()] = n;
    m_scene->nodes.push_back(NodePtr(n->m_node));

    auto& prim = n->m_prim;
    auto children = prim.GetChildren();
    for (auto cprim : children) {
        USDNode* c = nullptr;

        if (!c) {
            UsdGeomMesh schema(cprim);
            if (schema)
                c = new USDMeshNode(n, cprim);
        }
        if (!c) {
            UsdSkelBlendShape schema(cprim);
            if (schema)
                c = new USDBlendshapeNode(n, cprim);
        }
        if (!c) {
            UsdSkelRoot schema(cprim);
            if (schema)
                c = new USDSkelRootNode(n, cprim);
        }
        if (!c) {
            UsdSkelSkeleton schema(cprim);
            if (schema)
                c = new USDSkeletonNode(n, cprim);
        }
        if (!c) {
            UsdShadeMaterial schema(cprim);
            if (schema)
                c = new USDMaterialNode(n, cprim);
        }

        // todo
        //if (!c) {
        //    UsdGeomPointInstancer schema(cprim);
        //    if (schema) {
        //    }
        //}

        // xform must be last because Mesh is also Xformable
        if (!c) {
            UsdGeomXformable schema(cprim);
            if (schema) {
                c = new USDXformNode(n, cprim, true);
            }
        }
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
    s_current_scene = this;
    double time = m_scene->time_current;

    for (auto& n : m_nodes)
        n->read(time);
    ++m_read_count;
}

void USDScene::write()
{
    s_current_scene = const_cast<USDScene*>(this);
    double time = m_scene->time_current;

    if (m_write_count == 0) {
        for (auto& n : m_nodes)
            n->beforeWrite();
    }
    for (auto& n : m_nodes)
        n->write(time);
    ++m_write_count;

    if (!std::isnan(time) && time > m_max_time) {
        m_stage->SetEndTimeCode(time);
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
        m_node_table[path] = ret;
        return ret;
    }
    return nullptr;
}

Node* USDScene::createNode(Node* parent, const char* name, Node::Type type)
{
    s_current_scene = this;

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
        Case(Mesh, USDMeshNode);
        Case(Blendshape, USDBlendshapeNode);
        Case(SkelRoot, USDSkelRootNode);
        Case(Skeleton, USDSkeletonNode);
        Case(Material, USDMaterialNode);
        Case(Xform, USDXformNode);
        Case(Unknown, USDNode);
#undef Case
    default: break;
    }

    if (ret) {
        m_nodes.push_back(USDNodePtr(ret));
    }
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
        Case(Root, USDRootNode);
        Case(Mesh, USDMeshNode);
        Case(Blendshape, USDBlendshapeNode);
        Case(SkelRoot, USDSkelRootNode);
        Case(Skeleton, USDSkeletonNode);
        Case(Material, USDMaterialNode);
        Case(Xform, USDXformNode);
        Case(Unknown, USDNode);
#undef Case
    default: break;
    }

    if (ret) {
        m_nodes.push_back(USDNodePtr(ret));
    }
    return ret;
}

USDNode* USDScene::findNode(const std::string& path)
{
    auto it = m_node_table.find(path);
    return it == m_node_table.end() ? nullptr : it->second;
}

ScenePtr CreateUSDSceneInternal()
{
    auto ret = new Scene();
    ret->impl.reset(new mqusd::USDScene(ret));
    return ScenePtr(ret);
}

} // namespace mqusd

mqusdCoreAPI mqusd::SceneInterface* mqusdCreateUSDSceneInterface(mqusd::Scene *scene)
{
    return new mqusd::USDScene(scene);
}
