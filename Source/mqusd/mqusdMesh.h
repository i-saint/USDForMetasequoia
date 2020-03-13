#pragma once
#include "mqusd.h"


class mqusdMesh
{
public:
    std::string name;
    RawVector<float3> points;    // per-vertex
    RawVector<float3> normals;   // per-index
    RawVector<float2> uvs;       // 
    RawVector<float4> colors;    // 
    RawVector<int> material_ids; // per-face
    RawVector<int> counts;       // 
    RawVector<int> indices;

    void clear();
    void resize(int npoints, int nindices, int nfaces);
    void merge(const mqusdMesh& other);
    void clearInvalidComponent();

    void applyScale(float v);
    void applyTransform(const float4x4 v);
    void flipX();
    void flipYZ();
    void flipFaces();

    int getMaxMaterialID() const;
};

class mqusdMaterial
{
public:
    std::string name;
    int shader = MQMATERIAL_SHADER_CLASSIC;
    bool use_vertex_color = false;
    bool double_sided = false;
    float3 color = float3::one();
    float diffuse = 1.0f;
    float alpha = 1.0f;
    float3 ambient_color = float3::zero();
    float3 specular_color = float3::one();
    float3 emission_color = float3::zero();
};

inline float3 to_float3(MQColor v) { return (float3&)v; };
