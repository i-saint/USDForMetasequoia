#include "pch.h"
#include "mqusd.h"
#include "mqusdSceneGraph.h"

namespace mqusd {

const double default_time = std::numeric_limits<double>::quiet_NaN();

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
        case Type::Unknown: ret = std::make_shared<Node>(); break;
        case Type::Root: ret = std::make_shared<RootNode>(); break;
        case Type::Xform: ret = std::make_shared<XformNode>(); break;
        case Type::Mesh: ret = std::make_shared<MeshNode>(); break;
        case Type::Blendshape: ret = std::make_shared<BlendshapeNode>(); break;
        case Type::SkelRoot: ret = std::make_shared<SkelRootNode>(); break;
        case Type::Skeleton: ret = std::make_shared<SkeletonNode>(); break;
        case Type::Instancer: ret = std::make_shared<InstancerNode>(); break;
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

std::string Node::makeUniqueName(const char* name)
{
    std::string base = SanitizeNodeName(name);
    std::string ret = base;
    for (int i = 1; ; ++i) {
        bool ok = true;
        for (auto c : children) {
            if (ret == GetLeafName(c->path)) {
                ok = false;
                break;
            }
        }
        if (ok)
            break;

        char buf[16];
        sprintf(buf, "_%d", i);
        ret = base + buf;
    }
    return ret;
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

    transform_container(blendshape_paths, blendshapes, [](auto& d, auto* s) {
        d = s->path;
    });
    if (skeleton) {
        skeleton_path = skeleton->path;
        transform_container(joint_paths, joints, [](auto& d, auto* s) {
            d = s->path;
        });
    }

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

    transform_container(blendshapes, blendshape_paths, [this](auto& d, auto& s) {
        d = static_cast<BlendshapeNode*>(scene->findNodeByPath(s));
        if (!d) {
            mqusdDbgPrint("blendshape not found %s\n", s.c_str());
        }
    });

    skeleton = static_cast<SkeletonNode*>(scene->findNodeByPath(skeleton_path));
    if (skeleton) {
        transform_container(joints, joint_paths, [this](auto& d, auto& s) {
            d = skeleton->findJointByPath(s);
            if (!d) {
                mqusdDbgPrint("joint not found %s\n", s.c_str());
            }
        });
    }
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

    if (opt.flip_v) {
        mu::InvertV(uvs.data(), uvs.size());
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

    blendshapes.clear();
    blendshape_paths.clear();

    skeleton = nullptr;
    skeleton_path.clear();
    joints.clear();
    joint_paths.clear();
    joint_matrices.clear();

    joints_per_vertex = 0;
    joint_indices.clear();
    joint_weights.clear();
    bind_transform = float4x4::identity();
}

void MeshNode::merge(const MeshNode& v, const float4x4& trans)
{
    auto append = [](auto& dst, const auto& src) {
        dst.insert(dst.end(), src.cdata(), src.cdata() + src.size());
    };
    auto append_padded = [](auto& dst, const auto& src, int base_size, const auto default_value) {
        if (!dst.empty() && src.empty())
            dst.insert(dst.end(), src.cdata(), src.cdata() + src.size());
        else if (!dst.empty() && src.empty())
            dst.resize(dst.size() + src.size(), default_value);
        else if (dst.empty() && !src.empty())
            dst.resize(base_size + src.size(), default_value);
    };

    int vertex_offset = (int)points.size();
    int index_offset = (int)indices.size();

    append(points, v.points);
    append(counts, v.counts);
    append(indices, v.indices);
    append_padded(normals,      v.normals,      index_offset, float3::zero());
    append_padded(uvs,          v.uvs,          index_offset, float2::zero());
    append_padded(colors,       v.colors,       index_offset, float4::zero());
    append_padded(material_ids, v.material_ids, index_offset, 0);

    joints_per_vertex = 0;
    joint_indices.clear();
    joint_weights.clear();
    bind_transform = float4x4::identity();

    if (vertex_offset > 0) {
        int index_end = index_offset + (int)v.indices.size();
        for (int ii = index_offset; ii < index_end; ++ii)
            indices[ii] += vertex_offset;
    }

    if (trans != float4x4::identity()) {
        auto p = points.data() + vertex_offset;
        auto n = normals.data() + index_offset;
        mu::MulPoints(trans, p, p, v.points.size());
        mu::MulVectors(trans, n, n, v.normals.size());
    }
}

void MeshNode::bake(MeshNode& dst, const float4x4& trans)
{
    dst.merge(*this);

    // setup
    int npoints = (int)points.size();
    float3* dst_points = dst.points.end() - npoints;
    float3* dst_normals = normals.empty() ? nullptr : dst.normals.end() - normals.size();

    // blendshape
    if (!blendshapes.empty() && blendshapes.size() == blendshape_weights.size()) {
        size_t nbs = blendshapes.size();
        for (size_t bsi = 0; bsi < nbs; ++bsi) {
            float weight = blendshape_weights[bsi];
            if (weight == 0.0f)
                continue;

            const auto* bs = blendshapes[bsi];
            const float3* point_offsets = bs->point_offsets.cdata();
            if (!bs->indices.empty()) {
                size_t nbsi = bs->indices.size();
                const int* i = bs->indices.cdata();
                for (size_t oi = 0; oi < nbsi; ++oi)
                    dst_points[i[oi]] += point_offsets[oi] * weight;
            }
            else if (bs->point_offsets.size() == npoints) {
                for (size_t oi = 0; oi < npoints; ++oi)
                    dst_points[oi] += point_offsets[oi] * weight;
            }

            // todo: handle normal offsets
            //auto normal_offsets = bs->normal_offsets.cdata();
        }
    }

    // skeleton
    if (skeleton) {
        transform_container(joint_matrices, joints, [this](auto& m, auto j) {
            m = mu::invert(j->bindpose) * j->global_matrix;
        });

        auto* iv = joint_indices.cdata();
        auto* wv = joint_weights.cdata();
        for (int pi = 0; pi < npoints; ++pi) {
            {
                auto p = mu::mul_p(bind_transform, dst_points[pi]);
                auto r = float3::zero();
                for (int ji = 0; ji < joints_per_vertex; ++ji)
                    r += mu::mul_p(joint_matrices[iv[ji]], p) * wv[ji];
                dst_points[pi] = r;
            }
            if (dst_normals) {
                auto n = mu::mul_v(bind_transform, dst_normals[pi]);
                auto r = float3::zero();
                for (int ji = 0; ji < joints_per_vertex; ++ji)
                    r += mu::mul_v(joint_matrices[iv[ji]], n) * wv[ji];
                dst_normals[pi] = r;
            }
            iv += joints_per_vertex;
            wv += joints_per_vertex;
        }
    }

    if (trans != float4x4::identity()) {
        mu::MulPoints(trans, dst_points, dst_points, npoints);
        mu::MulVectors(trans, dst_normals, dst_normals, normals.size());
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
        skeleton_path = skeleton->path;

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
    F(proto_paths) F(matrices)

void InstancerNode::serialize(std::ostream& os)
{
    super::serialize(os);

    transform_container(proto_paths, protos, [](std::string& d, const Node* s) {
        d = s->path;
    });

#define Body(V) write(os, V);
    EachMember(Body)
#undef Body
}

void InstancerNode::deserialize(std::istream& is)
{
    super::deserialize(is);
#define Body(V) read(is, V);
    EachMember(Body)
#undef Body
}

void InstancerNode::resolve()
{
    super::resolve();

    transform_container(protos, proto_paths, [this](auto& d, auto& s) {
        d = scene->findNodeByPath(s);
        if (!d) {
            mqusdDbgPrint("not found %s\n", s.c_str());
        }
    });
}

#undef EachMember

InstancerNode::InstancerNode(Node* parent, const char* name)
    : super(parent, name)
{
}

Node::Type InstancerNode::getType() const
{
    return Type::Instancer;
}

void InstancerNode::convert(const ConvertOptions& opt)
{
    super::convert(opt);

    if (opt.scale_factor != 1.0f) {
        auto convert = [&opt](float4x4& m) { (float3&)m[3] *= opt.scale_factor; };
        for (auto& m : matrices)
            convert(m);
    }
    if (opt.flip_x) {
        auto convert = [](float4x4& m) { m = flip_x(m); };
        for (auto& m : matrices)
            convert(m);
    }
    if (opt.flip_yz) {
        auto convert = [](float4x4& m) { m = flip_z(swap_yz(m)); };
        for (auto& m : matrices)
            convert(m);
    }
}

void InstancerNode::gatherMeshes(ProtoRecord& prec, Node* n, float4x4 m)
{
    if (auto xform = dynamic_cast<XformNode*>(n))
        m = xform->local_matrix * m;

    if (auto mesh = dynamic_cast<MeshNode*>(n)) {
        MeshRecord tmp;
        tmp.mesh = mesh;
        tmp.matrix = m;
        prec.mesh_records.push_back(tmp);
    }
    else if (auto inst = dynamic_cast<InstancerNode*>(n)) {
        MeshRecord tmp;
        tmp.instancer = inst;
        tmp.matrix = m;
        prec.mesh_records.push_back(tmp);
    }

    n->eachChild([this, &prec, &m](Node* c) {
        gatherMeshes(prec, c, m);
    });
}

void InstancerNode::gatherMeshes()
{
    if (!proto_records.empty())
        return;

    size_t nprotos = protos.size();
    proto_records.clear();
    proto_records.resize(nprotos);
    for (size_t i = 0; i < nprotos; ++i) {
        auto& prec = proto_records[i];
        prec.mesh_records.clear();
        prec.merged_mesh.clear();

        protos[i]->eachChild([this, &prec](Node* n) {
            gatherMeshes(prec, n, float4x4::identity());
        });
    }
}

void InstancerNode::bake(MeshNode& dst, const float4x4& trans)
{
    gatherMeshes();

    for (auto& prec : proto_records) {
        prec.merged_mesh.clear();
        for (auto& mrec : prec.mesh_records) {
            if (mrec.mesh)
                mrec.mesh->bake(prec.merged_mesh, mrec.matrix);
            if (mrec.instancer)
                mrec.instancer->bake(prec.merged_mesh, mrec.matrix);
        }
    }

    size_t vertex_offset = dst.points.size();
    size_t normal_offset = dst.normals.size();

    size_t ninstances = proto_indices.size();
    if (matrices.size() == ninstances) {
        // reserve space
        size_t npoints = 0;
        size_t nindices = 0;
        size_t nfaces = 0;
        for (size_t i = 0; i < ninstances; ++i) {
            auto& mesh = proto_records[proto_indices[i]].merged_mesh;
            npoints += mesh.points.size();
            nindices += mesh.indices.size();
            nfaces += mesh.counts.size();
        }
        dst.points.reserve(npoints);
        dst.indices.reserve(nindices);
        dst.counts.reserve(nfaces);
        dst.normals.reserve(nindices);
        dst.uvs.reserve(nindices);
        dst.colors.reserve(nindices);
        dst.material_ids.reserve(nfaces);

        // do merge
        for (size_t i = 0; i < ninstances; ++i)
            dst.merge(proto_records[proto_indices[i]].merged_mesh, matrices[i]);
    }

    if (trans != float4x4::identity()) {
        auto p = dst.points.data() + vertex_offset;
        auto n = dst.normals.data() + normal_offset;
        mu::MulPoints(trans, p, p, dst.points.size() - vertex_offset);
        mu::MulVectors(trans, n, n, dst.normals.size() - normal_offset);
    }
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
    F(path) F(nodes) F(up_axis) F(frame_rate) F(time_start) F(time_end) F(time_current)

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

    Node* ret = nullptr;
    if (impl) {
        ret = impl->createNode(parent, name, type);
    }
    else {
        ret = createNodeImpl(parent, name, type);
        registerNode(ret);
    }
    return ret;
}

void Scene::registerNode(Node* n)
{
    if (n) {
        nodes.push_back(NodePtr(n));
        if (n->getType() == Node::Type::Root)
            root_node = static_cast<RootNode*>(n);
    }
}

Node* Scene::createNodeImpl(Node* parent, const char* name, Node::Type type)
{
    Node* ret = nullptr;
    switch (type) {
#define Case(E, T) case Node::Type::E: ret = new T(parent, name); break;
        Case(Xform, XformNode);
        Case(Mesh, MeshNode);
        Case(Blendshape, BlendshapeNode);
        Case(SkelRoot, SkelRootNode);
        Case(Skeleton, SkeletonNode);
        Case(Instancer, InstancerNode);
        Case(Material, MaterialNode);
#undef Case
    default: break;
    }
    return ret;
}

SceneInterface::~SceneInterface()
{
}

} // namespace mqusd
