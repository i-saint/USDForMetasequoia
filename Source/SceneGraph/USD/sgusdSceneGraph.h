#pragma once
#include "sgusdInternal.h"

namespace sg {

#define DefSchemaTraits(Type, Typename)\
    using UsdType = Type;\
    static constexpr const char* getUsdTypeName() { return Typename; };


class USDNode;
class USDScene;

class USDNode
{
public:
    DefSchemaTraits(UsdSchemaBase, "");

    USDNode(USDNode* parent, UsdPrim prim, bool create_node = true);
    USDNode(Node* n, UsdPrim prim);
    virtual ~USDNode();

    virtual void beforeRead();
    virtual void read(UsdTimeCode t);

    virtual void beforeWrite();
    virtual void write(UsdTimeCode t);

    void setNode(Node *node);
    const std::string& getName() const;
    const std::string& getPath() const;
    std::string makeUniqueName(const std::string& name) const;

    template<class NodeT>
    NodeT* cast() { return dynamic_cast<NodeT*>(this); }

    template<class Body>
    void eachChildR(const Body& body)
    {
        for (auto c : m_children) {
            body(c);
            c->eachChildR(body);
        }
    }

    template<class NodeT = Node, sgEnableIf(std::is_base_of<Node, NodeT>::value)>
    NodeT* getNode() { return static_cast<NodeT*>(m_node); }

    template<class T>
    void padSample(const UsdAttribute& attr, UsdTimeCode t, const T& default_sample = {});

public:
    UsdPrim m_prim;
    USDScene* m_scene = nullptr;
    Node* m_node = nullptr;
    USDNode* m_parent = nullptr;
    std::vector<USDNode*> m_children;
    uint32_t m_write_count = 0;

    UsdAttribute m_attr_display_name;
};
using USDNodePtr = std::shared_ptr<USDNode>;


class USDRootNode : public USDNode
{
using super = USDNode;
public:
    USDRootNode(UsdPrim prim);
    USDRootNode(Node* n, UsdPrim prim);
};


class USDXformNode : public USDNode
{
using super = USDNode;
public:
    DefSchemaTraits(UsdGeomXformable, "Xform");

    USDXformNode(USDNode* parent, UsdPrim prim, bool create_node = true);
    USDXformNode(Node* n, UsdPrim prim);
    void read(UsdTimeCode t) override;
    void write(UsdTimeCode t) override;

private:
    UsdGeomXformable m_xform;
    std::vector<UsdGeomXformOp> m_xf_ops;
};


class USDBlendshapeNode : public USDNode
{
using super = USDNode;
public:
    DefSchemaTraits(UsdSkelBlendShape, "BlendShape");

    USDBlendshapeNode(USDNode* parent, UsdPrim prim);
    USDBlendshapeNode(Node* n, UsdPrim prim);
    void read(UsdTimeCode t) override;
    void beforeWrite() override;

private:
    UsdSkelBlendShape m_blendshape;

    // sample holder
    VtArray<int> m_point_indices;

    struct Target
    {
        VtArray<GfVec3f> point_offsets;
        VtArray<GfVec3f> normal_offsets;
    };
    std::vector<Target> m_targets;
};


class USDSkelRootNode : public USDXformNode
{
using super = USDXformNode;
public:
    DefSchemaTraits(UsdSkelRoot, "SkelRoot");

    USDSkelRootNode(USDNode* parent, UsdPrim prim);
    USDSkelRootNode(Node* n, UsdPrim prim);
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
    USDSkeletonNode(Node* n, UsdPrim prim);
    void beforeRead() override;
    void read(UsdTimeCode t) override;
    void beforeWrite() override;
    void write(UsdTimeCode t) override;

private:
    UsdSkelSkeleton m_skel;
    UsdSkelCache m_cache;
};


class USDSkelAnimationNode : public USDNode
{
using super = USDNode;
public:
    DefSchemaTraits(UsdSkelAnimation, "SkelAnimation");

    USDSkelAnimationNode(USDNode* parent, UsdPrim prim);
    USDSkelAnimationNode(Node* n, UsdPrim prim);
    void beforeRead() override;


    struct BlendshapeData
    {
        std::string id;
        float weight;
    };

    // will be called from USDMeshNode. USDMeshNode's read() possibly be called before USDSkelAnimationNode's.
    // so this method will do read data from USD on-demand.
    std::vector<const BlendshapeData*>& getBlendshapeData(UsdTimeCode t);

private:
    UsdSkelAnimation m_anim;
    UsdTimeCode m_prev_read = UsdTimeCode(-1);
    std::vector<BlendshapeData> m_blendshapes;
    std::vector<const BlendshapeData*> m_blendshapes_sorted;
    VtArray<float> m_bs_weights;
};


class USDMeshNode : public USDXformNode
{
using super = USDXformNode;
public:
    DefSchemaTraits(UsdGeomMesh, "Mesh");

    USDMeshNode(USDNode* parent, UsdPrim prim);
    USDMeshNode(Node* n, UsdPrim prim);
    void beforeRead() override;
    void read(UsdTimeCode t) override;
    void beforeWrite() override;
    void write(UsdTimeCode t) override;

public:
    UsdGeomMesh m_mesh;

