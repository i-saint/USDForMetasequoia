#include "mqabc.h"

namespace mqusd {

class ABCOScene;

class ABCONode
{
public:
    ABCONode(ABCONode* parent, Abc::OObject obj, bool create_node);
    virtual ~ABCONode();
    virtual void write();

    void setNode(Node* node);
    std::string getPath() const;

protected:
    Abc::OObject m_obj;
    ABCOScene* m_scene = nullptr;
    Node* m_node = nullptr;
    ABCONode* m_parent = nullptr;
    std::vector<ABCONode*> m_children;
};
mqusdDeclPtr(ABCONode);


class ABCORootNode : public ABCONode
{
public:
    ABCORootNode(ABCONode* parent, Abc::OObject obj);

protected:
};


class ABCOXformNode : public ABCONode
{
public:
    ABCOXformNode(ABCONode* parent, Abc::OObject obj);
    void write() override;

protected:
    AbcGeom::OXformSchema m_schema;
    AbcGeom::XformSample m_sample;
};


class ABCOMeshNode : public ABCONode
{
public:
    ABCOMeshNode(ABCONode* parent, Abc::OObject obj);
    void write() override;

protected:
    AbcGeom::OPolyMeshSchema m_schema;
    AbcGeom::OC3fGeomParam m_rgb_param;
    AbcGeom::OC4fGeomParam m_rgba_param;
    AbcGeom::OInt32ArrayProperty m_mids_prop;
    AbcGeom::OPolyMeshSchema::Sample m_sample;
};


class ABCOMaterialNode : public ABCONode
{
public:
    ABCOMaterialNode(ABCONode* parent, Abc::OObject obj);
    void write() override;

protected:
    AbcMaterial::IMaterialSchema m_schema;
    Abc::OBoolProperty m_use_vertex_color_prop;
    Abc::OBoolProperty m_double_sided_prop;
    Abc::OC3fProperty m_color_prop;
    Abc::OFloatProperty m_diffuse_prop;
    Abc::OFloatProperty m_alpha_prop;
    Abc::OC3fProperty m_ambient_prop;
    Abc::OC3fProperty m_specular_prop;
    Abc::OC3fProperty m_emission_prop;
};



class ABCOScene : SceneInterface
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

    ABCONode* findNode(const std::string& path);

private:
    void constructTree(ABCONode* n);
    template<class NodeT> ABCONode* createNodeImpl(ABCONode* parent, std::string path);
    template<class NodeT> ABCONode* wrapNodeImpl(Node* node);

    std::string m_abc_path;
    std::shared_ptr<std::fstream> m_stream;
    Abc::OArchive m_archive;
    std::vector<ABCONodePtr> m_nodes;
    std::map<std::string, ABCONode*> m_node_table;
    ABCORootNode* m_root = nullptr;

    Scene* m_scene = nullptr;
    int m_write_count = 0;
    double m_max_time = 0.0;
};

} // namespace mqusd
