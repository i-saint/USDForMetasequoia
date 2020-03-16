#include "pch.h"
#include "mqusd.h"
#include "mqusdSceneGraph.h"

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
        auto convert = [&opt](float4x4& m) { (float3&)m[3] *= opt.scale_factor; };
        convert(local_matrix);
        convert(global_matrix);
    }
    if (opt.flip_x) {
        auto convert = [](float4x4& m) { m = flip_x(m); };
        convert(local_matrix);
        convert(global_matrix);
    }
    if (opt.flip_yz) {
        auto convert = [](float4x4& m) { m = flip_z(swap_yz(m)); };
        convert(local_matrix);
        convert(global_matrix);
    }
}

std::tuple<float3, quatf, float3> XformNode::getLocalTRS() const
{
    return mu::extract_trs(local_matrix);
}
std::tuple<float3, quatf, float3> XformNode::getGlobalTRS() const
{
    return mu::extract_trs(global_matrix);
}

void XformNode::setLocalTRS(const float3& t, const quatf& r, const float3& s)
{
    local_matrix = mu::transform(t, r, s);
}
void XformNode::setGlobalTRS(const float3& t, const quatf& r, const float3& s)
{
    global_matrix = mu::transform(t, r, s);
}



MeshNode::MeshNode(Node* p)
    : super(p)
{
}

Node::Type MeshNode::getType() const
{
    return Type::Mesh;
}

void MeshNode::convert(const ConvertOptions& opt)
{
    super::convert(opt);

    if (opt.scale_factor != 1.0f) {
        mu::Scale(points.data(), opt.scale_factor, points.size());
        (float3&)bind_transform[3] *= opt.scale_factor;

        for (auto bs: blendshapes) {
            mu::Scale(bs->point_offsets.data(), opt.scale_factor, bs->point_offsets.size());
        }
    }

    if (opt.flip_x) {
        mu::InvertX(points.data(), points.size());
        mu::InvertX(normals.data(), normals.size());
        bind_transform = flip_x(bind_transform);

        for (auto bs : blendshapes) {
            mu::InvertX(bs->point_offsets.data(), bs->point_offsets.size());
            mu::InvertX(bs->normal_offsets.data(), bs->normal_offsets.size());
        }
    }

    if (opt.flip_yz) {
        auto convert = [this](auto& v) { return flip_z(swap_yz(v)); };
        for (auto& v : points) v = convert(v);
        for (auto& v : normals) v = convert(v);
        bind_transform = convert(bind_transform);

        for (auto bs : blendshapes) {
            for (auto& v : bs->point_offsets) v = convert(v);
            for (auto& v : bs->normal_offsets) v = convert(v);
        }
    }

    if (opt.flip_faces) {
        size_t nfaces = counts.size();
        int* idata = indices.data();
        int* cdata = counts.data();
        for (size_t fi = 0; fi < nfaces; ++fi) {
            int c = *cdata;
            std::reverse(idata, idata + c);
            idata += c;
            ++cdata;
        }
    }
}

void MeshNode::applyTransform(const float4x4& v)
{
    if (v != float4x4::identity()) {
        mu::MulPoints(v, points.cdata(), points.data(), points.size());
        mu::MulVectors(v, normals.cdata(), normals.data(), normals.size());
        bind_transform *= v;

        for (auto bs : blendshapes) {
            mu::MulVectors(v, bs->point_offsets.cdata(), bs->point_offsets.data(), bs->point_offsets.size());
            mu::MulVectors(v, bs->normal_offsets.cdata(), bs->normal_offsets.data(), bs->normal_offsets.size());
        }
    }
}
void MeshNode::toWorldSpace()
{
    applyTransform(global_matrix);
}
void MeshNode::toLocalSpace()
{
    applyTransform(invert(global_matrix));
}

void MeshNode::clear()
{
    points.clear();
    normals.clear();
    uvs.clear();
    colors.clear();
    material_ids.clear();
    counts.clear();
    indices.clear();

    blendshapes.clear();

    skeleton = nullptr;
    joints.clear();
    joints_per_vertex = 0;
    joint_indices.clear();
    joint_weights.clear();
    bind_transform = float4x4::identity();
}

void MeshNode::merge(const MeshNode& v)
{
    auto append = [](auto& dst, const auto& src) {
        dst.insert(dst.end(), src.cdata(), src.cdata() + src.size());
    };

    int vertex_offset = (int)points.size();
    int index_offset = (int)indices.size();

    append(points, v.points);
    append(normals, v.normals);
    append(uvs, v.uvs);
    append(colors, v.colors);
    append(material_ids, v.material_ids);
    append(counts, v.counts);
    append(indices, v.indices);

    if (vertex_offset > 0) {
        int index_end = index_offset + (int)v.indices.size();
        for (int ii = index_offset; ii < index_end; ++ii)
            indices[ii] += vertex_offset;
    }
}