    UsdGeomPrimvar m_pv_st;
    UsdGeomPrimvar m_pv_colors;
    UsdAttribute m_attr_bs_ids;
    UsdAttribute m_attr_joints;
    UsdAttribute m_attr_joint_indices;
    UsdAttribute m_attr_joint_weights;
    UsdAttribute m_attr_bind_transform;

    std::vector<USDBlendshapeNode*> m_blendshapes;
    std::vector<std::string> m_blendshape_ids; // will be used to resolve animations
    USDSkelAnimationNode* m_animation = nullptr;
    USDSkeletonNode* m_skeleton = nullptr;

    // sample holder
    VtArray<int> m_counts;
    VtArray<int> m_indices;
    VtArray<GfVec3f> m_points;
    VtArray<GfVec3f> m_normals;
    VtArray<GfVec2f> m_uvs;
    VtArray<GfVec4f> m_colors;
    VtArray<int> m_uv_indices;
    VtArray<int> m_material_ids;
    VtArray<int> m_joint_indices;
    VtArray<float> m_joint_weights;

    // subset data
    struct SubsetData
    {
        UsdGeomSubset subset;
        VtArray<int> sample;
        FaceSetPtr dst;
    };
    std::vector<SubsetData> m_isubsets;
    std::map<uint32_t, SubsetData> m_osubsets;
};


class USDInstancerNode : public USDXformNode
{
using super = USDXformNode;
public:
    DefSchemaTraits(UsdGeomPointInstancer, "PointInstancer");

    USDInstancerNode(USDNode* parent, UsdPrim prim);
    USDInstancerNode(Node* n, UsdPrim prim);
    void beforeRead() override;
    void read(UsdTimeCode t) override;
    void beforeWrite() override;
    void write(UsdTimeCode t) override;

public:
    UsdGeomPointInstancer m_instancer;
    VtArray<int> m_proto_indices;
    VtArray<GfMatrix4d> m_matrices;

    VtArray<GfVec3f> m_positions;
    VtArray<GfQuath> m_orientations;
    VtArray<GfVec3f> m_scales;
};


class USDMaterialNode : public USDNode
{
using super = USDNode;
public:
    DefSchemaTraits(UsdShadeMaterial, "Material");

    USDMaterialNode(USDNode* parent, UsdPrim prim);
    USDMaterialNode(Node* n, UsdPrim prim);
    void beforeRead() override;
    void read(UsdTimeCode t) override;
    void beforeWrite() override;

public:
    UsdShadeMaterial m_material;

    UsdShadeShader m_surface;
    UsdShadeShader m_tex_diffuse;
    UsdShadeShader m_tex_opacity;
    UsdShadeShader m_tex_bump;

    UsdShadeInput m_in_use_vertex_color;
    UsdShadeInput m_in_double_sided;
    UsdShadeInput m_in_diffuse_color;
    UsdShadeInput m_in_diffuse;
    UsdShadeInput m_in_opacity;
    UsdShadeInput m_in_roughness;
    UsdShadeInput m_in_ambient_color;
    UsdShadeInput m_in_specular_color;
    UsdShadeInput m_in_emissive_color;
};


class USDShaderNode : public USDNode
{
using super = USDNode;
public:
    DefSchemaTraits(UsdShadeShader, "Shader");

    USDShaderNode(USDNode* parent, UsdPrim prim);
    USDShaderNode(Node* n, UsdPrim prim);

private:
    UsdShadeShader m_shader;
};

#undef DefSchemaTraits


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
    void read() override;
    void write() override;

    bool isNodeTypeSupported(Node::Type type) override;
    Node* createNode(Node* parent, const char* name, Node::Type type) override;
    bool wrapNode(Node* node) override;
    double frameToTime(int frame) override;

    Scene* getHostScene();
    UsdStageRefPtr& getStage();
    UsdTimeCode toTimeCode(double time) const;
    UsdTimeCode getPrevTime() const;
    USDNode* findUSDNodeImpl(const std::string& path);
    Node* findNodeImpl(const std::string& path);

    template<class NodeT, sgEnableIf(std::is_base_of<USDNode, NodeT>::value)>
    NodeT* findNode(const std::string& path)
    {
        return dynamic_cast<NodeT*>(findUSDNodeImpl(path));
    }

    template<class NodeT, sgEnableIf(std::is_base_of<Node, NodeT>::value)>
    NodeT* findNode(const std::string& path)
    {
        return dynamic_cast<NodeT*>(findNodeImpl(path));
    }

private:
    void registerNode(USDNode* n);
    void constructTree(USDNode* n);
    template<class NodeT> USDNode* createNodeImpl(USDNode* parent, std::string path);
    template<class NodeT> USDNode* wrapNodeImpl(Node* node);

    UsdStageRefPtr m_stage;
    std::vector<USDNodePtr> m_nodes;
    std::map<std::string, USDNode*> m_node_table;
    USDRootNode* m_root = nullptr;

    Scene* m_scene = nullptr;
    UsdTimeCode m_prev_time;
    int m_read_count = 0;
    int m_write_count = 0;
    double m_frame_rate = 30.0;
    double m_max_time = 0.0;
};

} // namespace sg
