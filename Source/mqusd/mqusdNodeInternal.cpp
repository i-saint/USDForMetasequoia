#include "pch.h"
#include "mqusd.h"
#include "mqusdNodeInternal.h"

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

void XformNode_::update(double si)
{
    if (schema) {
        // todo

        if (parent_xform)
            global_matrix = local_matrix * parent_xform->global_matrix;
        else
            global_matrix = local_matrix;
    }

    super::update(si);
}


MeshNode_::MeshNode_(Node* p, UsdPrim usd)
    : super(p, usd)
{
    schema = UsdGeomMesh(usd);
    attr_uvs = prim.GetAttribute(TfToken(mqusdUVAttr));
    attr_mids = prim.GetAttribute(TfToken(mqusdMaterialIDAttr));
}

void MeshNode_::update(double si)
{
    updateMeshData(si);
    super::update(si);
}

void MeshNode_::updateMeshData(double si)
{
    mesh.clear();
    if (!schema)
        return;

    auto t = UsdTimeCode(si);
    {
        VtArray<int> counts;
        schema.GetFaceVertexCountsAttr().Get(&counts, t);
        mesh.counts.assign(counts.cdata(), counts.size());
    }
    {
        VtArray<int> indices;
        schema.GetFaceVertexIndicesAttr().Get(&indices, t);
        mesh.indices.assign(indices.cdata(), indices.size());
    }
    {
        VtArray<GfVec3f> points;
        schema.GetPointsAttr().Get(&points, t);
        mesh.points.assign((float3*)points.cdata(), points.size());
    }
    {
        VtArray<GfVec3f> normals;
        schema.GetNormalsAttr().Get(&normals, t);
        mesh.normals.assign((float3*)normals.cdata(), normals.size());
    }
    if (attr_uvs) {
        VtArray<GfVec2f> uvs;
        attr_uvs.Get(&uvs, t);
        mesh.uvs.assign((float2*)uvs.cdata(), uvs.size());
    }
    if (attr_mids) {
        VtArray<int> mids;
        attr_mids.Get(&mids, t);
        mesh.material_ids.assign(mids.cdata(), mids.size());
    }

    // validate
    mesh.clearInvalidComponent();
}


MaterialNode_::MaterialNode_(Node* p, UsdPrim usd)
    : super(p, usd)
{
    // todo
}

void MaterialNode_::update(double si)
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
}

bool Scene_::open(const char* p)
{
    m_stage = UsdStage::Open(path);
    if (!m_stage)
        return false;
    path = p;

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

void Scene_::close()
{
    path.clear();
    root_node = nullptr;
    mesh_nodes.clear();
    material_nodes.clear();
    nodes.clear();

    m_stage = {};
}

void Scene_::update(double t)
{
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
