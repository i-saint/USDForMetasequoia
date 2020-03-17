#pragma once
#include "mqusd.h"

#ifndef PXR_USD_USD_PRIM_H
class UsdPrim;
#endif

namespace mqusd {

struct mqusdPlayerSettings;
class SceneInterface;
using SceneInterfacePtr = std::shared_ptr<SceneInterface>;

struct ConvertOptions
{
    float scale_factor = 1.0f;
    bool flip_x = false;
    bool flip_yz = false;
    bool flip_faces = false;
};


class Node
{
public:
    enum class Type
    {
        Unknown,
        Root,
        Xform,
        Mesh,
        Blendshape,
        Skeleton,
        SkelAnimation,
        Material,
        Scope,
    };

    Node(Node* parent);
    virtual ~Node();
    virtual Type getType() const;
    virtual void convert(const ConvertOptions& opt);

    template<class NodeT> NodeT* findParent();
    std::string getPath() const;

    void* impl = nullptr;
    Node* parent = nullptr;
    std::vector<Node*> children;
    std::string name;
    uint32_t id = 0;
};
using NodePtr = std::shared_ptr<Node>;


class RootNode : public Node
{
using super = Node;
public:
    static const Type node_type = Type::Root;

    RootNode();
    Type getType() const override;
};


class XformNode : public Node
{
using super = Node;
public:
    static const Type node_type = Type::Xform;

    XformNode(Node* parent);
    Type getType() const override;
    void convert(const ConvertOptions& opt) override;

    std::tuple<float3, quatf, float3> getLocalTRS() const;
    std::tuple<float3, quatf, float3> getGlobalTRS() const;
    void setLocalTRS(const float3& t, const quatf& r, const float3& s);
    void setGlobalTRS(const float3& t, const quatf& r, const float3& s);

public:
    XformNode* parent_xform = nullptr;
    float4x4 local_matrix = float4x4::identity();
    float4x4 global_matrix = float4x4::identity();
};


class BlendshapeNode : public Node
{
using super = Node;
public:
    static const Type node_type = Type::Blendshape;

    BlendshapeNode(Node* parent);
    Type getType() const override;
    void convert(const ConvertOptions& opt) override;

    void clear();

public:
    SharedVector<int> indices;
    SharedVector<float3> point_offsets;
    SharedVector<float3> normal_offsets;
};


class SkeletonNode : public XformNode
{
using super = XformNode;
public:
    class Joint
    {
    public:
        Joint(const std::string& path);
        std::tuple<float3, quatf, float3> getLocalTRS() const;
        std::tuple<float3, quatf, float3> getGlobalTRS() const;
        void setLocalTRS(const float3& t, const quatf& r, const float3& s);
        void setGlobalTRS(const float3& t, const quatf& r, const float3& s);

    public:
        Joint* parent = nullptr;
        std::vector<Joint*> children;
        std::string name;
        std::string path;
        int index = 0;
        float4x4 bindpose = float4x4::identity();
        float4x4 restpose = float4x4::identity();
        float4x4 local_matrix = float4x4::identity();
        float4x4 global_matrix = float4x4::identity();
    };
    using JointPtr = std::shared_ptr<Joint>;

public:
    static const Type node_type = Type::Skeleton;

    SkeletonNode(Node* parent = nullptr);
    Type getType() const override;
    void convert(const ConvertOptions& opt) override;

    void clear();
    Joint* addJoint(const std::string& path);
    void updateGlobalMatrices(const float4x4& base);

    Joint* findJointByName(const std::string& name);
    Joint* findJointByPath(const std::string& path);

public:
    std::vector<JointPtr> joints;
};


class MeshNode : public XformNode
{
using super = XformNode;
public:
    static const Type node_type = Type::Mesh;

    MeshNode(Node* parent = nullptr);
    Type getType() const override;
    void convert(const ConvertOptions& opt) override;
    void applyTransform(const float4x4& v);
    void toWorldSpace();
    void toLocalSpace();

    void clear();
    void merge(const MeshNode& other);
    void validate();

    int getMaxMaterialID() const;

public:
    SharedVector<float3> points;    // per-vertex
    SharedVector<float3> normals;   // per-index
    SharedVector<float2> uvs;       // 
    SharedVector<float4> colors;    // 
    SharedVector<int> material_ids; // per-face
    SharedVector<int> counts;       // 
    SharedVector<int> indices;

    // blendshape
    std::vector<BlendshapeNode*> blendshapes;

    // skinning
    SkeletonNode* skeleton = nullptr;
    std::vector<std::string> joints;// paths to joints in skeleton
    int joints_per_vertex = 0;
    SharedVector<int> joint_indices;   // size must be points.size() * joints_per_vertex
    SharedVector<float> joint_weights; // 
    float4x4 bind_transform = float4x4::identity();
};


class MaterialNode : public Node
{
using super = Node;
public:
    static const Type node_type = Type::Material;

    MaterialNode(Node* parent = nullptr);
    Type getType() const override;
    virtual bool valid() const;

public:
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


class Scene
{
public:
    Scene();
    virtual ~Scene();
    bool open(const char* path);
    bool create(const char* path);
    bool save();
    void close();
    void read(double time);
    void write(double time) const;

    Node* createNode(Node *parent, const char *name, Node::Type type);

public:
    SceneInterfacePtr impl;
    std::string path;
    std::vector<NodePtr> nodes;
    RootNode* root_node = nullptr;
    std::vector<MeshNode*> mesh_nodes;
    std::vector<MaterialNode*> material_nodes;
    double time_start = 0.0;
    double time_end = 0.0;
};
using ScenePtr = std::shared_ptr<Scene>;

ScenePtr CreateUSDScene();


class SceneInterface
{
public:
    virtual ~SceneInterface();
    virtual bool open(const char* path) = 0;
    virtual bool create(const char* path) = 0;
    virtual bool save() = 0;
    virtual void close() = 0;
    virtual void read(double time) = 0;
    virtual void write(double time) const = 0;

    virtual Node* createNode(Node* parent, const char* name, Node::Type type) = 0;
};

inline float3 to_float3(MQColor v) { return (float3&)v; };

} // namespace mqusd
