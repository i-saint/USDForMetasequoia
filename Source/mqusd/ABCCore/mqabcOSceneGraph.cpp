#include "pch.h"
#include "mqabcOSceneGraph.h"

namespace mqusd {

template<class NodeT>
static inline NodeT* CreateNode(ABCONode* parent, Abc::OObject obj)
{
    return new NodeT(
        parent ? parent->m_node : nullptr,
        obj.getName().c_str());
}


ABCONode::ABCONode(ABCONode* parent, Abc::OObject obj, bool create_node)
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
    return m_obj.getFullName();
}



ABCORootNode::ABCORootNode(Abc::OObject obj)
    : super(nullptr, obj, false)
{
    setNode(new RootNode());
}


ABCOXformNode::ABCOXformNode(ABCONode* parent, Abc::OObject obj)
    : super(parent, obj, false)
{
    setNode(CreateNode<XformNode>(nullptr, obj));
}

void ABCOXformNode::write()
{
}

ABCOMeshNode::ABCOMeshNode(ABCONode* parent, Abc::OObject obj)
    : super(parent, obj, false)
{
    setNode(CreateNode<MeshNode>(nullptr, obj));
}

void ABCOMeshNode::beforeWrite()
{
}

void ABCOMeshNode::write()
{
}

ABCOMaterialNode::ABCOMaterialNode(ABCONode* parent, Abc::OObject obj)
    : super(parent, obj, false)
{
    setNode(CreateNode<MaterialNode>(nullptr, obj));
}

void ABCOMaterialNode::beforeWrite()
{
}

void ABCOMaterialNode::write()
{
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
    m_root = new ABCORootNode(AbcGeom::OObject(m_archive, AbcGeom::kTop, tsi));
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
        m_scene->nodes.push_back(NodePtr(n->m_node));
        m_nodes.push_back(ABCONodePtr(n));
        m_node_table[n->getPath()] = n;
    }
}

template<class NodeT>
inline ABCONode* ABCOScene::createNodeImpl(ABCONode* parent, const char* name)
{
    auto abc = typename NodeT::AbcType(parent->m_obj, SanitizeNodeName(name));
    if (abc.valid()) {
        return new NodeT(parent, abc);
    }
    return nullptr;
}

Node* ABCOScene::createNode(Node* parent_, const char* name, Node::Type type)
{
    auto parent = (ABCONode*)parent_->impl;
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
