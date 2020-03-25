#include "pch.h"
#include "mqabcOSceneGraph.h"

namespace mqusd {

template<class NodeT>
static inline NodeT* CreateNode(ABCONode* parent, Abc::OObject* obj)
{
    return new NodeT(
        parent ? parent->m_node : nullptr,
        obj->getName().c_str());
}


ABCONode::ABCONode(ABCONode* parent, Abc::OObject* obj, bool create_node)
    : m_obj(obj)
    , m_scene(ABCOScene::getCurrent())
    , m_parent(parent)
{
    if (m_parent)
        m_parent->m_children.push_back(this);
    if (create_node)
        setNode(CreateNode<Node>(parent, obj));
}

ABCONode::~ABCONode()
{
}

void ABCONode::beforeWrite()
{
}

void ABCONode::write()
{
}

void ABCONode::setNode(Node* node)
{
    m_node = node;
    m_node->impl = this;
}

std::string ABCONode::getPath() const
{
    return m_obj->getFullName();
}



ABCORootNode::ABCORootNode(Abc::OObject* obj)
    : super(nullptr, obj, false)
{
    setNode(new RootNode());
}


ABCOXformNode::ABCOXformNode(ABCONode* parent, Abc::OObject* obj)
    : super(parent, obj, false)
{
    m_schema = dynamic_cast<AbcGeom::OXform*>(m_obj)->getSchema();
    setNode(CreateNode<XformNode>(parent, obj));
}

void ABCOXformNode::write()
{
    super::write();
    const auto& src = *static_cast<XformNode*>(m_node);

    double4x4 mat;
    mat.assign(src.local_matrix);
    m_sample.setMatrix((abcM44d&)mat);
    m_schema.set(m_sample);
}


ABCOMeshNode::ABCOMeshNode(ABCONode* parent, Abc::OObject* obj)
    : super(parent, obj, false)
{
    m_schema = dynamic_cast<AbcGeom::OPolyMesh*>(m_obj)->getSchema();
    setNode(CreateNode<MeshNode>(parent, obj));
}

void ABCOMeshNode::beforeWrite()
{
    super::beforeWrite();
}

void ABCOMeshNode::write()
{
    super::write();
    const auto& src = *static_cast<MeshNode*>(m_node);

    m_sample.reset();
    m_sample.setFaceIndices(Abc::Int32ArraySample(src.indices.cdata(), src.indices.size()));
    m_sample.setFaceCounts(Abc::Int32ArraySample(src.counts.cdata(), src.counts.size()));
    m_sample.setPositions(Abc::P3fArraySample((const abcV3*)src.points.cdata(), src.points.size()));
    if (src.normals.size() == src.indices.size()) {
        m_normals.setVals(Abc::V3fArraySample((const abcV3*)src.normals.cdata(), src.normals.size()));
        m_sample.setNormals(m_normals);
    }
    if (src.uvs.size() == src.indices.size()) {
        m_uvs.setVals(Abc::V2fArraySample((const abcV2*)src.uvs.cdata(), src.uvs.size()));
        m_sample.setUVs(m_uvs);
    }
    m_schema.set(m_sample);

    if (src.colors.size() == src.indices.size()) {
        if (!m_rgba_param.valid())
            m_rgba_param = AbcGeom::OC4fGeomParam(m_schema.getArbGeomParams(), mqabcAttrVertexColor, false, AbcGeom::GeometryScope::kFacevaryingScope, 1, 1);

        m_rgba.setVals(Abc::C4fArraySample((const abcC4*)src.colors.cdata(), src.colors.size()));
        m_rgba_param.set(m_rgba);
    }
    if (src.material_ids.size() == src.counts.size()) {
        if (!m_mids_prop.valid())
            m_mids_prop = AbcGeom::OInt32ArrayProperty(m_schema.getArbGeomParams(), mqabcAttrMaterialID, 1);

        m_mids_prop.set(Abc::Int32ArraySample(src.material_ids.cdata(), src.material_ids.size()));
    }
}


ABCOMaterialNode::ABCOMaterialNode(ABCONode* parent, Abc::OObject* obj)
    : super(parent, obj, false)
{
    m_schema = dynamic_cast<AbcMaterial::OMaterial*>(m_obj)->getSchema();
    setNode(CreateNode<MaterialNode>(parent, obj));
}

void ABCOMaterialNode::beforeWrite()
{
    super::beforeWrite();
}

void ABCOMaterialNode::write()
{
    super::write();
}


static thread_local ABCOScene* g_current_scene;

ABCOScene* ABCOScene::getCurrent()
{
    return g_current_scene;
}

ABCOScene::ABCOScene(Scene* scene)
    : m_scene(scene)
{
}

ABCOScene::~ABCOScene()
{
    close();
}

bool ABCOScene::open(const char* /*path*/)
{
    // not supported
    return false;
}

bool ABCOScene::create(const char* path)
{
    try
    {
        m_archive = Abc::OArchive(Alembic::AbcCoreOgawa::WriteArchive(), path);
        m_abc_path = path;
    }
    catch (Alembic::Util::Exception e)
    {
        mqusdDbgPrint("failed to open %s\nexception: %s", path, e.what());
        return false;
    }

    // add dummy time sampling
    auto ts = Abc::TimeSampling(Abc::chrono_t(1.0f / 30.0f), Abc::chrono_t(0.0));
    auto tsi = m_archive.addTimeSampling(ts);

    g_current_scene = this;
    auto top = std::make_shared<AbcGeom::OObject>(m_archive, AbcGeom::kTop, tsi);
    m_objects.push_back(top);
    m_root = new ABCORootNode(top.get());
    registerNode(m_root);

    mqusdDbgPrint("succeeded to open %s\nrecording started", m_abc_path.c_str());
    return true;
}

bool ABCOScene::save()
{
    // not supported
    return true;
}

void ABCOScene::close()
{
    m_root = nullptr;
    m_node_table.clear();
    m_nodes.clear();
    m_objects.clear();
    m_archive.reset();
    m_abc_path.clear();
}

void ABCOScene::read()
{
    // not supported
}

void ABCOScene::write()
{
    g_current_scene = this;
    double time = m_scene->time_current;

    if (m_write_count == 0) {
        for (auto& n : m_nodes)
            n->beforeWrite();
    }
    for (auto& n : m_nodes)
        n->write();
    ++m_write_count;

    if (!std::isnan(time) && time > m_max_time) {
        m_max_time = time;
    }
}

void ABCOScene::registerNode(ABCONode* n)
{
    if (n) {
        m_nodes.push_back(ABCONodePtr(n));
        m_node_table[n->getPath()] = n;

        m_scene->nodes.push_back(NodePtr(n->m_node));
        if (n->m_node->getType() == Node::Type::Root)
            m_scene->root_node = static_cast<RootNode*>(n->m_node);
    }
}

template<class NodeT>
inline ABCONode* ABCOScene::createNodeImpl(ABCONode* parent, const char* name)
{
    auto abc = std::make_shared<NodeT::AbcType>(*parent->m_obj, SanitizeNodeName(name));
    if (abc->valid()) {
        m_objects.push_back(abc);
        return new NodeT(parent, abc.get());
    }
    return nullptr;
}

Node* ABCOScene::createNode(Node* parent_, const char* name, Node::Type type)
{
    auto parent = parent_ ? (ABCONode*)parent_->impl : nullptr;
    ABCONode* ret = nullptr;
    switch (type) {
    case Node::Type::Mesh: ret = createNodeImpl<ABCOMeshNode>(parent, name); break;
    case Node::Type::Xform: ret = createNodeImpl<ABCOXformNode>(parent, name); break;
    case Node::Type::Material: ret = createNodeImpl<ABCOMaterialNode>(parent, name); break;
    default: ret = createNodeImpl<ABCONode>(parent, name); break;
    }

    registerNode(ret);
    return ret ? ret->m_node : nullptr;
}

bool ABCOScene::wrapNode(Node* /*node*/)
{
    // not supported
    return false;
}

ABCONode* ABCOScene::findNode(const std::string& path)
{
    auto it = m_node_table.find(path);
    return it == m_node_table.end() ? nullptr : it->second;
}


ScenePtr CreateABCOScene()
{
    auto ret = new Scene();
    ret->impl.reset(new ABCOScene(ret));
    return ScenePtr(ret);
}

} // namespace mqusd
