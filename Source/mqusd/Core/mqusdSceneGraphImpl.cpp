#include "pch.h"
#include "mqusd.h"
#include "mqusdSceneGraphImpl.h"

Node_::Node_(Node* p, UsdPrim usd)
    : super(p, usd)
{
}


RootNode_::RootNode_(UsdPrim usd)
    : super(nullptr, usd)
{
}


XformNode_::XformNode_(Node* p, UsdPrim usd)
    : super(p, usd)
{
    schema = UsdGeomXformable(usd);
}

void XformNode_::seek(double si)
{
    if (schema) {
        // todo

        if (parent_xform)
            global_matrix = local_matrix * parent_xform->global_matrix;
        else
            global_matrix = local_matrix;
    }
}


MeshNode_::MeshNode_(Node* p, UsdPrim usd)
    : super(p, usd)
{
    schema = UsdGeomMesh(usd);
    attr_uvs = prim.GetAttribute(TfToken(mqusdUVAttr));
    attr_mids = prim.GetAttribute(TfToken(mqusdMaterialIDAttr));
}

void MeshNode_::seek(double si)
{
    mesh.clear();
    if (!schema)
        return;

    auto t = UsdTimeCode(si);
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


MaterialNode_::MaterialNode_(Node* p, UsdPrim usd)
    : super(p, usd)
{
    // todo
}

void MaterialNode_::seek(double si)
{
    // nothing to do for now
}

bool MaterialNode_::valid() const
{
    // todo
    return true;
}


Scene_::Scene_()
{
}

Scene_::~Scene_()
{
    close();
}

bool Scene_::open(const char* p)
{
    path = p;
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
        root_node = new RootNode_(root);
        constructTree(root_node);
    }
    return true;
}

void Scene_::constructTree(Node* n)
{
    nodes.push_back(NodePtr(n));

    auto& prim = *n->getPrim();
    auto children = prim.GetChildren();
    for (auto cprim : children) {
        Node* c = nullptr;

        if (!c) {
            UsdGeomMesh mesh(cprim);
            if (mesh) {
                auto mn = new MeshNode_(n, cprim);
                mesh_nodes.push_back(mn);
                c = mn;
            }
        }
        // xform must be last because Mesh is also Xformable
        if (!c) {
            UsdGeomXformable xform(cprim);
            if (xform) {
                c = new XformNode_(n, cprim);
            }
        }
        if (!c) {
            c = new Node_(n, cprim);
        }

        constructTree(c);
    }
}

void Scene_::close()
{
    m_stage = {};
    super::close();
}

void Scene_::seek(double t)
{
    super::seek(t);
}


#ifdef _WIN32
    #define mqusdCoreAPI extern "C" __declspec(dllexport)
#else
    #define mqusdCoreAPI extern "C" 
#endif

mqusdCoreAPI Scene* mqusdCreateScene()
{
    return new Scene_();
}
