#include "pch.h"
#include "mqusd.h"
#include "mqusdSceneGraph.h"

namespace mqusd {

double default_time = std::numeric_limits<double>::quiet_NaN();

std::string SanitizeNodeName(const std::string& name)
{
    std::string ret = name;
    for (auto& c : ret) {
        if (!std::isalnum(c))
            c = '_';
    }
    return ret;
}

std::string SanitizeNodePath(const std::string& path)
{
    std::string ret = path;
    for (auto& c : ret) {
        if (c == '/')
            continue;
        if (!std::isalnum(c))
            c = '_';
    }
    return ret;
}

std::string GetParentPath(const std::string& path)
{
    auto pos = path.find_last_of('/');
    if (pos == std::string::npos || (pos == 0 && path.size() == 1))
        return "";
    else if (pos == 0)
        return "/";
    else
        return std::string(path.begin(), path.begin() + pos);
}

std::string GetLeafName(const std::string& path)
{
    auto pos = path.find_last_of('/');
    if (pos == std::string::npos)
        return path;
    else
        return std::string(path.begin() + pos + 1, path.end());
}


static uint32_t GenID()
{
    static uint32_t s_seed;
    if (s_seed == 0) {
        auto r = std::mt19937{};
        auto d = std::uniform_int_distribution<uint32_t>{ 0, 0xefffffff };
        s_seed = d(r);
    }
    return ++s_seed;
}

bool ConvertOptions::operator==(const ConvertOptions& v) const
{
    return
        scale_factor == v.scale_factor &&
        flip_x == v.flip_x &&
        flip_yz == v.flip_yz &&
        flip_faces == v.flip_faces;
}

bool ConvertOptions::operator!=(const ConvertOptions& v) const
{
    return !(*this == v);
}


#define EachMember(F)\
    F(path) F(id)

void Node::serialize(std::ostream& os)
{
    auto type = getType();
    write(os, type);

#define Body(V) write(os, V);
    EachMember(Body)
#undef Body
}

void Node::deserialize(std::istream& is)
{
    // type will be consumed by create()
#define Body(V) read(is, V);
    EachMember(Body)
#undef Body
}
void Node::resolve()
{
    children.clear();
    parent = scene->findNodeByPath(GetParentPath(path));
    if (parent)
        parent->children.push_back(this);
}
#undef EachMember

void Node::deserialize(std::istream& is, NodePtr& ret)
{
    Type type;
    read(is, type);

    if (!ret) {
        switch (type) {
        case Type::Root: ret = std::make_shared<RootNode>(); break;
        case Type::Xform: ret = std::make_shared<XformNode>(); break;
        case Type::Mesh: ret = std::make_shared<MeshNode>(); break;
        case Type::Blendshape: ret = std::make_shared<BlendshapeNode>(); break;
        case Type::SkelRoot: ret = std::make_shared<SkelRootNode>(); break;
        case Type::Skeleton: ret = std::make_shared<SkeletonNode>(); break;
        case Type::Material: ret = std::make_shared<MaterialNode>(); break;
        default:
            throw std::runtime_error("Node::create() failed");
            break;
        }
    }
    if (ret)
        ret->deserialize(is);
}

Node::Node(Node* p, const char* name)
    : parent(p)
{
    scene = Scene::getCurrent();
    id = GenID();
    if (parent)
        parent->children.push_back(this);
    if (name) {
        if (parent)
            path = parent->path;
        if (path != "/")
            path += "/";
        if (std::strcmp(name, "/") != 0)
            path += name;
    }
}

Node::~Node()
{
}

Node::Type Node::getType() const
{
    return Type::Unknown;
}

void Node::convert(const ConvertOptions& /*opt*/)
{
}

std::string Node::getName() const
{
    return GetLeafName(path);
}

const std::string& Node::getPath() const
{
    return path;
}

template<class NodeT>
NodeT* Node::findParent()
{
    for (Node* p = parent; p != nullptr; p = p->parent) {
        if (auto tp = dynamic_cast<NodeT*>(p))
            return tp;
    }
    return nullptr;
}


RootNode::RootNode()
    : super(nullptr, "/")
{
}

Node::Type RootNode::getType() const
{
    return Type::Root;
}


#define EachMember(F)\
    F(local_matrix) F(global_matrix)

void XformNode::serialize(std::ostream& os)
{
    super::serialize(os);
#define Body(V) write(os, V);
    EachMember(Body)
#undef Body
}

void XformNode::deserialize(std::istream& is)
{
    super::deserialize(is);
#define Body(V) read(is, V);
    EachMember(Body)
#undef Body
}

void XformNode::resolve()
{
    super::resolve();
    parent_xform = findParent<XformNode>();
}
#undef EachMember

XformNode::XformNode(Node* p, const char *name)
    : super(p, name)
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



#define EachMember(F)\
    F(points) F(normals) F(uvs) F(colors) F(material_ids) F(counts) F(indices)\
    F(joints_per_vertex) F(joint_indices) F(joint_weights) F(bind_transform)\
    F(blendshape_paths) F(skeleton_path) F(joint_paths)

void MeshNode::serialize(std::ostream& os)
{
    super::serialize(os);

    // preserve paths of related nodes to resolve on deserialize
    size_t nbs = blendshapes.size();
    blendshape_paths.resize(nbs);
    for (size_t bi = 0; bi < nbs; ++bi)
        blendshape_paths[bi] = blendshapes[bi]->getPath();
    if (skeleton)
        skeleton_path = skeleton->getPath();

#define Body(V) write(os, V);
    EachMember(Body)
#undef Body
}

void MeshNode::deserialize(std::istream& is)
{
    super::deserialize(is);
#define Body(V) read(is, V);
    EachMember(Body)
#undef Body
}
void MeshNode::resolve()
{
    super::resolve();

    blendshapes.clear();
    for (auto& bp : blendshape_paths) {
        if (auto bs = scene->findNodeByPath(bp)) {
            blendshapes.push_back(static_cast<BlendshapeNode*>(bs));
        }
        else {
            mqusdDbgPrint("MeshNode::resolve(): node not found %s\n", bp.c_str());
        }
    }
    skeleton = static_cast<SkeletonNode*>(scene->findNodeByPath(skeleton_path));
}
#undef EachMember

MeshNode::MeshNode(Node* p, const char* name)
    : super(p, name)
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
        auto* cdata = counts.cdata();

