#pragma once
#include "mqusd.h"
#include "mqusdMesh.h"

#ifndef PXR_USD_USD_PRIM_H
class UsdPrim;
#endif
struct mqusdPlayerSettings;
class SceneInterface;
using SceneInterfacePtr = std::shared_ptr<SceneInterface>;

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

    template<class NodeT> NodeT* findParent();
    std::string getPath() const;

    void* impl = nullptr;
    Node* parent = nullptr;
    std::vector<Node*> children;
    std::string name;
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

    XformNode* parent_xform = nullptr;
    float4x4 local_matrix = float4x4::identity();
    float4x4 global_matrix = float4x4::identity();
};


class MeshNode : public XformNode
{
using super = XformNode;
public:
    static const Type node_type = Type::Mesh;

    MeshNode(Node* parent);
    Type getType() const override;
    void convert(const mqusdPlayerSettings& settings);

public:
    MeshPtr mesh;
};


class BlendshapeNode : public Node
{
using super = Node;
public:
    static const Type node_type = Type::Blendshape;

    BlendshapeNode(Node* parent);
    Type getType() const override;

public:
    BlendshapePtr blendshape;
};


class SkeletonNode : public XformNode
{
using super = XformNode;
public:
    static const Type node_type = Type::Skeleton;

    SkeletonNode(Node* parent);
    Type getType() const override;

public:
    SkeletonPtr skeleton;
};


class MaterialNode : public Node
{
using super = Node;
public:
    static const Type node_type = Type::Material;

    MaterialNode(Node* parent);
    Type getType() const override;
    virtual bool valid() const;

    MaterialPtr material;
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
