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
}

Node::Type MeshNode::getType() const
{
    return Type::PolyMesh;
}

void MeshNode::convert(const mqusdPlayerSettings& settings)
{
    mesh.applyTransform(global_matrix);
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


#define mqusdTBBDll "tbb" muDLLSuffix
#define mqusdUSDDll "usd_ms" muDLLSuffix
#define mqusdCoreDll "mqusdCore" muDLLSuffix

static void* g_core_module;
static Scene* (*g_mqusdCreateScene)();

static void LoadCoreModule()
{
    static std::once_flag s_flag;
    std::call_once(s_flag, []() {
        std::string dir = mu::GetCurrentModuleDirectory();
#ifdef _WIN32
        std::string core_dir = dir + "/mqusdCore";
#else
        std::string core_dir = dir;
#endif

        std::string plugin_dir = core_dir + "/usd/plugInfo.json";
        mu::SetEnv("PXR_PLUGINPATH_NAME", plugin_dir.c_str());

        std::string tbb_dll = core_dir + "/" mqusdTBBDll;
        std::string usd_dll = core_dir + "/" mqusdUSDDll;
        std::string core_dll = core_dir + "/" mqusdCoreDll;
        mu::LoadModule(tbb_dll.c_str());
        mu::LoadModule(usd_dll.c_str());
        g_core_module = mu::LoadModule(core_dll.c_str());
        if (g_core_module) {
            (void*&)g_mqusdCreateScene = mu::GetSymbol(g_core_module, "mqusdCreateScene");
        }
    });
}

ScenePtr CreateScene()
{
    LoadCoreModule();
    if (g_mqusdCreateScene)
        return ScenePtr(g_mqusdCreateScene());
    else
        return nullptr;
}
