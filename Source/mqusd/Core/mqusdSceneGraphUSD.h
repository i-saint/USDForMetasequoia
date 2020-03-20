#pragma once
#include "mqusdCore.h"

namespace mqusd {

#define DefSchemaTraits(Type, Typename)\
    using UsdType = Type;\
    static constexpr const char* getUsdTypeName() { return Typename; };


class USDNode;
class USDScene;

class USDNode
{
public:
    DefSchemaTraits(UsdSchemaBase, "");

    USDNode(USDNode* parent, UsdPrim prim, bool create_node);
    virtual ~USDNode();

    virtual void beforeRead();
    virtual void read(double time);

    virtual void beforeWrite();
    virtual void write(double time) const;

    void setNode(Node *node);
    std::string getPath() const;

    template<class Body>
    void eachChildR(const Body& body)
    {
        for (auto c : m_children) {
            body(c);
            c->eachChildR(body);
        }
    }

public:
    UsdPrim m_prim;
    USDScene* m_scene = nullptr;
    Node* m_node = nullptr;
    USDNode* m_parent = nullptr;
    std::vector<USDNode*> m_children;
};
using USDNodePtr = std::shared_ptr<USDNode>;


class USDRootNode : public USDNode
{
using super = USDNode;
public:
    USDRootNode(UsdPrim prim);
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

    USDMeshNode(USDNode* parent, UsdPrim prim);
    void beforeRead() override;
    void read(double time) override;
    void beforeWrite() override;
    void write(double time) const override;

private:
    mutable UsdGeomMesh m_mesh;

    UsdAttribute m_attr_uv;
    UsdAttribute m_attr_uv_indices;
    UsdAttribute m_attr_mids;
    UsdAttribute m_attr_joints;
    UsdAttribute m_attr_joint_indices;
    UsdAttribute m_attr_joint_weights;
    UsdAttribute m_attr_bind_transform;

    // sample holder
    mutable VtArray<int> m_counts;
    mutable VtArray<int> m_indices;
    mutable VtArray<GfVec3f> m_points;
    mutable VtArray<GfVec3f> m_normals;
    mutable VtArray<GfVec2f> m_uvs;
    mutable VtArray<int> m_uv_indices;
    mutable VtArray<int> m_material_ids;
    mutable VtArray<int> m_joint_indices;
    mutable VtArray<float> m_joint_weights;
};


class USDBlendshapeNode : public USDNode
{
using super = USDNode;
public:
    DefSchemaTraits(UsdSkelBlendShape, "BlendShape");

    USDBlendshapeNode(USDNode* parent, UsdPrim prim);
    void beforeRead() override;
    void beforeWrite() override;

private:
    mutable UsdSkelBlendShape m_blendshape;

    // sample holder
    mutable VtArray<int> m_point_indices;
    mutable VtArray<GfVec3f> m_point_offsets;
    mutable VtArray<GfVec3f> m_normal_offsets;
};


class USDSkelRootNode : public USDXformNode
{
using super = USDXformNode;
public:
    DefSchemaTraits(UsdSkelRoot, "SkelRoot");

    USDSkelRootNode(USDNode* parent, UsdPrim prim);
    void beforeRead() override;
    void beforeWrite() override;

private:
    UsdRelationship m_skel;
};


class USDSkeletonNode : public USDXformNode
{
using super = USDXformNode;
public:
    DefSchemaTraits(UsdSkelSkeleton, "Skeleton");

    USDSkeletonNode(USDNode* parent, UsdPrim prim);
    void beforeRead() override;
    void read(double time) override;
    void beforeWrite() override;
    void write(double time) const override;

private:
    mutable UsdSkelSkeleton m_skel;
    UsdSkelCache m_skel_cache;
};


class USDInstancerNode : USDXformNode
{
using super = USDXformNode;
public:
    USDInstancerNode(USDNode* parent, UsdPrim prim);
    void read(double time) override;
    void write(double time) const override;
};


class USDMaterialNode : public USDNode
{
using super = USDNode;
public:
    DefSchemaTraits(UsdShadeMaterial, "Material");

    USDMaterialNode(USDNode* parent, UsdPrim prim);
    void read(double time) override;
    void write(double time) const override;

private:
    UsdShadeMaterial m_material;
};


class USDScene : public SceneInterface
{
public:
    static USDScene* getCurrent();

    USDScene(Scene *scene);
    ~USDScene() override;

    bool open(const char* path) override;
    bool create(const char* path) override;
    bool save() override;
    void close() override;
    void read(double time) override;
    void write(double time) const override;
    Node* createNode(Node* parent, const char* name, Node::Type type) override;

    USDNode* findNode(const std::string& path);

private:
    void constructTree(USDNode* n);
    template<class NodeT>
    USDNode* createNodeImpl(USDNode*parent, std::string path);

    UsdStageRefPtr m_stage;
    std::vector<USDNodePtr> m_nodes;
    std::map<std::string, USDNode*> m_node_table;
    USDRootNode* m_root = nullptr;

    Scene* m_scene = nullptr;
    mutable int m_frame = 0;
};

} // namespace mqusd
