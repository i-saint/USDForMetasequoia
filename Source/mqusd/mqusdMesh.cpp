#include "pch.h"
#include "mqusdMesh.h"

namespace mqusd {

Joint::Joint(const std::string& p)
    : path(p)
{
    auto pos = path.find_last_of('/');
    if (pos == std::string::npos)
        name = path;
    else
        name = std::string(path.begin() + pos + 1, path.end());
}


void Skeleton::clear()
{
    name.clear();
    joints.clear();
}

Joint* Skeleton::addJoint(const std::string& path)
{
    auto ret = new Joint(path);
    joints.push_back(JointPtr(ret));
    return ret;
}

void Skeleton::buildJointRelations()
{
    for (auto& joint : joints) {
        auto& path = joint->path;
        auto pos = path.find_last_of('/');
        if (pos != std::string::npos) {
            auto parent_path = std::string(path.begin(), path.begin() + pos);
            if (auto parent = findJointByPath(parent_path)) {
                joint->parent = parent;
                parent->children.push_back(joint.get());
            }
        }
    }
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

void Skeleton::updateGlobalMatrices(const float4x4& base)
{
    // update global matrices from top to down recursively
    for (auto& joint : joints) {
        if (!joint->parent)
            UpdateGlobalMatricesImpl(*joint, base);
    }
}

Joint* Skeleton::findJointByName(const std::string& name)
{
    auto it = std::find_if(joints.begin(), joints.end(),
        [&name](auto& joint) { return joint->name == name; });
    return it == joints.end() ? nullptr : (*it).get();
}

Joint* Skeleton::findJointByPath(const std::string& path)
{
    auto it = std::find_if(joints.begin(), joints.end(),
        [&path](auto& joint) { return joint->path == path; });
    return it == joints.end() ? nullptr : (*it).get();
}


void Blendshape::clear()
{
    name.clear();
    indices.clear();
    point_offsets.clear();
    normal_offsets.clear();
}



void Mesh::clear()
{
    points.clear();
    normals.clear();
    uvs.clear();
    colors.clear();
    material_ids.clear();
    counts.clear();
    indices.clear();

    blendshapes.clear();

    skeleton = {};
    joints.clear();
    joints_per_vertex = 0;
    joint_indices.clear();
    joint_weights.clear();
    bind_transform = float4x4::identity();
}

void Mesh::resize(int npoints, int nindices, int nfaces)
{
    points.resize_discard(npoints);
    normals.resize_discard(nindices);
    uvs.resize_discard(nindices);
    colors.resize_discard(nindices);
    material_ids.resize_discard(nfaces);
    counts.resize_discard(nfaces);
    indices.resize_discard(nindices);
}

void Mesh::merge(const Mesh& v)
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

void Mesh::clearInvalidComponent()
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

void Mesh::applyScale(float v)
{
    if (v != 1.0f)
        mu::Scale(points.data(), v, points.size());
}

void Mesh::applyTransform(const float4x4 v)
{
    if (v == float4x4::identity())
        return;
    mu::MulPoints(v, points.cdata(), points.data(), points.size());
    mu::MulVectors(v, normals.cdata(), normals.data(), normals.size());
}

void Mesh::flipX()
{
    mu::InvertX(points.data(), points.size());
    mu::InvertX(normals.data(), normals.size());
}

void Mesh::flipYZ()
{
    auto convert = [this](auto& v) { return flip_z(swap_yz(v)); };

    for (auto& v : points) v = convert(v);
    for (auto& v : normals) v = convert(v);
}

void Mesh::flipFaces()
{
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

int Mesh::getMaxMaterialID() const
{
    int mid_min = 0;
    int mid_max = 0;
    mu::MinMax(material_ids.cdata(), material_ids.size(), mid_min, mid_max);
    return mid_max;
}

} // namespace mqusd
