#include "pch.h"
#include "mqusdMesh.h"

void mqusdMesh::clear()
{
    points.clear();
    normals.clear();
    uvs.clear();
    colors.clear();
    material_ids.clear();
    counts.clear();
    indices.clear();
}

void mqusdMesh::resize(int npoints, int nindices, int nfaces)
{
    points.resize_discard(npoints);
    normals.resize_discard(nindices);
    uvs.resize_discard(nindices);
    colors.resize_discard(nindices);
    material_ids.resize_discard(nfaces);
    counts.resize_discard(nfaces);
    indices.resize_discard(nindices);
}

void mqusdMesh::merge(const mqusdMesh& v)
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

void mqusdMesh::clearInvalidComponent()
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

void mqusdMesh::applyScale(float v)
{
    if (v != 1.0f)
        mu::Scale(points.data(), v, points.size());
}

void mqusdMesh::applyTransform(const float4x4 v)
{
    if (v == float4x4::identity())
        return;
    mu::MulPoints(v, points.cdata(), points.data(), points.size());
    mu::MulVectors(v, normals.cdata(), normals.data(), normals.size());
}

void mqusdMesh::flipX()
{
    mu::InvertX(points.data(), points.size());
    mu::InvertX(normals.data(), normals.size());
}

void mqusdMesh::flipYZ()
{
    auto convert = [this](auto& v) { return flip_z(swap_yz(v)); };

    for (auto& v : points) v = convert(v);
    for (auto& v : normals) v = convert(v);
}

void mqusdMesh::flipFaces()
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

int mqusdMesh::getMaxMaterialID() const
{
    int mid_min = 0;
    int mid_max = 0;
    mu::MinMax(material_ids.cdata(), material_ids.size(), mid_min, mid_max);
    return mid_max;
}
