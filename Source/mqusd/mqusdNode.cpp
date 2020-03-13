#include "pch.h"
#include "mqusd.h"
#include "mqusdNode.h"
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


RootNode::RootNode(Node* p)
    : super(nullptr)
{
}

Node::Type RootNode::getType() const
{
    return Type::Root;
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



MaterialNode::MaterialNode(Node* p)
    : super(p)
{
}

Node::Type MaterialNode::getType() const
{
    return Type::Material;
}

bool MaterialNode::valid() const
{
    return true;
}



Scene::Scene()
{
}

Scene::~Scene()
{
}

void Scene::update(double t)
{
}
