#pragma once

#define sgVersion       100
#define sgVersionString "100"

#include "MeshUtils/MeshUtils.h"
#include "sgSerialization.h"

namespace sg {

using mu::float2;
using mu::float3;
using mu::float4;
using mu::quatf;
using mu::float2x2;
using mu::float3x3;
using mu::float4x4;
using mu::double4x4;

class Node;
class RootNode;
class XformNode;
class MeshNode;
class BlendshapeNode;
class SkelRootNode;
class SkeletonNode;
class MaterialNode;

class SceneInterface;
class Scene;

extern const double default_time;
bool IsDefaultTime(double t);


struct ConvertOptions
{
    float scale_factor = 1.0f;
    bool flip_v = false;
    bool flip_x = false;
    bool flip_yz = false;
    bool flip_faces = false;

    bool operator==(const ConvertOptions& v) const;
    bool operator!=(const ConvertOptions& v) const;
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
        Instancer,
        Material,
        Scope,
    };
    static const Type node_type = Type::Unknown;

    Node();
    Node(Node* parent, const char *name);
    virtual ~Node();
    virtual Type getType() const;
    virtual void serialize(serializer& s) const;
    virtual void deserialize(deserializer& d);
    virtual void convert(const ConvertOptions& opt);

    std::string getName() const;
    std::string getDisplayName() const;
    const std::string& getPath() const;

    template<class NodeT>
    NodeT* cast() { return dynamic_cast<NodeT*>(this); }

    template<class Body>
    void eachChild(const Body& body)
    {
        for (auto c : children)
            body(c);
    }

    template<class Body>
    void eachChildR(const Body& body)
    {
        for (auto c : children) {
            body(c);
            c->eachChildR(body);
        }
    }

    template<class NodeT>
    NodeT* findParent()
    {
        for (Node* p = parent; p != nullptr; p = p->parent) {
            if (auto tp = dynamic_cast<NodeT*>(p))
                return tp;
        }
        return nullptr;
    }

public:
    // serializable
    std::string path;
    std::string display_name;
    uint32_t id = ~0u;

    Scene* scene = nullptr;
    Node* parent = nullptr;
    std::vector<Node*> children;

    // non-serializable
    void* impl = nullptr;
    void* userdata = nullptr;
    bool removed = false;
};
sgSerializable(Node);
sgDeclPtr(Node);


class RootNode : public Node
{
using super = Node;
public:
    static const Type node_type = Type::Root;

    RootNode();
    Type getType() const override;
};
sgSerializable(RootNode);


class XformNode : public Node
{
using super = Node;
public:
    static const Type node_type = Type::Xform;

    XformNode();
    XformNode(Node* parent, const char* name);
    Type getType() const override;
    void serialize(serializer& s) const override;
    void deserialize(deserializer& d) override;
    void convert(const ConvertOptions& opt) override;

    std::tuple<float3, quatf, float3> getLocalTRS() const;
    std::tuple<float3, quatf, float3> getGlobalTRS() const;
    void setLocalTRS(const float3& t, const quatf& r, const float3& s);
    void setGlobalTRS(const float3& t, const quatf& r, const float3& s);

public:
    // serializable
    bool visibility = true;
    float4x4 local_matrix = float4x4::identity();
    float4x4 global_matrix = float4x4::identity();
    XformNode* parent_xform = nullptr;
};
sgSerializable(XformNode);


class BlendshapeTarget
{
public:
    SharedVector<float3> point_offsets;
    SharedVector<float3> normal_offsets;
    float weight = 0.0f;

    void serialize(serializer& s) const;
    void deserialize(deserializer& d);
};
sgSerializable(BlendshapeTarget);
sgDeclPtr(BlendshapeTarget);

class BlendshapeNode : public Node
{
using super = Node;
public:
    static const Type node_type = Type::Blendshape;

    BlendshapeNode();
    BlendshapeNode(Node* parent, const char* name);
    Type getType() const override;
    void serialize(serializer& s) const override;
    void deserialize(deserializer& d) override;
    void convert(const ConvertOptions& opt) override;

    void clear();
    void makeMesh(MeshNode& dst, const MeshNode& base, float weight = 1.0f);
    BlendshapeTarget* addTarget(float weight);
    BlendshapeTarget* addTarget(const MeshNode& target, const MeshNode& base, float weight = 1.0f);
    void apply(float3* dst_points, float3* dst_normals, float weight);


public:
    // serializable
    SharedVector<int> indices;
    std::vector<BlendshapeTargetPtr> targets;
};
sgSerializable(BlendshapeNode);


class SkelRootNode : public XformNode
{
using super = XformNode;
public:
    static const Type node_type = Type::SkelRoot;

    SkelRootNode();
    SkelRootNode(Node* parent, const char* name);
    Type getType() const override;
    void serialize(serializer& s) const override;
    void deserialize(deserializer& d) override;

public:
    // serializable
    SkeletonNode* skeleton = nullptr;
};
sgSerializable(SkelRootNode);


