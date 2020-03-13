#include "pch.h"
#include "mqusd.h"
#include "mqusdNode.h"
#include "mqusdNodeInternal.h"
#include "Player/mqusdPlayerPlugin.h"

Node::Node(Node* p)
    : parent(p)
{
    if (parent)
        parent->children.push_back(this);
}

Node::~Node()
{
}

void Node::release()
{
    delete this;
}

Node::Type Node::getType() const
{
    return Type::Unknown;
}

void Node::update(double si)
{
    for (auto child : children)
        child->update(si);
}

UsdPrim* Node::getPrim()
{
    return nullptr;
}

template<class NodeT>
NodeT* Node::findParent()
{
    for (Node* p = parent; p != nullptr; p = p->parent) {
        if (p->getType() == NodeT::node_type)
            return (NodeT*)p;
    }
    return nullptr;
}

Node_::Node_(Node* p, UsdPrim usd)
    : super(p, usd)
{
}


RootNode::RootNode(Node* p)
    : super(nullptr)
{
}

Node::Type RootNode::getType() const
{
    return Type::Root;
}

RootNode_::RootNode_(UsdPrim usd)
    : super(nullptr, usd)
{
}


XformNode::XformNode(Node* p)
    : super(p)
{
    parent_xform = findParent<XformNode>();
}

Node::Type XformNode::getType() const
{
    return Type::Xform;
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



MeshNode::MeshNode(Node* p)
    : super(p)
{
    parent_xform = findParent<XformNode>();
}

Node::Type MeshNode::getType() const
{
    return Type::PolyMesh;
}

void MeshNode::convert(const mqusdPlayerSettings& settings)
{
    if (parent_xform)
        mesh.applyTransform(parent_xform->global_matrix);
    mesh.applyScale(settings.scale_factor);
    if (settings.flip_x)
        mesh.flipX();
    if (settings.flip_yz)
        mesh.flipYZ();
    if (settings.flip_faces)
        mesh.flipFaces();
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

    VtArray<int> counts;
    VtArray<int> indices;
    VtArray<int> mids;
    VtArray<GfVec3f> points;
    VtArray<GfVec3f> normals;
    VtArray<GfVec2f> uvs;

    auto t = UsdTimeCode(si);

    schema.GetFaceVertexCountsAttr().Get(&counts, t);
    mesh.counts.assign(counts.cdata(), counts.size());

    schema.GetFaceVertexIndicesAttr().Get(&indices, t);
    mesh.indices.assign(indices.cdata(), indices.size());

    schema.GetPointsAttr().Get(&points, t);
    mesh.points.assign((float3*)points.cdata(), points.size());

    schema.GetNormalsAttr().Get(&normals, t);
    mesh.normals.assign((float3*)normals.cdata(), normals.size());

    if (attr_uvs) {
        attr_uvs.Get(&uvs, t);
        mesh.uvs.assign((float2*)uvs.cdata(), uvs.size());
    }
    if (attr_mids) {
        attr_mids.Get(&mids, t);
        mesh.material_ids.assign(mids.cdata(), mids.size());
    }

    // validate
    mesh.clearInvalidComponent();
}



MaterialNode::MaterialNode(Node* p)
    : super(p)
{
}

Node::Type MaterialNode::getType() const
{
    return Type::Material;
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

bool MaterialNode::valid() const
{
    return false;
}

