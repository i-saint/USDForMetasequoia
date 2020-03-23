#pragma once
#include "mqusd.h"
#include "mqusdSerialization.h"

#ifndef PXR_USD_USD_PRIM_H
class UsdPrim;
#endif

namespace mqusd {

extern double default_time;

class Node;
class RootNode;
class XformNode;
class MeshNode;
class BlendshapeNode;
class SkelRootNode;
class SkeletonNode;
class MaterialNode;
class Scene;

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
        SkelRoot,
        Skeleton,
        Material,
        Scope,
    };
    static std::shared_ptr<Node> create(std::istream& is);

    Node(Node* parent, const char *name);
    virtual ~Node();
    virtual Type getType() const;
    virtual void serialize(std::ostream& os);
    virtual void deserialize(std::istream& is);
    virtual void resolve();
    virtual void convert(const ConvertOptions& opt);

    std::string getName() const;
    const std::string& getPath() const;
    template<class NodeT> NodeT* findParent();

public:
    // serializable
    std::string path;
    uint32_t id = 0;

    // non-serializable
    Scene* scene = nullptr;
    Node* parent = nullptr;
    std::vector<Node*> children;

    void* impl = nullptr;
    void* userdata = nullptr;
};
mqusdSerializable(Node);
mqusdDeclPtr(Node);


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

    XformNode(Node* parent = nullptr, const char* name = nullptr);
    Type getType() const override;
    void serialize(std::ostream& os) override;
    void deserialize(std::istream& is) override;
    void resolve() override;
    void convert(const ConvertOptions& opt) override;

    std::tuple<float3, quatf, float3> getLocalTRS() const;
    std::tuple<float3, quatf, float3> getGlobalTRS() const;
    void setLocalTRS(const float3& t, const quatf& r, const float3& s);
    void setGlobalTRS(const float3& t, const quatf& r, const float3& s);

public:
    // serializable
    float4x4 local_matrix = float4x4::identity();
    float4x4 global_matrix = float4x4::identity();

    // non-serializable
    XformNode* parent_xform = nullptr;
};


class BlendshapeNode : public Node
{
using super = Node;
public:
    static const Type node_type = Type::Blendshape;

    BlendshapeNode(Node* parent = nullptr, const char* name = nullptr);
    Type getType() const override;
    void serialize(std::ostream& os) override;
    void deserialize(std::istream& is) override;
    void convert(const ConvertOptions& opt) override;

    void clear();
    void makeMesh(MeshNode& dst, const MeshNode& base);
    void makeOffsets(const MeshNode& target, const MeshNode& base);

public:
    // serializable
    SharedVector<int> indices;
    SharedVector<float3> point_offsets;
    SharedVector<float3> normal_offsets;
};


class SkelRootNode : public XformNode
{
using super = XformNode;
public:
    static const Type node_type = Type::SkelRoot;

    SkelRootNode(Node* parent = nullptr, const char* name = nullptr);
    Type getType() const override;
    void serialize(std::ostream& os) override;
    void deserialize(std::istream& is) override;
    void resolve() override;

public:
    // serializable
    std::string skeleton_path;

    // non-serializable
    SkeletonNode* skeleton = nullptr;
};


class Joint
{
public:
    static std::shared_ptr<Joint> create(std::istream& is);

    Joint();
    Joint(SkeletonNode* skel, const std::string& path);
    void serialize(std::ostream& os);
    void deserialize(std::istream& is);
    void resolve();

    std::string getName()const;
    std::tuple<float3, quatf, float3> getLocalTRS() const;
    std::tuple<float3, quatf, float3> getGlobalTRS() const;
    void setLocalTRS(const float3& t, const quatf& r, const float3& s);
    void setGlobalTRS(const float3& t, const quatf& r, const float3& s);

public:
    // serializable
    std::string path;
    int index = 0;
    float4x4 bindpose = float4x4::identity(); // world space
    float4x4 restpose = float4x4::identity(); // default pose. local space
    float4x4 local_matrix = float4x4::identity();  // for animation
    float4x4 global_matrix = float4x4::identity(); // 

    // non-serializable
    SkeletonNode* skeleton = nullptr;
    Joint* parent = nullptr;
    std::vector<Joint*> children;
    void* userdata = nullptr;
};
mqusdSerializable(Joint);
mqusdDeclPtr(Joint);

class SkeletonNode : public XformNode
{
using super = XformNode;
public:
    static const Type node_type = Type::Skeleton;

    SkeletonNode(Node* parent = nullptr, const char* name = nullptr);
    Type getType() const override;
    void serialize(std::ostream& os) override;
    void deserialize(std::istream& is) override;
    void resolve() override;
    void convert(const ConvertOptions& opt) override;

    void clear();
    Joint* addJoint(const std::string& path);
    void updateGlobalMatrices(const float4x4& base);

    Joint* findJointByPath(const std::string& path);

public:
    // serializable
    std::vector<JointPtr> joints;
};