class Joint
{
public:
    Joint();
    Joint(SkeletonNode* skel, const std::string& path);
    void serialize(serializer& s) const;
    void deserialize(deserializer& d);

    std::string getName() const;
    std::tuple<float3, quatf, float3> getLocalTRS() const;
    std::tuple<float3, quatf, float3> getGlobalTRS() const;
    void setLocalTRS(const float3& t, const quatf& r, const float3& s);
    void setGlobalTRS(const float3& t, const quatf& r, const float3& s);

public:
    // serializable
    std::string path;
    int index = 0;
    float4x4 bindpose = float4x4::identity(); // world space. *not* inverted
    float4x4 restpose = float4x4::identity(); // default pose. local space
    float4x4 local_matrix = float4x4::identity();  // for animation
    float4x4 global_matrix = float4x4::identity(); // 

    SkeletonNode* skeleton = nullptr;
    Joint* parent = nullptr;
    std::vector<Joint*> children;

    // non-serializable
    void* userdata = nullptr;
};
sgSerializable(Joint);
sgDeclPtr(Joint);

class SkeletonNode : public XformNode
{
using super = XformNode;
public:
    static const Type node_type = Type::Skeleton;

    SkeletonNode();
    SkeletonNode(Node* parent, const char* name);
    Type getType() const override;
    void serialize(serializer& s) const override;
    void deserialize(deserializer& d) override;
    void convert(const ConvertOptions& opt) override;

    void clear();
    Joint* addJoint(const std::string& path);
    void updateGlobalMatrices(const float4x4& base);

    Joint* findJointByPath(const std::string& path);

public:
    // serializable
    std::vector<JointPtr> joints;
};
sgSerializable(SkeletonNode);


class FaceSet
{
public:
    void serialize(serializer& s) const;
    void deserialize(deserializer& d);
    void clear();
    std::shared_ptr<FaceSet> clone();
    void merge(const FaceSet& v, int face_offset, int index_offset);
    void addOffset(int face_offset, int index_offset);

public:
    // serializable
    MaterialNode* material = nullptr;
    SharedVector<int> faces;
    SharedVector<int> counts;
    SharedVector<int> indices;

    // non-serializable
    void* userdata = nullptr;
};
sgSerializable(FaceSet);
sgDeclPtr(FaceSet);

class MeshNode : public XformNode
{
using super = XformNode;
public:
    static const Type node_type = Type::Mesh;

    MeshNode();
    MeshNode(Node* parent, const char* name);
    Type getType() const override;
    void serialize(serializer& s) const override;
    void deserialize(deserializer& d) override;
    void convert(const ConvertOptions& opt) override;

    void applyTransform(const float4x4& v);
    void toWorldSpace();
    void toLocalSpace();

    // material_ids & materials -> facesets
    void buildFaceSets(bool cleanup = true);
    // facesets -> material_ids & materials
    void buildMaterialIDs(bool cleanup = true);

    void clear();
    void merge(const MeshNode& other, const float4x4& trans = float4x4::identity());
    void bake(MeshNode& dst, const float4x4& trans = float4x4::identity());
    void applySkinning(float3* dst_points, float3* dst_normals);
    void validate();

    bool isSkinned() const;
    int getMaxMaterialID() const;

    template<class Body>
    void eachBSTarget(const Body& body)
    {
        for (auto bs : blendshapes) {
            for (auto& t : bs->targets)
                body(*t);
        }
    };

public:
    // serializable
    SharedVector<float3> points;    // per-vertex
    SharedVector<float3> normals;   // per-index
    SharedVector<float2> uvs;       // 
    SharedVector<float4> colors;    // 
    SharedVector<int> material_ids; // per-face
    SharedVector<int> counts;       // 
    SharedVector<int> indices;

    SkeletonNode* skeleton = nullptr;
    std::vector<Joint*> joints;
    int joints_per_vertex = 0;
    SharedVector<int> joint_indices;   // size must be points.size() * joints_per_vertex
    SharedVector<float> joint_weights; // 
    float4x4 bind_transform = float4x4::identity();

    std::vector<BlendshapeNode*> blendshapes;

    std::vector<MaterialNode*> materials;
    std::vector<FaceSetPtr> facesets;

    // non-serializable
    RawVector<float> blendshape_weights;
    RawVector<float4x4> joint_matrices;

};
sgSerializable(MeshNode);
sgDeclPtr(MeshNode);


class InstancerNode : public XformNode
{
using super = XformNode;
public:
    static const Type node_type = Type::Instancer;

    InstancerNode();
    InstancerNode(Node* parent, const char* name);
    Type getType() const override;
    void serialize(serializer& s) const override;
    void deserialize(deserializer& d) override;
    void convert(const ConvertOptions& opt) override;

    void bake(MeshNode& dst, const float4x4& trans = float4x4::identity());

public:
    struct MeshRecord
    {
        MeshNode* mesh = nullptr;
        InstancerNode* instancer = nullptr;
        float4x4 matrix = float4x4::identity();
    };
    struct ProtoRecord
    {
        std::vector<MeshRecord> mesh_records;
        MeshNode merged_mesh;
    };
    void gatherMeshes();
    void gatherMeshes(ProtoRecord& prec, Node* n, float4x4 m);

public:
    // serializable
    std::vector<Node*> protos;
    SharedVector<int> proto_indices;
    SharedVector<float4x4> matrices;

