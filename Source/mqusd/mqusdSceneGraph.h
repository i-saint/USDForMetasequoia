#pragma once
#include "mqusd.h"
#include "mqusdMesh.h"

#ifndef PXR_USD_USD_PRIM_H
class UsdPrim;
#endif
struct mqusdPlayerSettings;

class Node
{
public:
    enum class Type
    {
        Unknown,
        Root,
        Xform,
        PolyMesh,
        Material,
    };

    Node(Node* parent);
    virtual ~Node();
    virtual void release();
    virtual Type getType() const;
    virtual void seek(double si);
    virtual void write();

    virtual UsdPrim* getPrim();

    template<class NodeT> NodeT* findParent();

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

    RootNode(Node* parent = nullptr);
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
    static const Type node_type = Type::PolyMesh;

    MeshNode(Node* parent);
    Type getType() const override;
    void convert(const mqusdPlayerSettings& settings);

    mqusdMesh mesh;
};


class MaterialNode : public Node
{
using super = Node;
public:
    static const Type node_type = Type::Material;

    MaterialNode(Node* parent);
    Type getType() const override;
    virtual bool valid() const;

    mqusdMaterial material;
};


class Scene
{
public:
    Scene();
    virtual ~Scene();
    virtual void release();
    virtual bool open(const char* path);
    virtual void close();
    virtual void seek(double t);
    virtual void write();

public:
    std::string path;
    std::vector<NodePtr> nodes;
    RootNode* root_node = nullptr;
    std::vector<MeshNode*> mesh_nodes;
    std::vector<MaterialNode*> material_nodes;
    double time_start = 0.0;
    double time_end = 0.0;
};
using ScenePtr = std::shared_ptr<Scene>;

ScenePtr CreateScene();