        auto do_flip = [nfaces, cdata](auto *data) {
            for (size_t fi = 0; fi < nfaces; ++fi) {
                int c = cdata[fi];
                std::reverse(data, data + c);
                data += c;
            }
        };

        do_flip(indices.data());
        if (uvs.size() == indices.size())
            do_flip(uvs.data());
        if (normals.size() == indices.size())
            do_flip(normals.data());
        if (colors.size() == indices.size())
            do_flip(colors.data());
    }
}

void MeshNode::applyTransform(const float4x4& v)
{
    if (v != float4x4::identity()) {
        mu::MulPoints(v, points.data(), points.data(), points.size());
        mu::MulVectors(v, normals.data(), normals.data(), normals.size());
        bind_transform *= v;

        for (auto bs : blendshapes) {
            mu::MulVectors(v, bs->point_offsets.data(), bs->point_offsets.data(), bs->point_offsets.size());
            mu::MulVectors(v, bs->normal_offsets.data(), bs->normal_offsets.data(), bs->normal_offsets.size());
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

    blendshape_paths.clear();
    blendshapes.clear();

    skeleton = nullptr;
    skeleton_path.clear();
    joint_paths.clear();

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

MeshNode* MeshNode::findParentMesh() const
{
    for (auto p = parent; p; p = p->parent) {
        if (p->getType() == Node::Type::Mesh)
            return static_cast<MeshNode*>(p);
    }
    return nullptr;
}



#define EachMember(F)\
    F(indices) F(point_offsets) F(normal_offsets)

void BlendshapeNode::serialize(std::ostream& os)
{
    super::serialize(os);
#define Body(V) write(os, V);
    EachMember(Body)
#undef Body
}

void BlendshapeNode::deserialize(std::istream& is)
{
    super::deserialize(is);
#define Body(V) read(is, V);
    EachMember(Body)
#undef Body
}
#undef EachMember

BlendshapeNode::BlendshapeNode(Node* p, const char* name)
    : super(p, name)
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

void BlendshapeNode::makeMesh(MeshNode& dst, const MeshNode& base)
{
    dst.points = base.points;
    dst.normals = base.normals;
    dst.uvs = base.uvs;
    dst.colors = base.colors;
    dst.material_ids = base.material_ids;
    dst.counts = base.counts;
    dst.indices = base.indices;

    if (indices.empty()) {
        size_t npoints = dst.points.size();
        if (!point_offsets.empty() && point_offsets.size() == npoints) {
            auto* points = dst.points.data();
            for (size_t pi = 0; pi < npoints; ++pi)
                points[pi] += point_offsets[pi];
        }

        size_t nnormals = dst.points.size();
        if (!normal_offsets.empty() && normal_offsets.size() == nnormals) {
            auto* normals = dst.normals.data();
            for (size_t pi = 0; pi < nnormals; ++pi)
                normals[pi] += normal_offsets[pi];
            mu::Normalize(normals, dst.normals.size());
        }
    }
    else {
        size_t nindices = indices.size();
        auto* idx = indices.data();
        if (!point_offsets.empty()) {
            auto* points = dst.points.data();
            for (size_t ii = 0; ii < nindices; ++ii)
                points[idx[ii]] += point_offsets[ii];
        }
        if (!normal_offsets.empty()) {
            auto* normals = dst.normals.data();
            for (size_t ii = 0; ii < nindices; ++ii)
                normals[idx[ii]] += normal_offsets[ii];
            mu::Normalize(normals, dst.normals.size());
        }
    }
}

void BlendshapeNode::makeOffsets(const MeshNode& target, const MeshNode& base)
{
    if (!base.points.empty() && base.points.size() == target.points.size()) {
        size_t n = base.points.size();
        point_offsets.resize_discard(n);
        auto* b = base.points.cdata();
        auto* t = target.points.cdata();
        auto* d = point_offsets.data();
        for (size_t i = 0; i < n; ++i)
            d[i] = t[i] - b[i];
    }
    if (!base.normals.empty() && base.normals.size() == target.normals.size()) {
        size_t n = base.normals.size();
        normal_offsets.resize_discard(n);
        auto* b = base.normals.cdata();
        auto* t = target.normals.cdata();
        auto* d = normal_offsets.data();
        for (size_t i = 0; i < n; ++i)
            d[i] = t[i] - b[i];
    }
}



#define EachMember(F)\
    F(skeleton_path)

void SkelRootNode::serialize(std::ostream& os)
{
    super::serialize(os);

    if (skeleton)
        skeleton_path = skeleton->getPath();

#define Body(V) write(os, V);
    EachMember(Body)
#undef Body
}

void SkelRootNode::deserialize(std::istream& is)
{
    super::deserialize(is);
#define Body(V) read(is, V);
    EachMember(Body)
#undef Body
}

void SkelRootNode::resolve()
{
    super::resolve();
    skeleton = static_cast<SkeletonNode*>(scene->findNodeByPath(skeleton_path));
}
#undef EachMember

SkelRootNode::SkelRootNode(Node* parent, const char* name)
    : super(parent, name)
{
}

Node::Type SkelRootNode::getType() const
{
    return Type::SkelRoot;
}



#define EachMember(F)\
    F(path) F(index) F(bindpose) F(restpose) F(local_matrix) F(global_matrix)

void Joint::serialize(std::ostream& os)
{
#define Body(V) write(os, V);
    EachMember(Body)
#undef Body
}

void Joint::deserialize(std::istream& is)
{
#define Body(V) read(is, V);
    EachMember(Body)
#undef Body
}

void Joint::resolve()
{
    children.clear();
    if (auto v = skeleton->findJointByPath(GetParentPath(path))) {
        parent = v;
        parent->children.push_back(this);
    }
}
#undef EachMember

void Joint::deserialize(std::istream& is, JointPtr& ret)
{
    if (!ret)
        ret = std::make_shared<Joint>();
    ret->deserialize(is);
}

Joint::Joint()
{
}

Joint::Joint(SkeletonNode* s, const std::string& p)
    : skeleton(s)
    , path(p)
{
    if (auto v = skeleton->findJointByPath(GetParentPath(path))) {
        parent = v;
        parent->children.push_back(this);
    }
}

std::string Joint::getName() const
{
    return GetLeafName(path);
}

std::tuple<float3, quatf, float3> Joint::getLocalTRS() const
{
    return mu::extract_trs(local_matrix);
}
std::tuple<float3, quatf, float3> Joint::getGlobalTRS() const
{
    return mu::extract_trs(global_matrix);
}

void Joint::setLocalTRS(const float3& t, const quatf& r, const float3& s)
{
    local_matrix = mu::transform(t, r, s);
}
void Joint::setGlobalTRS(const float3& t, const quatf& r, const float3& s)
{
    global_matrix = mu::transform(t, r, s);
}



#define EachMember(F)\
    F(joints)

void SkeletonNode::serialize(std::ostream& os)
{
    super::serialize(os);
#define Body(V) write(os, V);
    EachMember(Body)
#undef Body
}

void SkeletonNode::deserialize(std::istream& is)
{
    super::deserialize(is);
#define Body(V) read(is, V);
    EachMember(Body)
#undef Body
}
void SkeletonNode::resolve()
{
    super::resolve();
    for (auto& joint : joints) {
        joint->skeleton = this;
        joint->resolve();
    }
}
#undef EachMember

SkeletonNode::SkeletonNode(Node* parent, const char* name)
    : super(parent, name)
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

Joint* SkeletonNode::addJoint(const std::string& jpath_)
{
    auto jpath = SanitizeNodePath(jpath_);
    auto ret = new Joint(this, jpath);
    ret->index = (int)joints.size();
    joints.push_back(JointPtr(ret));
    return ret;
}

static void UpdateGlobalMatricesImpl(Joint& joint, const float4x4& base)
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

Joint* SkeletonNode::findJointByPath(const std::string& jpath)
{
    // rbegin() & rend() because in many cases this method is used to find last added joint
    auto it = std::find_if(joints.rbegin(), joints.rend(),
        [&jpath](auto& joint) { return joint->path == jpath; });
    return it == joints.rend() ? nullptr : it->get();
}



#define EachMember(F)\
    F(shader) F(use_vertex_color) F(double_sided) F(color) F(diffuse) F(alpha) F(ambient_color) F(specular_color) F(emission_color)

void MaterialNode::serialize(std::ostream& os)
{
    super::serialize(os);
#define Body(V) write(os, V);
    EachMember(Body)
#undef Body
}

void MaterialNode::deserialize(std::istream& is)
{
    super::deserialize(is);
#define Body(V) read(is, V);
    EachMember(Body)
#undef Body
}
#undef EachMember

MaterialNode::MaterialNode(Node* p, const char* name)
    : super(p, name)
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


static thread_local Scene* g_current_scene;

Scene* Scene::getCurrent()
{
    return g_current_scene;
}

#define EachMember(F)\
    F(path) F(nodes) F(up_axis) F(time_start) F(time_end) F(time_current)

void Scene::serialize(std::ostream& os)
{
#define Body(V) mqusd::write(os, V);
    EachMember(Body)
#undef Body
}

void Scene::deserialize(std::istream& is)
{
#define Body(V) mqusd::read(is, V);
    EachMember(Body)
#undef Body

    for (auto& n : nodes)
        n->resolve();
    if (impl) {
        for (auto& n : nodes)
            impl->wrapNode(n.get());
    }
}
#undef EachMember

void Scene::deserialize(std::istream& is, std::shared_ptr<Scene>& ret)
{
    if (!ret)
        ret = std::shared_ptr<Scene>();
    ret->deserialize(is);
}

Scene::Scene()
{
}

Scene::~Scene()
{
    close();
}

bool Scene::open(const char* path_)
{
    g_current_scene = this;
    if (impl && impl->open(path_)) {
        path = path_;
        return true;
    }
    return false;
}

bool Scene::create(const char* path_)
{
    g_current_scene = this;
    if (impl && impl->create(path_)) {
        path = path_;
        return true;
    }
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
    nodes.clear();
}

void Scene::read(double time)
{
    g_current_scene = this;
    time_current = time;
    if (impl)
        impl->read();
}

void Scene::write(double time)
{
    g_current_scene = const_cast<Scene*>(this);
    time_current = time;
    if (impl)
        impl->write();
}

Node* Scene::findNodeByID(uint32_t id)
{
    if (id == 0)
        return nullptr;

    auto it = std::find_if(nodes.begin(), nodes.end(), [id](NodePtr& n) { return n->id == id; });
    return it == nodes.end() ? nullptr : it->get();
}

Node* Scene::findNodeByPath(const std::string& npath)
{
    if (npath.empty())
        return nullptr;

    auto it = std::find_if(nodes.begin(), nodes.end(), [&npath](NodePtr& n) { return n->path == npath; });
    return it == nodes.end() ? nullptr : it->get();
}

Node* Scene::createNode(Node* parent, const char* name, Node::Type type)
{
    g_current_scene = this;
    if (impl) {
        auto ret = impl->createNode(parent, name, type);
        if (ret)
            nodes.push_back(NodePtr(ret));
        return ret;
    }
    else {
        return createNodeImpl(parent, name, type);
    }
}

Node* Scene::createNodeImpl(Node* parent, const char* name, Node::Type type)
{
    Node* ret = nullptr;
    switch (type) {
#define Case(E, T) case Node::Type::E: ret = new T(parent, name); break;
        Case(Mesh, MeshNode);
        Case(Blendshape, BlendshapeNode);
        Case(SkelRoot, SkelRootNode);
        Case(Skeleton, SkeletonNode);
        Case(Material, MaterialNode);
        Case(Xform, XformNode);
#undef Case
    default: break;
    }
    return ret;
}

SceneInterface::~SceneInterface()
{
}

} // namespace mqusd
