#include "pch.h"
#include "mqusd.h"
#include "mqusdSceneGraph.h"
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

void Node::seek(double si)
{
}

void Node::write()
{
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

void Scene::release()
{
    delete this;
}

bool Scene::open(const char* path)
{
    return false;
}

void Scene::close()
{
    path.clear();
    root_node = nullptr;
    mesh_nodes.clear();
    material_nodes.clear();
    nodes.clear();
}

void Scene::seek(double t)
{
    for (auto& n : nodes)
        n->seek(t);
}

void Scene::write()
{
    for (auto& n : nodes)
        n->write();
}


static void* g_core_module;
static Scene* (*g_mqusdCreateScene)();

static void LoadCoreModule()
{
    if (g_core_module)
        return;

    std::string path = mu::GetCurrentModuleDirectory();
    path += "/" "mqusdCore" muDLLSuffix;
    g_core_module = mu::LoadModule(path.c_str());
    if (g_core_module) {
        (void*&)g_mqusdCreateScene = mu::GetSymbol(g_core_module, "mqusdCreateScene");
    }
}

ScenePtr CreateScene()
{
    LoadCoreModule();
    if (g_mqusdCreateScene)
        return ScenePtr(g_mqusdCreateScene());
    return nullptr;
}