class MeshNode : public XformNode
{
using super = XformNode;
public:
    static const Type node_type = Type::Mesh;

    MeshNode(Node* parent = nullptr, const char* name = nullptr);
    Type getType() const override;
    void serialize(std::ostream& os);
    void deserialize(std::istream& is);
    void resolve() override;
    void convert(const ConvertOptions& opt) override;
    void applyTransform(const float4x4& v);
    void toWorldSpace();
    void toLocalSpace();

    void clear();
    void merge(const MeshNode& other);
    void validate();

    int getMaxMaterialID() const;
    MeshNode* findParentMesh() const;

public:
    // serializable
    SharedVector<float3> points;    // per-vertex
    SharedVector<float3> normals;   // per-index
    SharedVector<float2> uvs;       // 
    SharedVector<float4> colors;    // 
    SharedVector<int> material_ids; // per-face
    SharedVector<int> counts;       // 
    SharedVector<int> indices;

    int joints_per_vertex = 0;
    SharedVector<int> joint_indices;   // size must be points.size() * joints_per_vertex
    SharedVector<float> joint_weights; // 
    float4x4 bind_transform = float4x4::identity();

    std::vector<std::string> blendshape_paths;
    std::string skeleton_path;
    std::vector<std::string> joint_paths; // paths to joints in skeleton

    // non-serializable
    std::vector<BlendshapeNode*> blendshapes;
    SkeletonNode* skeleton = nullptr;
};


class MaterialNode : public Node
{
using super = Node;
public:
    static const Type node_type = Type::Material;

    MaterialNode(Node* parent = nullptr, const char* name = nullptr);
    Type getType() const override;
    void serialize(std::ostream& os) override;
    void deserialize(std::istream& is) override;
    virtual bool valid() const;

public:
    // serializable
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


enum class UpAxis
{
    Unknown,
    Y,
    Z,
};

class Scene
{
public:
    static std::shared_ptr<Scene> create(std::istream& is);
    static Scene* getCurrent();

    Scene();
    ~Scene();
    void serialize(std::ostream& os);
    void deserialize(std::istream& is);

    bool open(const char* path);
    bool create(const char* path);
    bool save();
    void close();
    void read(double time);
    void write(double time);

    Node* findNodeByID(uint32_t id);
    Node* findNodeByPath(const std::string& path);
    Node* createNode(Node *parent, const char *name, Node::Type type);

    // internal
    Node* createNodeImpl(Node* parent, const char* name, Node::Type type);

private:
    void classifyNode(Node *node);

public:
    // serializable
    std::string path;
    std::vector<NodePtr> nodes;
    UpAxis up_axis = UpAxis::Unknown;
    double time_start = 0.0;
    double time_end = 0.0;
    double time_current = default_time;

    // non-serializable
    SceneInterfacePtr impl;
    RootNode* root_node = nullptr;
    std::vector<MeshNode*> mesh_nodes;
    std::vector<SkeletonNode*> skeleton_nodes;
    std::vector<MaterialNode*> material_nodes;
};
mqusdSerializable(Scene);
mqusdDeclPtr(Scene);



class SceneInterface
{
public:
    virtual ~SceneInterface();
    virtual bool open(const char* path) = 0;
    virtual bool create(const char* path) = 0;
    virtual bool save() = 0;
    virtual void close() = 0;
    virtual void read() = 0;
    virtual void write() = 0;

    virtual Node* createNode(Node* parent, const char* name, Node::Type type) = 0;
    virtual bool wrapNode(Node* node) = 0;
};



inline float3 to_float3(const MQColor& v) { return (float3&)v; }
inline float3 to_float3(const MQPoint& v) { return (float3&)v; }
inline float4x4 to_float4x4(const MQMatrix& v) { return (float4x4&)v; }

template<class T>
inline bool is_uniform(const T* data, size_t data_size, size_t element_size)
{
    auto* base = data;
    size_t n = data_size / element_size;
    for (size_t di = 1; di < n; ++di) {
        data += element_size;
        for (size_t ei = 0; ei < element_size; ++ei) {
            if (data[ei] != base[ei])
                return false;
        }
    }
    return true;
}
template<class Container, class Body>
inline bool is_uniform(const Container& container, const Body& body)
{
    for (auto& e : container) {
        if (!body(e))
            return false;
    }
    return true;
}

template<class DstContainer, class SrcContainer, class Convert>
inline void transform_container(DstContainer& dst, const SrcContainer& src, const Convert& c)
{
    size_t n = src.size();
    dst.resize(src.size());
    for (size_t i = 0; i < n; ++i)
        c(dst[i], src[i]);
}

std::string SanitizeNodeName(const std::string& name);
std::string SanitizeNodePath(const std::string& path);
std::string GetParentPath(const std::string& path);
std::string GetLeafName(const std::string& path);


} // namespace mqusd
