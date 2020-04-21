#include "pch.h"
#include "SceneGraph.h"
#include "sgSerializationImpl.h"

namespace sg {

const double default_time = std::numeric_limits<double>::quiet_NaN();

bool IsDefaultTime(double t)
{
    return std::isnan(t);
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

sgRegisterType(Node);

#define EachMember(F)\
    F(path) F(display_name) F(id) F(scene) F(parent) F(children)

void Node::serialize(serializer& s) const
{
    EachMember(sgWrite)
}

void Node::deserialize(deserializer& d)
{
    EachMember(sgRead)
}
#undef EachMember

Node::Node() {}

Node::Node(Node* p, const char* name)
    : parent(p)
{
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

std::string Node::getDisplayName() const
{
    return display_name.empty() ? getName() : display_name;
}

const std::string& Node::getPath() const
{
    return path;
}


sgRegisterType(RootNode);

RootNode::RootNode()
    : super(nullptr, "/")
{
}

Node::Type RootNode::getType() const
{
    return Type::Root;
}


sgRegisterType(XformNode);

#define EachMember(F)\
    F(visibility) F(local_matrix) F(global_matrix) F(parent_xform)

void XformNode::serialize(serializer& s) const
{
    super::serialize(s);
    EachMember(sgWrite)
}

void XformNode::deserialize(deserializer& d)
{
    super::deserialize(d);
    EachMember(sgRead)
}
#undef EachMember

XformNode::XformNode() {}

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


sgRegisterType(FaceSet);

#define EachMember(F)\
    F(material) F(faces) F(counts) F(indices)

void FaceSet::serialize(serializer& s) const
{
    EachMember(sgWrite)
}
void FaceSet::deserialize(deserializer& d)
{
    EachMember(sgRead)
}
#undef EachMember

void FaceSet::clear()
{
    material = nullptr;
    faces.clear();
    counts.clear();
    indices.clear();
    userdata = nullptr;
}

FaceSetPtr FaceSet::clone()
{
    return std::make_shared<FaceSet>(*this);
}

void FaceSet::merge(const FaceSet& v, int face_offset, int index_offset)
{
    auto* fv = append(faces, v.faces);
    if (face_offset > 0)
        add(fv, v.faces.size(), face_offset);

    append(counts, v.counts);

    auto* iv = append(indices, v.indices);
    if (index_offset > 0)
        add(iv, v.indices.size(), index_offset);
}

void FaceSet::addOffset(int face_offset, int index_offset)
{
    if (face_offset > 0)
        add(faces.data(), faces.size(), face_offset);
    if (index_offset > 0)
        add(indices.data(), indices.size(), index_offset);
}


sgRegisterType(MeshNode);

#define EachMember(F)\
    F(points) F(normals) F(uvs) F(colors) F(material_ids) F(counts) F(indices)\
    F(skeleton) F(joints) F(joints_per_vertex) F(joint_indices) F(joint_weights) F(bind_transform)\
    F(blendshapes)\
    F(materials) F(facesets)

void MeshNode::serialize(serializer& s) const
{
    super::serialize(s);
    EachMember(sgWrite)
}

void MeshNode::deserialize(deserializer& d)
{
    super::deserialize(d);
    EachMember(sgRead)
}
#undef EachMember

MeshNode::MeshNode() {}

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

        eachBSTarget([&opt](auto& t) {
            mu::Scale(t.point_offsets.data(), opt.scale_factor, t.point_offsets.size());
        });
    }

    if (opt.flip_x) {
        mu::InvertX(points.data(), points.size());
        mu::InvertX(normals.data(), normals.size());
        bind_transform = flip_x(bind_transform);

        eachBSTarget([](auto& t) {
            mu::InvertX(t.point_offsets.data(), t.point_offsets.size());
            mu::InvertX(t.normal_offsets.data(), t.normal_offsets.size());
        });
    }

    if (opt.flip_yz) {
        auto convert = [this](auto& v) { return flip_z(swap_yz(v)); };
        for (auto& v : points) v = convert(v);
        for (auto& v : normals) v = convert(v);
        bind_transform = convert(bind_transform);

        eachBSTarget([&convert](auto& t) {
            for (auto& v : t.point_offsets) v = convert(v);
            for (auto& v : t.normal_offsets) v = convert(v);
        });
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

        eachBSTarget([this, &do_flip](auto& t) {
            if (t.normal_offsets.size() == indices.size())
                do_flip(t.normal_offsets.data());
        });
    }
}

void MeshNode::applyTransform(const float4x4& v)
{
    if (v != float4x4::identity()) {
        mu::MulPoints(v, points.data(), points.data(), points.size());
        mu::MulVectors(v, normals.data(), normals.data(), normals.size());
        bind_transform *= v;

        eachBSTarget([&v](auto& t) {
            mu::MulVectors(v, t.point_offsets.data(), t.point_offsets.data(), t.point_offsets.size());
            mu::MulVectors(v, t.normal_offsets.data(), t.normal_offsets.data(), t.normal_offsets.size());
        });
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

void MeshNode::buildFaceSets(bool cleanup)
{
    if (material_ids.empty() || materials.empty() || material_ids.size() != counts.size()) {
        facesets.clear();
        return;
    }

    // setup
    facesets.resize(materials.size() + 1);
    each_with_index(facesets, [this](auto& faceset, int i) {
        if (!faceset)
            faceset = std::make_shared<FaceSet>();
        faceset->clear();
        if (i < (int)materials.size())
            faceset->material = materials[i];
    });

    // select face by material
    each_with_index(material_ids, [this](int mid, int fi) {
        if (mid >= 0)
            facesets[mid]->faces.push_back(fi);
    });

    // erase empty facesets
    erase_if(facesets, [](auto& faceset) {
        return faceset->faces.empty();
    });

    if (cleanup) {
        material_ids.clear();
        materials.clear();
    }
}

void MeshNode::buildMaterialIDs(bool cleanup)
{
    if (counts.empty() || facesets.empty())
        return;

    material_ids.resize_discard(counts.size());
    fill(material_ids, -1);

    materials.resize(facesets.size());
    // process by reverse order because of later element maybe dummy.
    // (dummy contains overlapped faces and is overwritten by other valid facesets)
    each_with_index_r(facesets, [this](auto& faceset, int i) {
        auto m = faceset->material;
        materials[i] = m;
        int mi = m ? m->index : -1;
        for (int f : faceset->faces)
            material_ids[f] = mi;
    });

    if (cleanup) {
        facesets.clear();
    }
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
    joint_matrices.clear();

    joints_per_vertex = 0;
    joint_indices.clear();
    joint_weights.clear();
    bind_transform = float4x4::identity();

    materials.clear();
    facesets.clear();
}

void MeshNode::merge(const MeshNode& v, const float4x4& trans)
{
    auto append = [](auto& dst, const auto& src) {
        dst.insert(dst.end(), src.data(), src.data() + src.size());
    };
    auto append_padded = [](auto& dst, const auto& src, int base_size, const auto default_value) {
        if (!dst.empty() && !src.empty()) {
            dst.insert(dst.end(), src.cdata(), src.cdata() + src.size());
        }
        else if (!dst.empty() && src.empty()) {
            dst.resize(dst.size() + src.size(), default_value);
        }
        else if (dst.empty() && !src.empty()) {
            dst.resize(base_size, default_value);
            dst.insert(dst.end(), src.cdata(), src.cdata() + src.size());
        }
    };

    int vertex_offset = (int)points.size();
    int index_offset = (int)indices.size();
    int face_offset = (int)counts.size();

    // handle vertex data
    append(points, v.points);
    append(counts, v.counts);
    append(indices, v.indices);
    if (vertex_offset > 0)
        add(indices.data() + index_offset, v.indices.size(), vertex_offset);

    append_padded(normals, v.normals, index_offset, float3::zero());
    append_padded(uvs,     v.uvs,     index_offset, float2::zero());
    append_padded(colors,  v.colors,  index_offset, float4::one());

    // remove skinning data for now
    joints_per_vertex = 0;
    joint_indices.clear();
    joint_weights.clear();
    bind_transform = float4x4::identity();
    skeleton = nullptr;
    joints.clear();
    joint_matrices.clear();

    // handle materials & facesets
    append(materials, v.materials);
    append_padded(material_ids, v.material_ids, face_offset, -1);
    facesets.reserve(facesets.size() + v.facesets.size());
    for (auto& fs : v.facesets) {
        FaceSetPtr dfs;
        for (auto& f : facesets) {
            if (f->material == fs->material) {
                dfs = f;
                break;
            }
        }
        if (dfs) {
            dfs->merge(*fs, face_offset, index_offset);
        }
        else {
            dfs = fs->clone();
            dfs->addOffset(face_offset, index_offset);
            facesets.push_back(dfs);
        }
    }

    // handle transform
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
        for (size_t bsi = 0; bsi < nbs; ++bsi)
            blendshapes[bsi]->apply(dst_points, dst_normals, blendshape_weights[bsi]);
    }

    // skeleton
    if (isSkinned())
        applySkinning(dst_points, dst_normals);

    if (trans != float4x4::identity()) {
        mu::MulPoints(trans, dst_points, dst_points, npoints);
        mu::MulVectors(trans, dst_normals, dst_normals, normals.size());
    }
}

void MeshNode::applySkinning(float3* dst_points, float3* dst_normals)
{
    transform_container(joint_matrices, joints, [this](auto& m, auto j) {
        m = mu::invert(j->bindpose) * j->global_matrix;
    });

    int npoints = (int)points.size();
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

bool MeshNode::isSkinned() const
{
    return joints_per_vertex != 0 &&
        joint_indices.size() == points.size() * joints_per_vertex &&
        joint_weights.size() == points.size() * joints_per_vertex &&
        !joints.empty() && skeleton;
}

int MeshNode::getMaxMaterialID() const
{
    int mid_min = 0;
    int mid_max = 0;
    mu::MinMax(material_ids.cdata(), material_ids.size(), mid_min, mid_max);
    return mid_max;
}



sgRegisterType(BlendshapeTarget);

#define EachMember(F)\
    F(point_offsets) F(normal_offsets) F(weight)

void BlendshapeTarget::serialize(serializer& s) const
{
    EachMember(sgWrite)
}

void BlendshapeTarget::deserialize(deserializer& d)
{
    EachMember(sgRead)
}
#undef EachMember


sgRegisterType(BlendshapeNode);

#define EachMember(F)\
    F(indices) F(targets)

void BlendshapeNode::serialize(serializer& s) const
{
    super::serialize(s);
    EachMember(sgWrite)
}

void BlendshapeNode::deserialize(deserializer& d)
{
    super::deserialize(d);
    EachMember(sgRead)
}
#undef EachMember

BlendshapeNode::BlendshapeNode() {}

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
    targets.clear();
}

void BlendshapeNode::makeMesh(MeshNode& dst, const MeshNode& base, float weight)
{
    dst.points = base.points;
    dst.normals = base.normals;
    dst.uvs = base.uvs;
    dst.colors = base.colors;
    dst.material_ids = base.material_ids;
    dst.counts = base.counts;
    dst.indices = base.indices;

    apply(dst.points.data(), dst.normals.data(), weight);
}

BlendshapeTarget* BlendshapeNode::addTarget(float weight)
{
    for (auto& t : targets)
        if (t->weight == weight)
            return t.get();

    auto ret = new BlendshapeTarget();
    ret->weight = weight;
    targets.push_back(BlendshapeTargetPtr(ret));
    std::sort(targets.begin(), targets.end(), [](auto& a, auto& b) { return a->weight < b->weight; });
    return ret;
}

BlendshapeTarget* BlendshapeNode::addTarget(const MeshNode& target, const MeshNode& base, float weight)
{
    auto& dst = *addTarget(weight);
    if (!base.points.empty() && base.points.size() == target.points.size()) {
        size_t n = base.points.size();
        dst.point_offsets.resize_discard(n);
        auto* b = base.points.cdata();
        auto* t = target.points.cdata();
        auto* d = dst.point_offsets.data();
        for (size_t i = 0; i < n; ++i)
            d[i] = t[i] - b[i];
    }
    if (!base.normals.empty() && base.normals.size() == target.normals.size()) {
        size_t n = base.normals.size();
        dst.normal_offsets.resize_discard(n);
        auto* b = base.normals.cdata();
        auto* t = target.normals.cdata();
        auto* d = dst.normal_offsets.data();
        for (size_t i = 0; i < n; ++i)
            d[i] = t[i] - b[i];
    }
    return &dst;
}

void BlendshapeNode::apply(float3* dst_points, float3* dst_normals, float weight)
{
    if (weight == 0.0f || targets.empty())
        return;

    // https://graphics.pixar.com/usd/docs/api/_usd_skel__schemas.html#UsdSkel_BlendShape

    BlendshapeTarget* prev = nullptr;
    BlendshapeTarget* next = nullptr;
    for (auto& t : targets) {
        if (weight <= t->weight) {
            next = t.get();
            break;
        }
        else {
            prev = t.get();
        }
    }

    // weight:0 is treated as 'implicit target'
    if (weight > 0.0f) {
        if (prev && prev->weight < 0.0f)
            prev = nullptr;
        if (next && next->weight < 0.0f)
            next = nullptr;
    }
    else if (weight < 0.0f) {
        if (prev && prev->weight > 0.0f)
            prev = nullptr;
        if (next && next->weight > 0.0f)
            next = nullptr;
    }

    auto apply1 = [this, dst_points, dst_normals](BlendshapeTarget *t, float w) {
        const float3* po = t->point_offsets.cdata();
        if (!indices.empty()) {
            size_t nbsi = indices.size();
            const int* i = indices.cdata();
            for (size_t oi = 0; oi < nbsi; ++oi)
                dst_points[i[oi]] += po[oi] * w;
        }
        else {
            size_t npoints = t->point_offsets.size();
            for (size_t oi = 0; oi < npoints; ++oi)
                dst_points[oi] += po[oi] * w;
        }
        // todo: handle normal offsets
    };

    auto apply2 = [this, dst_points, dst_normals](BlendshapeTarget* t1, BlendshapeTarget* t2, float w) {
        const float3* po1 = t1->point_offsets.cdata();
        const float3* po2 = t2->point_offsets.cdata();
        if (!indices.empty()) {
            size_t nbsi = indices.size();
            const int* i = indices.cdata();
            for (size_t oi = 0; oi < nbsi; ++oi)
                dst_points[i[oi]] += mu::lerp(po1[oi], po2[oi], w);
        }
        else {
            size_t npoints = t1->point_offsets.size();
            for (size_t oi = 0; oi < npoints; ++oi)
                dst_points[oi] += mu::lerp(po1[oi], po2[oi], w);
        }
        // todo: handle normal offsets
    };

    if (next && !prev) {
        apply1(next, weight / next->weight);
    }
    else if (!next && prev) {
        apply1(prev, weight / prev->weight);
    }
    else if (next && prev) {
        float w = (weight - prev->weight) / (next->weight - prev->weight);
        apply2(prev, next, w);
    }
}


sgRegisterType(SkelRootNode);

#define EachMember(F)\
    F(skeleton)

void SkelRootNode::serialize(serializer& s) const
{
    super::serialize(s);
    EachMember(sgWrite)
}

void SkelRootNode::deserialize(deserializer& d)
{
    super::deserialize(d);
    EachMember(sgRead)
}
#undef EachMember

SkelRootNode::SkelRootNode() {}

SkelRootNode::SkelRootNode(Node* parent, const char* name)
    : super(parent, name)
{
}

Node::Type SkelRootNode::getType() const
{
    return Type::SkelRoot;
}


sgRegisterType(Joint);

#define EachMember(F)\
    F(path) F(index) F(bindpose) F(restpose) F(local_matrix) F(global_matrix)\
    F(skeleton) F(parent) F(children)

void Joint::serialize(serializer& s) const
{
    EachMember(sgWrite)
}

void Joint::deserialize(deserializer& d)
{
    EachMember(sgRead)
}
#undef EachMember

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


sgRegisterType(SkeletonNode);

#define EachMember(F)\
    F(joints)

void SkeletonNode::serialize(serializer& s) const
{
    super::serialize(s);
    EachMember(sgWrite)
}

void SkeletonNode::deserialize(deserializer& d)
{
    super::deserialize(d);
    EachMember(sgRead)
}
#undef EachMember

SkeletonNode::SkeletonNode() {}

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

Joint* SkeletonNode::addJoint(const std::string& jpath)
{
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


sgRegisterType(InstancerNode);

#define EachMember(F)\
    F(protos) F(proto_indices) F(matrices)

void InstancerNode::serialize(serializer& s) const
{
    super::serialize(s);
    EachMember(sgWrite)
}

void InstancerNode::deserialize(deserializer& d)
{
    super::deserialize(d);
    EachMember(sgRead)
}
#undef EachMember

InstancerNode::InstancerNode() {}

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


sgRegisterType(Texture);

const float4 Texture::default_fallback = { 0.0f, 0.0f, 0.0f, 1.0f };

#define EachMember(F)\
    F(file_path) F(st) F(wrap_s) F(wrap_t) F(fallback)

void Texture::serialize(serializer& s) const
{
    EachMember(sgWrite)
}

void Texture::deserialize(deserializer& d)
{
    EachMember(sgRead)
}
#undef EachMember

Texture::operator bool() const
{
    return !file_path.empty();
}


sgRegisterType(MaterialNode);

#define EachMember(F)\
    F(index) F(shader_type) F(use_vertex_color) F(double_sided)\
    F(diffuse_color) F(diffuse) F(opacity) F(roughness)\
    F(ambient_color) F(specular_color) F(emissive_color)\
    F(diffuse_texture) F(opacity_texture) F(bump_texture)

void MaterialNode::serialize(serializer& s) const
{
    super::serialize(s);
    EachMember(sgWrite)
}

void MaterialNode::deserialize(deserializer& d)
{
    super::deserialize(d);
    EachMember(sgRead)
}
#undef EachMember

MaterialNode::MaterialNode() {}

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



sgRegisterType(Scene);

static thread_local Scene* g_current_scene;
static const char sgMagic[] = "sg" sgVersionString;


Scene* Scene::getCurrent()
{
    return g_current_scene;
}

#define EachMember(F)\
    F(path) F(nodes) F(root_node) F(up_axis)\
    F(frame_count) F(frame_rate) F(time_start) F(time_end) F(time_current)

void Scene::serialize(serializer& s) const
{
    // magic code
    sg::write_array(s, sgMagic, 6);

    // add pointer record to make this resolvable
    hptr handle = s.getHandle(this);
    sg::write(s, handle);

    EachMember(sgWrite)
}

bool Scene::deserialize(deserializer& d)
{
    // check magic code
    char magic[6];
    sg::read_array(d, magic, 6);
    if (std::strcmp(magic, sgMagic) != 0)
        return false;

    hptr handle;
    sg::read(d, handle);
    d.setPointer(handle, this);

    EachMember(sgRead)

    if (impl) {
        for (auto& n : nodes)
            impl->wrapNode(n.get());
    }
    return true;
}
#undef EachMember

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

bool Scene::isNodeTypeSupported(Node::Type type) const
{
    if (impl)
        return impl->isNodeTypeSupported(type);
    return true;
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

double Scene::frameToTime(int frame)
{
    if (impl)
        return impl->frameToTime(frame);
    return (1.0 / frame_rate) * frame + time_start;
}

void Scene::registerNode(Node* n)
{
    if (n) {
        n->scene = this;
        n->id = (uint32_t)nodes.size();
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
        Case(Unknown, Node);
#undef Case
    case Node::Type::Root: ret = new RootNode(); break;
    default: break;
    }
    return ret;
}

} // namespace sg
