#include "pch.h"
#include "mqusd.h"
#include "mqusdSceneGraphImpl.h"


template<class NodeT>
USDBaseNode<NodeT>::USDBaseNode(Node* p, UsdPrim usd)
    : super(p)
    , prim(usd)
{
    this->name = usd.GetName();
}

template<class NodeT>
UsdPrim* USDBaseNode<NodeT>::getPrim()
{
    return &prim;
}


template<class NodeT, class SchemaT>
USDXformableNode<NodeT, SchemaT>::USDXformableNode(Node* p, UsdPrim usd)
    : super(p, usd)
{
    schema = SchemaT(usd);
}

template<class NodeT, class SchemaT>
void USDXformableNode<NodeT, SchemaT>::readXform(const UsdTimeCode& t)
{
    GfMatrix4d mat;
    bool reset_stack = false;
    schema.GetLocalTransformation(&mat, &reset_stack, t);
    this->local_matrix.assign((double4x4&)mat);

    if (this->parent_xform)
        this->global_matrix = this->local_matrix * this->parent_xform->global_matrix;
    else
        this->global_matrix = this->local_matrix;
}

template<class NodeT, class SchemaT>
void USDXformableNode<NodeT, SchemaT>::writeXform(const UsdTimeCode& t) const
{
    if (xf_ops.empty()) {
        xf_ops.push_back(schema.AddTransformOp());
    }

    auto& op = xf_ops[0];
    if (op.GetOpType() == UsdGeomXformOp::TypeTransform) {
        double4x4 data;
        data.assign(this->local_matrix);
        op.Set((const GfMatrix4d&)data, t);
    }
}


USDNode::USDNode(Node* p, UsdPrim usd)
    : super(p, usd)
{
}


USDRootNode::USDRootNode(UsdPrim usd)
    : super(nullptr, usd)
{
}

USDXformNode::USDXformNode(Node* p, UsdPrim usd)
    : super(p, usd)
{
    schema = UsdGeomXformable(usd);
}

void USDXformNode::read(double time)
{
    readXform(time);
}

void USDXformNode::write(double time) const
{
    writeXform(time);
}


USDMeshNode::USDMeshNode(Node* p, UsdPrim usd)
    : super(p, usd)
{
    schema = UsdGeomMesh(usd);
    attr_uvs = prim.GetAttribute(TfToken(mqusdUVAttr));
    attr_mids = prim.GetAttribute(TfToken(mqusdMaterialIDAttr));
}

void USDMeshNode::read(double time)
{
    mesh.clear();
    if (!schema)
        return;

    auto t = UsdTimeCode(time);
    readXform(time);
    {
        VtArray<int> data;
        schema.GetFaceVertexCountsAttr().Get(&data, t);
        mesh.counts.assign(data.cdata(), data.size());
    }
    {
        VtArray<int> data;
        schema.GetFaceVertexIndicesAttr().Get(&data, t);
        mesh.indices.assign(data.cdata(), data.size());
    }
    {
        VtArray<GfVec3f> data;
        schema.GetPointsAttr().Get(&data, t);
        mesh.points.assign((float3*)data.cdata(), data.size());
    }
    {
        VtArray<GfVec3f> data;
        schema.GetNormalsAttr().Get(&data, t);
        mesh.normals.assign((float3*)data.cdata(), data.size());
    }
    if (attr_uvs) {
        VtArray<GfVec2f> data;
        attr_uvs.Get(&data, t);
        mesh.uvs.assign((float2*)data.cdata(), data.size());
    }
    if (attr_mids) {
        VtArray<int> data;
        attr_mids.Get(&data, t);
        mesh.material_ids.assign(data.cdata(), data.size());
    }

    // validate
    mesh.clearInvalidComponent();
}

