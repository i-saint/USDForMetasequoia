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
    virtual void update(double si);

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


class MeshNode : public Node
{
using super = Node;
public:
    static const Type node_type = Type::PolyMesh;

    MeshNode(Node* parent);
    Type getType() const override;
    void convert(const mqusdPlayerSettings& settings);

    XformNode* parent_xform = nullptr;
    mqusdMesh mesh;
    size_t sample_count = 0;
};


class MaterialNode : public Node
{
using super = Node;
public:
    static const Type node_type = Type::Material;

    MaterialNode(Node* parent);
    Type getType() const override;
    bool valid() const;

    mqusdMaterial material;
};
