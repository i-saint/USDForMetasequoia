#pragma once
#include "mqusd.h"

class Joint
{
public:
    Joint(const std::string& path);

public:
    Joint* parent = nullptr;
    std::vector<Joint*> children;
    std::string name;
    std::string path;
    float4x4 bindpose = float4x4::identity();
    float4x4 restpose = float4x4::identity();
    float4x4 local_matrix = float4x4::identity();
    float4x4 global_matrix = float4x4::identity();
};
using JointPtr = std::shared_ptr<Joint>;


class Skeleton
{
public:
    void clear();
    Joint* makeJoint(const std::string& path);
    void buildJointRelations();

    Joint* findJointByName(const std::string& name);
    Joint* findJointByPath(const std::string& path);

public:
    std::string name;
    std::vector<JointPtr> joints;
};
using SkeletonPtr = std::shared_ptr<Skeleton>;


class Blendshape
{
public:
    void clear();

public:
    std::string name;
    RawVector<int> indices;
    RawVector<float3> point_offsets;
    RawVector<float3> normal_offsets;
};
using BlendshapePtr = std::shared_ptr<Blendshape>;


class Mesh
{
public:
    void clear();
    void resize(int npoints, int nindices, int nfaces);
    void merge(const Mesh& other);
    void clearInvalidComponent();

    void applyScale(float v);
    void applyTransform(const float4x4 v);
    void flipX();
    void flipYZ();
    void flipFaces();

    int getMaxMaterialID() const;

public:
    std::string name;
    RawVector<float3> points;    // per-vertex
    RawVector<float3> normals;   // per-index
    RawVector<float2> uvs;       // 
    RawVector<float4> colors;    // 
    RawVector<int> material_ids; // per-face
    RawVector<int> counts;       // 
    RawVector<int> indices;

    std::vector<BlendshapePtr> blendshapes;
    SkeletonPtr skeleton;
};
using MeshPtr = std::shared_ptr<Mesh>;


class Material
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
using MaterialPtr = std::shared_ptr<Material>;

inline float3 to_float3(MQColor v) { return (float3&)v; };
