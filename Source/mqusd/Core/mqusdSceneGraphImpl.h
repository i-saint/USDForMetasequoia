#pragma once
#include "mqusdCore.h"
#include "mqusdSceneGraph.h"


template<class NodeT>
class USDNode_ : public NodeT
{
using super = NodeT;
public:
    USDNode_(Node* p, UsdPrim usd);
    UsdPrim* getPrim() override;

protected:
    UsdPrim prim;
};

template<class NodeT, class SchemaT>
class USDXformableNode_ : public USDNode_<NodeT>
{
using super = USDNode_<NodeT>;
public:
    USDXformableNode_(Node* p, UsdPrim usd);

protected:
    void readXform(const UsdTimeCode& t);
    void writeXform(const UsdTimeCode& t) const;

    mutable SchemaT schema;
    mutable std::vector<UsdGeomXformOp> xf_ops;
};


class Node_ : public USDNode_<Node>
{
using super = USDNode_<Node>;
public:
    Node_(Node* parent, UsdPrim usd);
};


class RootNode_ : public USDNode_<RootNode>
{
using super = USDNode_<RootNode>;
public:
    RootNode_(UsdPrim usd);
};


class XformNode_ : public USDXformableNode_<XformNode, UsdGeomXformable>
{
using super = USDXformableNode_<XformNode, UsdGeomXformable>;
public:
    XformNode_(Node* parent, UsdPrim usd);
    void read(double time) override;
    void write(double time) const override;

private:
};


class MeshNode_ : public USDXformableNode_<MeshNode, UsdGeomMesh>
{
using super = USDXformableNode_<MeshNode, UsdGeomMesh>;
public:
    MeshNode_(Node* parent, UsdPrim usd);
    void read(double time) override;
    void write(double time) const override;

private:
    UsdAttribute attr_uvs;
    UsdAttribute attr_mids;
};


class InstancerNode_ : USDNode_<Node>
{
using super = USDNode_<Node>;
public:
    InstancerNode_(Node* parent, UsdPrim usd);
    void read(double time) override;
};

class InstanceNode_ : XformNode
{
using super = XformNode;
public:
    InstanceNode_(Node* parent);
    void read(double time) override;
};


class MaterialNode_ : public USDNode_<MaterialNode>
{
using super = USDNode_<MaterialNode>;
public:
    MaterialNode_(Node* parent, UsdPrim usd);
    void read(double time) override;
    bool valid() const override;

private:
};


class Scene_ : public Scene
{
using super = Scene;
public:
    Scene_();
    ~Scene_() override;

    bool open(const char* path) override;
    void close() override;
    void read(double time) override;
    void write(double time) const override;

private:
    void constructTree(Node* n);

    UsdStageRefPtr m_stage;
};
