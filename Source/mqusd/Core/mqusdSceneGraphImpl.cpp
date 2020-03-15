#include "pch.h"
#include "mqusd.h"
#include "mqusdSceneGraphImpl.h"


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
    if (m_xf_ops.empty()) {
        m_xf_ops.push_back(m_xform.AddTransformOp());
    }

    auto& op = m_xf_ops[0];
    if (op.GetOpType() == UsdGeomXformOp::TypeTransform) {
        auto t = UsdTimeCode(time);
        auto& src = static_cast<XformNode&>(*m_node);

        double4x4 data;
        data.assign(src.local_matrix);
        op.Set((const GfMatrix4d&)data, t);
    }
}


USDMeshNode::USDMeshNode(USDNode* parent, UsdPrim prim)
    : super(parent, prim, false)
{
    m_mesh = UsdGeomMesh(prim);
    m_attr_uvs = prim.GetAttribute(TfToken(mqusdUVAttr));
    m_attr_mids = prim.GetAttribute(TfToken(mqusdMaterialIDAttr));

    setNode(new MeshNode(parent ? parent->m_node : nullptr));
}

void USDMeshNode::read(double time)
{
    super::read(time);

    auto t = UsdTimeCode(time);
    auto& dst = static_cast<MeshNode&>(*m_node).mesh;
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
    if (m_attr_uvs) {
        VtArray<GfVec2f> data;
        m_attr_uvs.Get(&data, t);
        dst.uvs.assign((float2*)data.cdata(), data.size());
    }
    if (m_attr_mids) {
        VtArray<int> data;
        m_attr_mids.Get(&data, t);
        dst.material_ids.assign(data.cdata(), data.size());
    }

    // validate
    dst.clearInvalidComponent();
}

void USDMeshNode::write(double time) const
{
    super::write(time);

    auto t = UsdTimeCode(time);
    auto& src = static_cast<MeshNode&>(*m_node).mesh;
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
    if (m_attr_uvs && !src.uvs.empty()) {
        VtArray<GfVec2f> data;
        data.assign((GfVec2f*)src.uvs.begin(), (GfVec2f*)src.uvs.end());
        m_attr_uvs.Set(data, t);
    }
    if (m_attr_mids && !src.material_ids.empty()) {
        VtArray<int> data;
        data.assign(src.material_ids.begin(), src.material_ids.end());
        m_attr_mids.Set(data, t);
    }
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
    m_stage = UsdStage::CreateNew(path_);
    if (m_stage) {
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

void USDScene::constructTree(USDNode* n)
{
    mqusdDbgPrint("%s\n", n->getPath().c_str());

    m_nodes.push_back(USDNodePtr(n));
    m_scene->nodes.push_back(NodePtr(n->m_node));

    auto& prim = n->m_prim;
    auto children = prim.GetChildren();
    for (auto cprim : children) {
        USDNode* c = nullptr;

        if (!c) {
            UsdGeomMesh mesh(cprim);
            if (mesh) {
                c = new USDMeshNode(n, cprim);
                m_scene->mesh_nodes.push_back(static_cast<MeshNode*>(c->m_node));
            }
        }

        // todo
        //if (!c) {
        //    UsdGeomPointInstancer instancer(cprim);
        //    if (instancer) {
        //    }
        //}

        // xform must be last because Mesh is also Xformable
        if (!c) {
            UsdGeomXformable xform(cprim);
            if (xform) {
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
    case Node::Type::Xform:
        ret = createNodeImpl<USDXformNode>(usd_parent, path);
        break;
    case Node::Type::Mesh:
        ret = createNodeImpl<USDMeshNode>(usd_parent, path);
        break;
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


#ifdef _WIN32
    #define mqusdCoreAPI extern "C" __declspec(dllexport)
#else
    #define mqusdCoreAPI extern "C" 
#endif

mqusdCoreAPI SceneInterface* mqusdCreateUSDSceneInterface(Scene *scene)
{
    return new USDScene(scene);
}
