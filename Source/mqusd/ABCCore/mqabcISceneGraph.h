#include "mqabc.h"

namespace mqusd {

class ABCIScene;

class ABCINode
{
public:
    ABCINode(ABCINode* parent, Abc::IObject obj, bool create_node);
    virtual ~ABCINode();
    virtual void read(double time);

    void setNode(Node* node);
    std::string getPath() const;

protected:
    Abc::IObject m_obj;
    ABCIScene* m_scene = nullptr;
    Node* m_node = nullptr;
    ABCINode* m_parent = nullptr;
    std::vector<ABCINode*> m_children;
};
mqusdDeclPtr(ABCINode);


class ABCIRootNode : public ABCINode
{
public:
    ABCIRootNode(ABCINode* parent, Abc::IObject obj);

protected:
};


class ABCIXformNode : public ABCINode
{
public:
    ABCIXformNode(ABCINode* parent, Abc::IObject obj);
    void read(double time) override;

protected:
    AbcGeom::IXformSchema m_schema;
    AbcGeom::XformSample m_sample;
};


class ABCIMeshNode : public ABCINode
{
public:
    ABCIMeshNode(ABCINode* parent, Abc::IObject obj);
    void read(double time) override;

protected:
    AbcGeom::IPolyMeshSchema m_schema;
    AbcGeom::IC3fGeomParam m_rgb_param;
    AbcGeom::IC4fGeomParam m_rgba_param;
    AbcGeom::IInt32ArrayProperty m_mids_prop;
    AbcGeom::IPolyMeshSchema::Sample m_sample;
};


class ABCIMaterialNode : public ABCINode
{
public:
    ABCIMaterialNode(ABCINode* parent, Abc::IObject obj);
    void read(double time) override;

protected:
    AbcMaterial::IMaterialSchema m_schema;
    Abc::IBoolProperty m_use_vertex_color_prop;
    Abc::IBoolProperty m_double_sided_prop;
    Abc::IC3fProperty m_color_prop;
    Abc::IFloatProperty m_diffuse_prop;
    Abc::IFloatProperty m_alpha_prop;
    Abc::IC3fProperty m_ambient_prop;
    Abc::IC3fProperty m_specular_prop;
    Abc::IC3fProperty m_emission_prop;
};



class ABCIScene : SceneInterface
{
public:
    static ABCIScene* getCurrent();

    ABCIScene(Scene* scene);
    ~ABCIScene() override;

    bool open(const char* path) override;
    bool create(const char* path) override;
    bool save() override;
    void close() override;
    void read() override;
    void write() override;
    Node* createNode(Node* parent, const char* name, Node::Type type) override;
    bool wrapNode(Node* node) override;

    ABCINode* findNode(const std::string& path);

private:
    void constructTree(ABCINode* n);
    template<class NodeT> ABCINode* createNodeImpl(ABCINode* parent, std::string path);
    template<class NodeT> ABCINode* wrapNodeImpl(Node* node);

    std::string m_abc_path;
    std::shared_ptr<std::fstream> m_stream;
    Abc::IArchive m_archive;
    std::vector<ABCINodePtr> m_nodes;
    std::map<std::string, ABCINode*> m_node_table;
    ABCIRootNode* m_root = nullptr;

    Scene* m_scene = nullptr;
    int m_read_count = 0;
    double m_max_time = 0.0;
};

} // namespace mqusd
