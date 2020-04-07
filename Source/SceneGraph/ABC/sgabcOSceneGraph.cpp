#include "pch.h"
#include "sgabcOSceneGraph.h"

namespace sg {

template<class NodeT>
static inline NodeT* CreateNode(ABCONode* parent, Abc::OObject* obj)
{
    return new NodeT(
        parent ? parent->m_node : nullptr,
        obj->getName().c_str());
}

template<class T>
static inline void PadSamples(Abc::OTypedArrayProperty<T>& dst, uint32_t n)
{
    using SampleT = typename Abc::OTypedArrayProperty<T>::sample_type;
    while (dst.getNumSamples() < n)
        dst.set(SampleT());
}

template<class T>
static inline void PadSamples(T& dst, uint32_t n)
{
    using SampleT = typename T::Sample;
    while (dst.getNumSamples() < n)
        dst.set(SampleT());
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

void ABCONode::write(double /*t*/)
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

void ABCOXformNode::write(double t)
{
    super::write(t);
    const auto& src = *getNode<XformNode>();

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

void ABCOMeshNode::write(double t)
{
    super::write(t);
    const auto& src = *getNode<MeshNode>();
    uint32_t wc = m_scene->getWriteCount();

    m_sample.reset();
    m_sample.setFaceIndices(Abc::Int32ArraySample(src.indices.cdata(), src.indices.size()));
    m_sample.setFaceCounts(Abc::Int32ArraySample(src.counts.cdata(), src.counts.size()));
    m_sample.setPositions(Abc::P3fArraySample((const abcV3*)src.points.cdata(), src.points.size()));
    {
        m_normals.setVals(Abc::V3fArraySample((const abcV3*)src.normals.cdata(), src.normals.size()));
        m_sample.setNormals(m_normals);
    }
    {
        m_uvs.setVals(Abc::V2fArraySample((const abcV2*)src.uvs.cdata(), src.uvs.size()));
        m_sample.setUVs(m_uvs);
    }
    m_schema.set(m_sample);

    if(!src.colors.empty()){
        if (!m_rgba_param)
            m_rgba_param = AbcGeom::OC4fGeomParam(m_schema.getArbGeomParams(), mqabcAttrVertexColor, false, AbcGeom::GeometryScope::kFacevaryingScope, 1, 1);

        PadSamples(m_rgba_param, wc);
        m_rgba.setVals(Abc::C4fArraySample((const abcC4*)src.colors.cdata(), src.colors.size()));
        m_rgba_param.set(m_rgba);
    }

    if (!src.material_ids.empty()) {
        if (!m_mids_prop.valid())
            m_mids_prop = AbcGeom::OInt32ArrayProperty(m_schema.getArbGeomParams(), mqabcAttrMaterialID, 1);

        PadSamples(m_mids_prop, wc);
        m_mids_prop.set(Abc::Int32ArraySample(src.material_ids.cdata(), src.material_ids.size()));
    }
    else if (!src.facesets.empty()) {
        for (auto& fs : src.facesets) {
            auto mat = fs->material;
            if (!mat)
                continue;

            auto& data = m_facesets[mat->getName()];
            if (!data.faceset) {
                auto ofs = m_schema.createFaceSet(mat->getName());
                data.faceset = ofs.getSchema();
                data.faceset.setTimeSampling(1);
            }
            PadSamples(data.faceset, wc);
            data.sample.setFaces(Abc::Int32ArraySample(fs->faces.cdata(), fs->faces.size()));
            data.faceset.set(data.sample);
        }
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

void ABCOMaterialNode::write(double t)
{
    super::write(t);
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
        sgDbgPrint("failed to open %s\nexception: %s", path, e.what());
        return false;
    }

    // add dummy time sampling
    auto tsi = m_archive.addTimeSampling(Abc::TimeSampling(Abc::chrono_t(1.0f / m_scene->frame_rate), Abc::chrono_t(0.0)));
    g_current_scene = this;
    auto top = std::make_shared<AbcGeom::OObject>(m_archive, AbcGeom::kTop, tsi);
    m_objects.push_back(top);
    m_root = new ABCORootNode(top.get());
    registerNode(m_root);

    sgDbgPrint("succeeded to open %s\nrecording started", m_abc_path.c_str());
    return true;
}

bool ABCOScene::save()
{
    // not supported
    return true;
}

void ABCOScene::close()
{
    if (m_keep_time) {
        auto ts = Abc::TimeSampling(Abc::TimeSamplingType(Abc::TimeSamplingType::kAcyclic), m_timeline);
        *m_archive.getTimeSampling(1) = ts;
    }

    m_root = nullptr;
    m_node_table.clear();
    m_nodes.clear();
    m_objects.clear();
    m_archive.reset();
    m_abc_path.clear();
    m_timeline.clear();
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
        n->write(time);
    ++m_write_count;

    if (!std::isnan(time)) {
        if (m_keep_time)
            m_timeline.push_back(time);
        if (time > m_max_time)
            m_max_time = time;
    }
}

void ABCOScene::registerNode(ABCONode* n)
{
    if (n) {
        m_nodes.push_back(ABCONodePtr(n));
        m_node_table[n->getPath()] = n;
        m_scene->registerNode(n->m_node);
    }
}

template<class NodeT>
inline ABCONode* ABCOScene::createNodeImpl(ABCONode* parent, const char* name)
{
    auto abc = std::make_shared<typename NodeT::AbcType>(*parent->m_obj, parent->m_node->makeUniqueName(name));
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

#ifdef mqusdDebug
    sgDbgPrint("%s\n", ret->getPath().c_str());
#endif
    registerNode(ret);
    return ret ? ret->m_node : nullptr;
}

bool ABCOScene::wrapNode(Node* /*node*/)
{
    // not supported
    return false;
}

uint32_t ABCOScene::getWriteCount() const
{
    return m_write_count;
}

ABCONode* ABCOScene::findABCNodeImpl(const std::string& path)
{
    auto it = m_node_table.find(path);
    return it == m_node_table.end() ? nullptr : it->second;
}

Node* ABCOScene::findNodeImpl(const std::string& path)
{
    if (ABCONode* n = findABCNodeImpl(path))
        return n->m_node;
    return nullptr;
}


ScenePtr CreateABCOScene()
{
    auto ret = new Scene();
    ret->impl.reset(new ABCOScene(ret));
    return ScenePtr(ret);
}

} // namespace sg
