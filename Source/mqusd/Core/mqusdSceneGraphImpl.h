#pragma once
#include "mqusdCore.h"
#include "mqusdSceneGraph.h"


#define DefSchemaTraits(Type, Typename)\
    using UsdType = Type;\
    static constexpr const char* getUsdTypeName() { return Typename; };


class USDNode
{
public:
    DefSchemaTraits(UsdSchemaBase, "");

    USDNode(USDNode* parent, UsdPrim prim, bool create_node = true);
    virtual ~USDNode();
    virtual void read(double time);
    virtual void write(double time) const;

    void setNode(Node *node);
    std::string getPath() const;

public:
    UsdPrim m_prim;
    Node* m_node = nullptr;

    USDNode* m_parent = nullptr;
    std::vector<USDNode*> m_children;
};
using USDNodePtr = std::shared_ptr<USDNode>;


class USDRootNode : public USDNode
{
using super = USDNode;
public:
    USDRootNode(UsdPrim usd);
};


class USDXformNode : public USDNode
{
using super = USDNode;
public:
    DefSchemaTraits(UsdGeomXformable, "Xform");

    USDXformNode(USDNode* parent, UsdPrim prim, bool create_node = true);
    void read(double time) override;
    void write(double time) const override;

private:
    mutable UsdGeomXformable m_xform;
    mutable std::vector<UsdGeomXformOp> m_xf_ops;
};


class USDMeshNode : public USDXformNode
{
using super = USDXformNode;
public:
    DefSchemaTraits(UsdGeomMesh, "Mesh");

    USDMeshNode(USDNode* parent, UsdPrim usd);
    void read(double time) override;
    void write(double time) const override;

private:
    mutable UsdGeomMesh m_mesh;
    UsdAttribute m_attr_uvs;
    UsdAttribute m_attr_mids;
};


class USDInstancerNode : USDXformNode
{
using super = USDXformNode;
public:
    USDInstancerNode(USDNode* parent, UsdPrim usd);
    void read(double time) override;
    void write(double time) const override;
};


class USDMaterialNode : public USDNode
{
using super = USDNode;
public:
    USDMaterialNode(USDNode* parent, UsdPrim usd);
    void read(double time) override;
    void write(double time) const override;

private:
};


class USDScene : public SceneInterface
{
public:
    USDScene(Scene *scene);
    ~USDScene() override;

    bool open(const char* path) override;
    bool create(const char* path) override;
    bool save() override;
    void close() override;
    void read(double time) override;
    void write(double time) const override;

    Node* createNode(Node* parent, const char* name, Node::Type type) override;

private:
    void constructTree(USDNode* n);
    template<class NodeT>
    USDNode* createNodeImpl(USDNode*parent, std::string path);

    UsdStageRefPtr m_stage;
    std::vector<USDNodePtr> m_nodes;
    USDRootNode* m_root = nullptr;

    Scene* m_scene = nullptr;
};