void MeshNode::validate()
{
    auto nindices = indices.size();
    auto nfaces = counts.size();

    if (!normals.empty() && normals.size() != nindices)
        normals.clear();
    if (!uvs.empty() && uvs.size() != nindices)
        uvs.clear();
    if (!colors.empty() && colors.size() != nindices)
        colors.clear();
    if (!material_ids.empty() && material_ids.size() != nfaces)
        material_ids.clear();
}

int MeshNode::getMaxMaterialID() const
{
    int mid_min = 0;
    int mid_max = 0;
    mu::MinMax(material_ids.cdata(), material_ids.size(), mid_min, mid_max);
    return mid_max;
}


BlendshapeNode::BlendshapeNode(Node* p)
    : super(p)
{
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

void BlendshapeNode::clear()
{
    indices.clear();
    point_offsets.clear();
    normal_offsets.clear();
}



SkeletonNode::Joint::Joint(const std::string& p)
    : path(p)
{
    auto pos = path.find_last_of('/');
    if (pos == std::string::npos)
        name = path;
    else
        name = std::string(path.begin() + pos + 1, path.end());
}

std::tuple<float3, quatf, float3> SkeletonNode::Joint::getLocalTRS() const
{
    return mu::extract_trs(local_matrix);
}
std::tuple<float3, quatf, float3> SkeletonNode::Joint::getGlobalTRS() const
{
    return mu::extract_trs(global_matrix);
}

void SkeletonNode::Joint::setLocalTRS(const float3& t, const quatf& r, const float3& s)
{
    local_matrix = mu::transform(t, r, s);
}
void SkeletonNode::Joint::setGlobalTRS(const float3& t, const quatf& r, const float3& s)
{
    global_matrix = mu::transform(t, r, s);
}


SkeletonNode::SkeletonNode(Node* parent)
    : super(parent)
{
}

Node::Type SkeletonNode::getType() const
{
    return Type::Skeleton;
}

void SkeletonNode::convert(const ConvertOptions& opt)
{
    super::convert(opt);

    if (opt.scale_factor != 1.0f) {
        auto convert = [&opt](float4x4& m) { (float3&)m[3] *= opt.scale_factor; };
        for (auto& joint : joints) {
            convert(joint->bindpose);
            convert(joint->restpose);
            convert(joint->local_matrix);
            convert(joint->global_matrix);
        }
    }
    if (opt.flip_x) {
        auto convert = [](float4x4& m) { m = flip_x(m); };
        for (auto& joint : joints) {
            convert(joint->bindpose);
            convert(joint->restpose);
            convert(joint->local_matrix);
            convert(joint->global_matrix);
        }
    }
    if (opt.flip_yz) {
        auto convert = [](float4x4& m) { m = flip_z(swap_yz(m)); };
        for (auto& joint : joints) {
            convert(joint->bindpose);
            convert(joint->restpose);
            convert(joint->local_matrix);
            convert(joint->global_matrix);
        }
    }
}

void SkeletonNode::clear()
{
    joints.clear();
}

SkeletonNode::Joint* SkeletonNode::addJoint(const std::string& path)
{
    auto ret = new Joint(path);
    joints.push_back(JointPtr(ret));

    // handle parenting
    auto pos = path.find_last_of('/');
    if (pos != std::string::npos) {
        auto parent_path = std::string(path.begin(), path.begin() + pos);
        if (auto parent = findJointByPath(parent_path)) {
            ret->parent = parent;
            parent->children.push_back(ret);
        }
    }
    return ret;
}

static void UpdateGlobalMatricesImpl(SkeletonNode::Joint& joint, const float4x4& base)
{
    if (joint.parent)
        joint.global_matrix = joint.local_matrix * joint.parent->global_matrix;
    else
        joint.global_matrix = joint.local_matrix * base;

    for (auto child : joint.children)
        UpdateGlobalMatricesImpl(*child, base);
}

void SkeletonNode::updateGlobalMatrices(const float4x4& base)
{
    // update global matrices from top to down recursively
    for (auto& joint : joints) {
        if (!joint->parent)
            UpdateGlobalMatricesImpl(*joint, base);
    }
}

SkeletonNode::Joint* SkeletonNode::findJointByName(const std::string& name)
{
    auto it = std::find_if(joints.begin(), joints.end(),
        [&name](auto& joint) { return joint->name == name; });
    return it == joints.end() ? nullptr : (*it).get();
}

SkeletonNode::Joint* SkeletonNode::findJointByPath(const std::string& path)
{
    // rbegin() & rend() because in many cases this method is called to find last added joint
    auto it = std::find_if(joints.rbegin(), joints.rend(),
        [&path](auto& joint) { return joint->path == path; });
    return it == joints.rend() ? nullptr : (*it).get();
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
