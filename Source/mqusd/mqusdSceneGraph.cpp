#include "pch.h"
#include "mqusd.h"
#include "mqusdSceneGraph.h"
#include "Player/mqusdPlayerPlugin.h"

namespace mqusd {

Node::Node(Node* p)
    : parent(p)
{
    if (parent)
        parent->children.push_back(this);
}

Node::~Node()
{
}

Node::Type Node::getType() const
{
    return Type::Unknown;
}

void Node::convert(const ConvertOptions& opt)
{
}

std::string Node::getPath() const
{
    std::string ret;
    if (parent)
        ret += parent->getPath();
    if (ret.empty() || ret.back() != '/')
        ret += "/";
    if (name != "/")
        ret += name;
    return ret;
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


RootNode::RootNode()
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

void XformNode::convert(const ConvertOptions& opt)
{
    super::convert(opt);

    if (opt.scale_factor != 1.0f) {
        (float3&)local_matrix[3] *= opt.scale_factor;
        (float3&)global_matrix[3] *= opt.scale_factor;
    }
    if (opt.flip_x) {
        local_matrix = flip_x(local_matrix);
        global_matrix = flip_x(global_matrix);
    }
    if (opt.flip_yz) {
        local_matrix = flip_z(swap_yz(local_matrix));
        global_matrix = flip_z(swap_yz(global_matrix));
    }
}



MeshNode::MeshNode(Node* p)
    : super(p)
{
    mesh = std::make_shared<Mesh>();
    mesh->name = name;
}

Node::Type MeshNode::getType() const
{
    return Type::Mesh;
}

void MeshNode::convert(const ConvertOptions& opt)
{
    super::convert(opt);

    mesh->applyScale(opt.scale_factor);
    if (opt.flip_x)
        mesh->flipX();
    if (opt.flip_yz)
        mesh->flipYZ();
    if (opt.flip_faces)
        mesh->flipFaces();
}

void MeshNode::toWorldSpace()
{
    mesh->applyTransform(global_matrix);
}
void MeshNode::toLocalSpace()
{
    mesh->applyTransform(invert(global_matrix));
}


BlendshapeNode::BlendshapeNode(Node* p)
    : super(p)
{
    blendshape = std::make_shared<Blendshape>();
    blendshape->name = name;
}

Node::Type BlendshapeNode::getType() const
{
    return Type::Blendshape;
}

void BlendshapeNode::convert(const ConvertOptions& opt)
{
    super::convert(opt);
    // handled by Mesh
}


SkeletonNode::SkeletonNode(Node* parent)
    : super(parent)
{
    skeleton = std::make_shared<Skeleton>();
    skeleton->name = name;
}

Node::Type SkeletonNode::getType() const
{
    return Type::Skeleton;
}

void SkeletonNode::convert(const ConvertOptions& opt)
{
    super::convert(opt);

    skeleton->applyScale(opt.scale_factor);
    if (opt.flip_x)
        skeleton->flipX();
    if (opt.flip_yz)
        skeleton->flipYZ();
}



MaterialNode::MaterialNode(Node* p)
    : super(p)
{
    material = std::make_shared<Material>();
    material->name = name;
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
    close();
}

bool Scene::open(const char* path)
{
    if (impl)
        return impl->open(path);
    return false;
}

bool Scene::create(const char* path)
{
    if (impl)
        return impl->create(path);
    return false;
}

bool Scene::save()
{
    if (impl)
        return impl->save();
    return false;
}

void Scene::close()
{
    if (impl)
        impl->close();

    path.clear();
    root_node = nullptr;
    mesh_nodes.clear();
    material_nodes.clear();
    nodes.clear();
}

void Scene::read(double time)
{
    if (impl)
        impl->read(time);
}

void Scene::write(double time) const
{
    if (impl)
        impl->write(time);
}

Node* Scene::createNode(Node* parent, const char* name, Node::Type type)
{
    if (impl)
        return impl->createNode(parent, name, type);
    return nullptr;
}

SceneInterface::~SceneInterface()
{
}


#define mqusdTBBDll muDLLPrefix "tbb" muDLLSuffix
#define mqusdUSDDll muDLLPrefix "usd_ms" muDLLSuffix
#define mqusdCoreDll muDLLPrefix "mqusdCore" muDLLSuffix

static void* g_core_module;
static SceneInterface* (*g_mqusdCreateUSDSceneInterface)(Scene* scene);

static void LoadCoreModule()
{
    static std::once_flag s_flag;
    std::call_once(s_flag, []() {
        g_core_module = mu::GetModule(mqusdCoreDll);
        if (!g_core_module) {
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
        }
        if (g_core_module) {
            (void*&)g_mqusdCreateUSDSceneInterface = mu::GetSymbol(g_core_module, "mqusdCreateUSDSceneInterface");
        }
    });
}

ScenePtr CreateUSDScene()
{
    LoadCoreModule();
    if (g_mqusdCreateUSDSceneInterface) {
        auto ret = new Scene();
        ret->impl.reset(g_mqusdCreateUSDSceneInterface(ret));
        return ScenePtr(ret);
    }
    else
        return nullptr;
}

} // namespace mqusd
