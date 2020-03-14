#pragma once
#include "mqusdCore.h"
#include "mqusdSceneGraph.h"


template<class NodeT>
class USDNode : public NodeT
{
using super = NodeT;
public:
    USDNode(Node* p, UsdPrim usd)
        : super(p)
        , prim(usd)
    {
        this->name = usd.GetName();
    }

    UsdPrim* getPrim() override
    {
        return &prim;
    }

protected:
    UsdPrim prim;
};


class Node_ : public USDNode<Node>
{
using super = USDNode<Node>;
public:
    Node_(Node* parent, UsdPrim usd);
};


class RootNode_ : public USDNode<RootNode>
{
using super = USDNode<RootNode>;
public:
    RootNode_(UsdPrim usd);
};


class XformNode_ : public USDNode<XformNode>
{
using super = USDNode<XformNode>;
public:
    XformNode_(Node* parent, UsdPrim usd);
    void seek(double si) override;

private:
    UsdGeomXformable schema;
};


class MeshNode_ : public USDNode<MeshNode>
{
using super = USDNode<MeshNode>;
public:
    MeshNode_(Node* parent, UsdPrim usd);
    void seek(double si) override;

private:
    UsdGeomMesh schema;
    UsdAttribute attr_uvs;
    UsdAttribute attr_mids;
};


class InstancerNode_ : USDNode<Node>
{
using super = USDNode<Node>;
public:
    InstancerNode_(Node* parent, UsdPrim usd);
    void seek(double si) override;
};

class InstanceNode_ : XformNode
{
using super = XformNode;
public:
    InstanceNode_(Node* parent);
    void seek(double si) override;
};


class MaterialNode_ : public USDNode<MaterialNode>
{
using super = USDNode<MaterialNode>;
public:
    MaterialNode_(Node* parent, UsdPrim usd);
    void seek(double si) override;
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
    void seek(double t) override;

private:
    void constructTree(Node* n);

    UsdStageRefPtr m_stage;
};