void USDMeshNode::write(double time) const
{
    auto t = UsdTimeCode(time);
    writeXform(time);
    {
        VtArray<int> data;
        data.assign(mesh.counts.begin(), mesh.counts.end());
        schema.GetFaceVertexCountsAttr().Set(data, t);
    }
    {
        VtArray<int> data;
        data.assign(mesh.indices.begin(), mesh.indices.end());
        schema.GetFaceVertexIndicesAttr().Set(data, t);
    }
    {
        VtArray<GfVec3f> data;
        data.assign((GfVec3f*)mesh.points.begin(), (GfVec3f*)mesh.points.end());
        schema.GetPointsAttr().Set(data, t);
    }
    if (!mesh.normals.empty()) {
        VtArray<GfVec3f> data;
        data.assign((GfVec3f*)mesh.normals.begin(), (GfVec3f*)mesh.normals.end());
        schema.GetNormalsAttr().Set(data, t);
    }
    if (attr_uvs && !mesh.uvs.empty()) {
        VtArray<GfVec2f> data;
        data.assign((GfVec2f*)mesh.uvs.begin(), (GfVec2f*)mesh.uvs.end());
        attr_uvs.Set(data, t);
    }
    if (attr_mids && !mesh.material_ids.empty()) {
        VtArray<int> data;
        data.assign(mesh.material_ids.begin(), mesh.material_ids.end());
        attr_mids.Set(data, t);
    }
}


USDMaterialNode::USDMaterialNode(Node* p, UsdPrim usd)
    : super(p, usd)
{
    // todo
}

void USDMaterialNode::read(double si)
{
    // nothing to do for now
}

bool USDMaterialNode::valid() const
{
    // todo
    return true;
}


USDScene::USDScene()
{
}

USDScene::~USDScene()
{
    close();
}

bool USDScene::open(const char* path_)
{
    path = path_;
    m_stage = UsdStage::Open(path);
    if (!m_stage)
        return false;

    time_start = m_stage->GetStartTimeCode();
    time_end = m_stage->GetEndTimeCode();

    //    auto masters = m_stage->GetMasters();
    //    for (auto& m : masters) {
    //        m_masters.push_back(createSchemaRecursive(nullptr, m));
    //}
    auto root = m_stage->GetPseudoRoot();
    if (root.IsValid()) {
        root_node = new USDRootNode(root);
        constructTree(root_node);
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

    path = path_;
    m_stage = UsdStage::CreateNew(path);
    if (m_stage) {
        auto root = m_stage->GetPseudoRoot();
        if (root.IsValid()) {
            root_node = new USDRootNode(root);
            constructTree(root_node);
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

void USDScene::constructTree(Node* n)
{
    nodes.push_back(NodePtr(n));

    auto& prim = *n->getPrim();
    auto children = prim.GetChildren();
    for (auto cprim : children) {
        Node* c = nullptr;

        if (!c) {
            UsdGeomMesh mesh(cprim);
            if (mesh) {
                auto mn = new USDMeshNode(n, cprim);
                mesh_nodes.push_back(mn);
                c = mn;
            }
        }

        // todo
        //if (!c) {
        //    UsdGeomPointInstancer instancer(cprim);
        //    if (instancer) {
        //        auto mn = new MeshNode_(n, cprim);
        //        mesh_nodes.push_back(mn);
        //        c = mn;
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
    super::close();
}

void USDScene::read(double time)
{
    super::read(time);
}

void USDScene::write(double time) const
{
    super::write(time);
}

template<class NodeT>
Node* USDScene::createNodeImpl(Node* parent, std::string path)
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

    Node* ret = nullptr;
    switch (type) {
    case Node::Type::Xform:
        ret = createNodeImpl<USDXformNode>(parent, path);
        break;
    case Node::Type::Mesh:
        ret = createNodeImpl<USDMeshNode>(parent, path);
        mesh_nodes.push_back((USDMeshNode*)ret);
        break;
    default: break;
    }
    if (ret)
        nodes.push_back(NodePtr(ret));
    return ret;
}


#ifdef _WIN32
    #define mqusdCoreAPI extern "C" __declspec(dllexport)
#else
    #define mqusdCoreAPI extern "C" 
#endif

mqusdCoreAPI Scene* mqusdCreateScene()
{
    return new USDScene();
}
