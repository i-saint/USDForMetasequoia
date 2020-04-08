#pragma once
#include "sgabcInternal.h"

namespace sg {

class ABCOScene;

#define DefSchemaTraits(Type)\
    using AbcType = Type;

class ABCONode
{
public:
    DefSchemaTraits(AbcGeom::OObject);

    ABCONode(ABCONode* parent, Abc::OObject* obj, bool create_node = true);
    virtual ~ABCONode();
    virtual void beforeWrite();
    virtual void write(double t);

    void setNode(Node* node);
    std::string getPath() const;

    template<class NodeT = Node, sgEnableIf(std::is_base_of<Node, NodeT>::value)>
    NodeT* getNode() { return static_cast<NodeT*>(m_node); }

public:
    Abc::OObject* m_obj;
    ABCOScene* m_scene = nullptr;
    Node* m_node = nullptr;
    ABCONode* m_parent = nullptr;
    std::vector<ABCONode*> m_children;
    uint32_t m_write_count = 0;
};
sgDeclPtr(ABCONode);


class ABCORootNode : public ABCONode
{
using super = ABCONode;
public:
    ABCORootNode(Abc::OObject* obj);

protected:
};


class ABCOXformNode : public ABCONode
{
using super = ABCONode;
public:
    DefSchemaTraits(AbcGeom::OXform);

    ABCOXformNode(ABCONode* parent, Abc::OObject* obj);
    void write(double t) override;

protected:
    AbcGeom::OXformSchema m_schema;
    AbcGeom::XformSample m_sample;
};


class ABCOMeshNode : public ABCONode
{
using super = ABCONode;
public:
    DefSchemaTraits(AbcGeom::OPolyMesh);

    ABCOMeshNode(ABCONode* parent, Abc::OObject* obj);
    void beforeWrite() override;
    void write(double t) override;

protected:
    AbcGeom::OPolyMeshSchema m_schema;
    AbcGeom::OC4fGeomParam m_rgba_param;
    AbcGeom::OInt32ArrayProperty m_mids_prop;

    AbcGeom::OPolyMeshSchema::Sample m_sample;
    AbcGeom::ON3fGeomParam::Sample m_normals;
    AbcGeom::OV2fGeomParam::Sample m_uvs;
    AbcGeom::OC4fGeomParam::Sample m_rgba;

    struct FacesetData
    {
        AbcGeom::OFaceSetSchema faceset;
        AbcGeom::OFaceSetSchema::Sample sample;
    };
    std::map<std::string, FacesetData> m_facesets;
};


class ABCOMaterialNode : public ABCONode
{
using super = ABCONode;
public:
    DefSchemaTraits(AbcMaterial::OMaterial);

    ABCOMaterialNode(ABCONode* parent, Abc::OObject* obj);
    void beforeWrite() override;
    void write(double t) override;

protected:
    AbcMaterial::OMaterialSchema m_schema;

    Abc::OBoolProperty  m_use_vertex_color_prop;
    Abc::OBoolProperty  m_double_sided_prop;
    Abc::OC3fProperty   m_diffuse_color_prop;
    Abc::OFloatProperty m_diffuse_prop;
    Abc::OFloatProperty m_opacity_prop;
    Abc::OFloatProperty m_roughness_prop;
    Abc::OC3fProperty   m_ambient_color_prop;
    Abc::OC3fProperty   m_specular_color_prop;
    Abc::OC3fProperty   m_emissive_color_prop;

    Abc::OStringProperty m_diffuse_texture_prop;
    Abc::OStringProperty m_opacity_texture_prop;
    Abc::OStringProperty m_bump_texture_prop;
};

#undef DefSchemaTraits


class ABCOScene : public SceneInterface
{
public:
    static ABCOScene* getCurrent();

    ABCOScene(Scene* scene);
    ~ABCOScene() override;

    bool open(const char* path) override;
    bool create(const char* path) override;
    bool save() override;
    void close() override;
    void read() override;
    void write() override;
    Node* createNode(Node* parent, const char* name, Node::Type type) override;
    bool wrapNode(Node* node) override;

    uint32_t getWriteCount() const;
    ABCONode* findABCNodeImpl(const std::string& path);
    Node* findNodeImpl(const std::string& path);

    template<class NodeT, sgEnableIf(std::is_base_of<ABCONode, NodeT>::value)>
    NodeT* findNode(const std::string& path)
    {
        return dynamic_cast<NodeT*>(findABCNodeImpl(path));
    }

    template<class NodeT, sgEnableIf(std::is_base_of<Node, NodeT>::value)>
    NodeT* findNode(const std::string& path)
    {
        return dynamic_cast<NodeT*>(findNodeImpl(path));
    }

private:
    void registerNode(ABCONode* n);
    template<class NodeT> ABCONode* createNodeImpl(ABCONode* parent, const char* name);

    std::string m_abc_path;
    Abc::OArchive m_archive;
    std::vector<std::shared_ptr<Abc::OObject>> m_objects;
    std::vector<ABCONodePtr> m_nodes;
    std::map<std::string, ABCONode*> m_node_table;
    ABCORootNode* m_root = nullptr;

    Scene* m_scene = nullptr;
    uint32_t m_write_count = 0;
    double m_max_time = 0.0;

    bool m_keep_time = false;
    std::vector<abcChrono> m_timeline;
};

} // namespace sg