    // non-serializable
    std::vector<ProtoRecord> proto_records;
};
sgSerializable(InstancerNode);


enum class WrapMode : int
{
    Unknown,
    Clamp,
    Repeat,
    Mirror,
    Black,
};

class Texture
{
public:
    static const float4 default_fallback;

    void serialize(serializer& s) const;
    void deserialize(deserializer& d);
    operator bool() const;

public:
    // serializable
    std::string file_path;
    float2 st = float2::zero();
    WrapMode wrap_s = WrapMode::Unknown;
    WrapMode wrap_t = WrapMode::Unknown;
    float4 fallback = Texture::default_fallback;
};
sgSerializable(Texture);
sgDeclPtr(Texture);


enum class ShaderType : int
{
    Unknown,
    MQClassic,
    MQConstant,
    MQLambert,
    MQPhong,
    MQBlinn,
    MQHLSL,
};

class MaterialNode : public Node
{
using super = Node;
public:
    static const Type node_type = Type::Material;

    MaterialNode();
    MaterialNode(Node* parent, const char* name);
    Type getType() const override;
    void serialize(serializer& s) const override;
    void deserialize(deserializer& d) override;
    virtual bool valid() const;

public:
    // serializable
    int index = 0;
    ShaderType shader_type = ShaderType::Unknown;
    bool use_vertex_color = false;
    bool double_sided = false;
    float3 diffuse_color = { 0.18f, 0.18f, 0.18f };
    float diffuse = 1.0f;
    float opacity = 1.0f;
    float roughness = 0.5f;
    float3 ambient_color = float3::zero();
    float3 specular_color = float3::zero();
    float3 emissive_color = float3::zero();

    TexturePtr diffuse_texture;
    TexturePtr opacity_texture;
    TexturePtr bump_texture;
};
sgSerializable(MaterialNode);


enum class UpAxis : int
{
    Unknown,
    Y,
    Z,
};

class SceneInterface
{
public:
    virtual ~SceneInterface() {}
    virtual bool open(const char* path) = 0;
    virtual bool create(const char* path) = 0;
    virtual bool save() = 0;
    virtual void close() = 0;
    virtual void read() = 0;
    virtual void write() = 0;

    virtual bool isNodeTypeSupported(Node::Type type) = 0;
    virtual Node* createNode(Node* parent, const char* name, Node::Type type) = 0;
    virtual bool wrapNode(Node* node) = 0;
};
sgDeclPtr(SceneInterface);

class Scene
{
public:
    static Scene* getCurrent();

    Scene();
    virtual ~Scene();
    void serialize(serializer& s) const;
    bool deserialize(deserializer& d);

    bool open(const char* path);
    bool create(const char* path);
    bool save();
    void close();
    void read(double time);
    void write(double time);

    Node* findNodeByID(uint32_t id);
    Node* findNodeByPath(const std::string& path);
    bool isNodeTypeSupported(Node::Type type) const;
    Node* createNode(Node* parent, const char* name, Node::Type type);

    template<class NodeT>
    bool isNodeTypeSupported() const
    {
        return isNodeTypeSupported(NodeT::node_type);
    }

    template<class NodeT>
    NodeT* createNode(Node* parent, const char* name)
    {
        return dynamic_cast<NodeT*>(createNode(parent, name, NodeT::node_type));
    }

    template<class Body>
    void eachNode(const Body& body)
    {
        for (auto& n : nodes)
            body(n.get());
    }

    template<class NodeT, class Body>
    void eachNode(const Body& body)
    {
        for (auto& n : nodes) {
            if (auto tn = dynamic_cast<NodeT*>(n.get()))
                body(tn);
        }
    }

    template<class NodeT, class Body>
    void eachNode(Node::Type type, const Body& body)
    {
        for (auto& n : nodes) {
            if (auto tn = dynamic_cast<NodeT*>(n.get())) {
                if (tn->getType() == type)
                    body(tn);
            }
        }
    }

    template<class NodeT>
    std::vector<NodeT*> getNodes()
    {
        std::vector<NodeT*> ret;
        eachNode<NodeT>([&ret](auto n) { ret.push_back(n); });
        return ret;
    }

    // internal
    virtual void registerNode(Node* n);
    virtual Node* createNodeImpl(Node* parent, const char* name, Node::Type type);

public:
    // serializable
    std::string path;
    std::vector<NodePtr> nodes;
    RootNode* root_node = nullptr;
    UpAxis up_axis = UpAxis::Unknown;
    int frame_count = 0;
    double frame_rate = 30.0;
    double time_start = 0.0;
    double time_end = 0.0;
    double time_current = default_time;

    // non-serializable
    SceneInterfacePtr impl;
};
sgSerializable(Scene);
sgDeclPtr(Scene);

} // namespace sg
