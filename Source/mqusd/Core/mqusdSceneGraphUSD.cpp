#include "pch.h"
#include "mqusd.h"
#include "mqusdSceneGraphUSD.h"

namespace mqusd {

USDNode::USDNode(USDNode* parent, UsdPrim prim, bool create_node)
    : m_prim(prim)
    , m_parent(parent)
{
    if (m_parent)
        m_parent->m_children.push_back(this);
    if (create_node)
        setNode(new Node(parent ? parent->m_node : nullptr));
}

USDNode::~USDNode()
{
}

void USDNode::read(double time)
{
}

void USDNode::write(double time) const
{

}

void USDNode::setNode(Node* node)
{
    m_node = node;
    m_node->impl = this;
    m_node->name = m_prim.GetPath().GetName();
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

USDXformNode::USDXformNode(USDNode* parent, UsdPrim prim, bool create_node)
    : super(parent, prim, false)
{
    m_xform = UsdGeomXformable(prim);
    if (create_node)
        setNode(new XformNode(parent ? parent->m_node : nullptr));
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

void USDXformNode::write(double time) const
{
    super::write(time);

    auto t = UsdTimeCode(time);
    auto& src = static_cast<XformNode&>(*m_node);

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


USDMeshNode::USDMeshNode(USDNode* parent, UsdPrim prim)
    : super(parent, prim, false)
{
    m_mesh = UsdGeomMesh(prim);

    m_attr_uv = prim.GetAttribute(TfToken(mqusdAttrUV));
    m_attr_uv_indices = prim.GetAttribute(TfToken(mqusdAttrUVIndices));
    m_attr_mids = prim.GetAttribute(TfToken(mqusdAttrMaterialIDs));

    m_attr_joints = prim.GetAttribute(TfToken(mqusdAttrJoints));
    m_attr_joint_indices = prim.GetAttribute(TfToken(mqusdAttrJointIndices));
    m_attr_joint_weights = prim.GetAttribute(TfToken(mqusdAttrJointWeights));
    m_attr_bind_transform = prim.GetAttribute(TfToken(mqusdAttrBindTransform));

    setNode(new MeshNode(parent ? parent->m_node : nullptr));

#ifdef mqusdDebug
    for (auto& attr : m_prim.GetAuthoredAttributes()) {
        mqusdDbgPrint("  %s (%s)\n", attr.GetName().GetText(), attr.GetTypeName().GetAsToken().GetText());
    }
#endif
}

void USDMeshNode::read(double time)
{
    super::read(time);

    auto t = UsdTimeCode(time);
    auto& dst = *static_cast<MeshNode&>(*m_node).mesh;
    dst.clear();
    {
        VtArray<int> data;
        m_mesh.GetFaceVertexCountsAttr().Get(&data, t);
        dst.counts.assign(data.cdata(), data.size());
    }
    {
        VtArray<int> data;
        m_mesh.GetFaceVertexIndicesAttr().Get(&data, t);
        dst.indices.assign(data.cdata(), data.size());
    }
    {
        VtArray<GfVec3f> data;
        m_mesh.GetPointsAttr().Get(&data, t);
        dst.points.assign((float3*)data.cdata(), data.size());
    }
    {
        VtArray<GfVec3f> data;
        m_mesh.GetNormalsAttr().Get(&data, t);
        dst.normals.assign((float3*)data.cdata(), data.size());
    }
    if (m_attr_uv) {
        VtArray<GfVec2f> data;
        m_attr_uv.Get(&data, t);

        if (m_attr_uv_indices) {
            VtArray<int> indices;
            m_attr_uv_indices.Get(&indices, t);
            dst.uvs.resize_discard(indices.size());
            mu::CopyWithIndices(dst.uvs.data(), (float2*)data.cdata(), indices);
        }
        else {
            dst.uvs.assign((float2*)data.cdata(), data.size());
        }
    }
    if (m_attr_mids) {
        VtArray<int> data;
        m_attr_mids.Get(&data, t);
        dst.material_ids.assign(data.cdata(), data.size());
    }

    // skinning
    if (m_attr_joints) {
        VtArray<TfToken> data;
        m_attr_joints.Get(&data, t);
        for (auto& t : data)
            dst.joints.push_back(t.GetString());
    }
    if (m_attr_joint_indices) {
        m_attr_joint_indices.GetMetadata(mqusdKeyElementSize, &dst.joints_per_vertex);

        VtArray<int> data;
        m_attr_joint_indices.Get(&data, t);
        dst.joint_indices.assign(data.cdata(), data.size());
    }
    if (m_attr_joint_weights) {
        VtArray<float> data;
        m_attr_joint_weights.Get(&data, t);
        dst.joint_weights.assign(data.cdata(), data.size());
    }
    if (m_attr_bind_transform) {
        GfMatrix4d data;
        m_attr_bind_transform.Get(&data, t);
        dst.bind_transform.assign((float4x4&)data);
    }

    // validate
    dst.clearInvalidComponent();
}

void USDMeshNode::write(double time) const
{
    super::write(time);

    auto t = UsdTimeCode(time);
    auto& src = *static_cast<MeshNode&>(*m_node).mesh;
    {
        VtArray<int> data;
        data.assign(src.counts.begin(), src.counts.end());
        m_mesh.GetFaceVertexCountsAttr().Set(data, t);
    }
    {
        VtArray<int> data;
        data.assign(src.indices.begin(), src.indices.end());
        m_mesh.GetFaceVertexIndicesAttr().Set(data, t);
    }
    {
        VtArray<GfVec3f> data;
        data.assign((GfVec3f*)src.points.begin(), (GfVec3f*)src.points.end());
        m_mesh.GetPointsAttr().Set(data, t);
    }
    if (!src.normals.empty()) {
        VtArray<GfVec3f> data;
        data.assign((GfVec3f*)src.normals.begin(), (GfVec3f*)src.normals.end());
        m_mesh.GetNormalsAttr().Set(data, t);
    }
    if (m_attr_uv && !src.uvs.empty()) {
        VtArray<GfVec2f> data;
        data.assign((GfVec2f*)src.uvs.begin(), (GfVec2f*)src.uvs.end());
        m_attr_uv.Set(data, t);
    }
    if (m_attr_mids && !src.material_ids.empty()) {
        VtArray<int> data;
        data.assign(src.material_ids.begin(), src.material_ids.end());
        m_attr_mids.Set(data, t);
    }
}



USDBlendshapeNode::USDBlendshapeNode(USDNode* parent, UsdPrim prim)
    : super(parent, prim, false)
{
    m_blendshape = UsdSkelBlendShape(prim);

    setNode(new BlendshapeNode(parent ? parent->m_node : nullptr));
    if (parent && parent->m_node->getType() == Node::Type::Mesh) {
        static_cast<MeshNode*>(parent->m_node)->mesh->blendshapes.push_back(
            static_cast<BlendshapeNode*>(m_node)->blendshape);
    }
}

void USDBlendshapeNode::read(double time)
{
    super::read(time);

    auto t = UsdTimeCode(time);
    auto& dst = *static_cast<BlendshapeNode*>(m_node)->blendshape;
    dst.clear();

    {
        VtArray<int> data;
        m_blendshape.GetPointIndicesAttr().Get(&data, t);
        dst.indices.assign(data.cdata(), data.size());
    }
    {
        VtArray<GfVec3f> data;
        m_blendshape.GetOffsetsAttr().Get(&data, t);
        dst.point_offsets.assign((float3*)data.cdata(), data.size());
    }
    {
        VtArray<GfVec3f> data;
        m_blendshape.GetNormalOffsetsAttr().Get(&data, t);
        dst.normal_offsets.assign((float3*)data.cdata(), data.size());
    }
}

void USDBlendshapeNode::write(double time) const
{
    // todo
}



USDSkeletonNode::USDSkeletonNode(USDNode* parent, UsdPrim prim)
    : super(parent, prim, false)
{
    m_skel = UsdSkelSkeleton(prim);

    setNode(new SkeletonNode(parent ? parent->m_node : nullptr));
}

void USDSkeletonNode::read(double time)
{
    super::read(time);

    auto t = UsdTimeCode(time);
    auto& dst = *static_cast<SkeletonNode*>(m_node)->skeleton;
    dst.clear();

    {
        VtArray<TfToken> data;
        m_skel.GetJointsAttr().Get(&data, t);
        for (auto& token: data)
            dst.addJoint(token.GetString());
        dst.buildJointRelations();
    }
    {
        VtArray<GfMatrix4d> data;
        m_skel.GetBindTransformsAttr().Get(&data, t);
        size_t n = data.size();
        if (dst.joints.size() == n) {
            for (size_t i = 0; i < n; ++i)
                dst.joints[i]->bindpose.assign((double4x4&)data[i]);
        }
    }
    {
        VtArray<GfMatrix4d> data;
        m_skel.GetRestTransformsAttr().Get(&data, t);
        size_t n = data.size();
        if (dst.joints.size() == n) {
            for (size_t i = 0; i < n; ++i)
                dst.joints[i]->restpose.assign((double4x4&)data[i]);
        }
    }
}

void USDSkeletonNode::write(double time) const
{
    super::write(time);

    auto t = UsdTimeCode(time);
    // todo
}


USDMaterialNode::USDMaterialNode(USDNode* parent, UsdPrim prim)
    : super(parent, prim, false)
{
    m_material = UsdShadeMaterial(prim);

    setNode(new MaterialNode(parent ? parent->m_node : nullptr));
}

void USDMaterialNode::read(double time)
{
    super::read(time);
    // todo
}

void USDMaterialNode::write(double time) const
{
    super::write(time);
    // todo
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
    mqusdDbgPrint("%s (%s)\n", n->getPath().c_str(), n->m_prim.GetTypeName().GetText());
#endif

    m_nodes.push_back(USDNodePtr(n));
    m_scene->nodes.push_back(NodePtr(n->m_node));

    auto& prim = n->m_prim;
    auto children = prim.GetChildren();
    for (auto cprim : children) {
        USDNode* c = nullptr;

        if (!c) {
            UsdGeomMesh schema(cprim);
            if (schema) {
                c = new USDMeshNode(n, cprim);
                m_scene->mesh_nodes.push_back(static_cast<MeshNode*>(c->m_node));
            }
        }
        if (!c) {
            UsdSkelBlendShape schema(cprim);
            if (schema)
                c = new USDBlendshapeNode(n, cprim);
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
                c = new USDXformNode(n, cprim);
            }
        }
        if (!c) {
            c = new USDNode(n, cprim);
        }

        constructTree(c);
    }
}

void USDScene::close()
{
    m_stage = {};
}

void USDScene::read(double time)
{
    for (auto& n : m_nodes)
        n->read(time);
}

void USDScene::write(double time) const
{
    for (auto& n : m_nodes)
        n->write(time);
    m_stage->SetEndTimeCode(time);
}

template<class NodeT>
USDNode* USDScene::createNodeImpl(USDNode* parent, std::string path)
{
    auto prim = m_stage->DefinePrim(SdfPath(path), TfToken(NodeT::getUsdTypeName()));
    if (prim)
        return new NodeT(parent, prim);
    return nullptr;
}

Node* USDScene::createNode(Node* parent, const char* name, Node::Type type)
{
    std::string path;
    {
        // sanitize
        std::string n = name;
        for (auto& c : n) {
            if (!std::isalnum(c))
                c = '_';
        }
        if (parent)
            path = parent->getPath();
        path += n;
    }

    USDNode* ret = nullptr;
    USDNode* usd_parent = parent ? (USDNode*)parent->impl : nullptr;
    switch (type) {
#define Case(E, T) case Node::Type::E: ret = createNodeImpl<T>(usd_parent, path); break;
        Case(Mesh, USDMeshNode);
        Case(Blendshape, USDBlendshapeNode);
        Case(Skeleton, USDSkeletonNode);
        Case(Material, USDMaterialNode);
        Case(Xform, USDXformNode);
#undef Case
    default: break;
    }

    if (ret) {
        m_nodes.push_back(USDNodePtr(ret));
        m_scene->nodes.push_back(NodePtr(ret->m_node));
        if (type == Node::Type::Mesh)
            m_scene->mesh_nodes.push_back(static_cast<MeshNode*>(ret->m_node));
    }
    return ret ? ret->m_node : nullptr;
}

} // namespace mqusd

#ifdef _WIN32
    #define mqusdCoreAPI extern "C" __declspec(dllexport)
#else
    #define mqusdCoreAPI extern "C" 
#endif

mqusdCoreAPI mqusd::SceneInterface* mqusdCreateUSDSceneInterface(mqusd::Scene *scene)
{
    return new mqusd::USDScene(scene);
}
