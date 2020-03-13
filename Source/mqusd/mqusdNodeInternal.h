#pragma once
#include "mqusdNode.h"

template<class NodeT>
class USDNode : public NodeT
{
using super = NodeT;
public:
    USDNode(Node* p, UsdPrim usd)
        : super(p)
        , prim(usd)
    {
        name = usd.GetName();
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
    void update(double si) override;

private:
    UsdGeomXformable schema;
};

class MeshNode_ : public USDNode<MeshNode>
{
using super = USDNode<MeshNode>;
public:
    MeshNode_(Node* parent, UsdPrim usd);
    void update(double si) override;

    void updateMeshData(double si);

private:
    UsdGeomMesh schema;
    UsdAttribute attr_uvs;
    UsdAttribute attr_mids;
};

class MaterialNode_ : public USDNode<MaterialNode>
{
using super = USDNode<MaterialNode>;
public:
    MaterialNode_(Node* parent, UsdPrim usd);
    void update(double si) override;

private:
};
