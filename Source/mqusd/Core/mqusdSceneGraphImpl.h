#pragma once
#include "mqusdCore.h"
#include "mqusdSceneGraph.h"


#define DefSchemaTraits(Type, Typename)\
    using UsdType = Type;\
    static constexpr const char* getUsdTypeName() { return Typename; };


template<class NodeT>
class USDBaseNode : public NodeT
{
using super = NodeT;
public:
    USDBaseNode(Node* parent, UsdPrim usd);
    UsdPrim* getPrim() override;

protected:
    UsdPrim prim;
};

template<class NodeT, class SchemaT>
class USDXformableNode : public USDBaseNode<NodeT>
{
using super = USDBaseNode<NodeT>;
public:
    USDXformableNode(Node* parent, UsdPrim usd);

protected:
    void readXform(const UsdTimeCode& t);
    void writeXform(const UsdTimeCode& t) const;

    mutable SchemaT schema;
    mutable std::vector<UsdGeomXformOp> xf_ops;
};


class USDNode : public USDBaseNode<Node>
{
using super = USDBaseNode<Node>;
public:
    DefSchemaTraits(UsdSchemaBase, "");

    USDNode(Node* parent, UsdPrim usd);
};


class USDRootNode : public USDBaseNode<RootNode>
{
using super = USDBaseNode<RootNode>;
public:
    USDRootNode(UsdPrim usd);
};


class USDXformNode : public USDXformableNode<XformNode, UsdGeomXformable>
{
using super = USDXformableNode<XformNode, UsdGeomXformable>;
public:
    DefSchemaTraits(UsdGeomXformable, "Xform");

    USDXformNode(Node* parent, UsdPrim usd);
    void read(double time) override;
    void write(double time) const override;

private:
};


class USDMeshNode : public USDXformableNode<MeshNode, UsdGeomMesh>
{
using super = USDXformableNode<MeshNode, UsdGeomMesh>;
public:
    DefSchemaTraits(UsdGeomMesh, "Mesh");

    USDMeshNode(Node* parent, UsdPrim usd);
    void read(double time) override;
    void write(double time) const override;

private:
    UsdAttribute attr_uvs;
    UsdAttribute attr_mids;
};


class USDInstancerNode : USDBaseNode<Node>
{
using super = USDBaseNode<Node>;
public:
    USDInstancerNode(Node* parent, UsdPrim usd);
    void read(double time) override;
};

class USDInstanceNode : XformNode
{
using super = XformNode;
public:
    USDInstanceNode(Node* parent);
    void read(double time) override;
};


class USDMaterialNode : public USDBaseNode<MaterialNode>
{
using super = USDBaseNode<MaterialNode>;
public:
    USDMaterialNode(Node* parent, UsdPrim usd);
    void read(double time) override;
    bool valid() const override;

private:
};


class USDScene : public Scene
{
using super = Scene;
public:
    USDScene();
    ~USDScene() override;

    bool open(const char* path) override;
    bool create(const char* path) override;
    bool save() override;
    void close() override;
    void read(double time) override;
    void write(double time) const override;

    Node* createNode(Node* parent, const char* name, Node::Type type) override;

private:
    void constructTree(Node* n);
    template<class NodeT>
    Node* createNodeImpl(Node *parent, std::string path);

    UsdStageRefPtr m_stage;
};
