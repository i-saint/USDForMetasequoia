#include "pch.h"
#include "sgabcISceneGraph.h"

namespace sg {

template<class NodeT>
static inline NodeT* CreateNode(ABCINode* parent, Abc::IObject obj)
{
    return new NodeT(
        parent ? parent->m_node : nullptr,
        obj.getName().c_str());
}

template<class NodeT>
static inline NodeT* FindParent(ABCINode *n)
{
    for (auto* p = n->m_parent; p != nullptr; p = p->m_parent) {
        if (auto tp = dynamic_cast<NodeT*>(p))
            return tp;
    }
    return nullptr;
}

template<class PropT>
static bool FindProperty(PropT& dst, const Abc::ICompoundProperty& parent, const char* name) {
    if (!parent)
        return false;
    size_t n = parent.getNumProperties();
    for (size_t i = 0; i < n; ++i) {
        auto& header = parent.getPropertyHeader(i);
        if (PropT::matches(header) && (!name || header.getName() == name)) {
            dst = PropT(parent, header.getName());
            return true;
        }
    }
    return false;
}

static void GetValue(const Abc::IBoolProperty& prop, bool& dst, const Abc::ISampleSelector& iss)
{
    if (!prop || prop.getNumSamples() == 0)
        return;
    Abc::bool_t tmp;
    prop.get(tmp, iss);
    dst = tmp;
}
static void GetValue(const Abc::IFloatProperty& prop, float& dst, const Abc::ISampleSelector& iss)
{
    if (!prop || prop.getNumSamples() == 0)
        return;
    prop.get(dst, iss);
}
static void GetValue(const Abc::IC3fProperty& prop, float3& dst, const Abc::ISampleSelector& iss)
{
    if (!prop || prop.getNumSamples() == 0)
        return;
    prop.get((abcC3&)dst, iss);
}
static void GetValue(const Abc::IStringProperty& prop, TexturePtr& dst, const Abc::ISampleSelector& iss)
{
    if (!prop || prop.getNumSamples() == 0)
        return;
    if (!dst)
        dst = std::make_shared<Texture>();
    prop.get(dst->file_path, iss);
}

static void UpdateGlobalMatrix(XformNode& n)
{
    if (n.parent_xform)
        n.global_matrix = n.local_matrix * n.parent_xform->global_matrix;
    else
        n.global_matrix = n.local_matrix;
}



ABCINode::ABCINode(ABCINode* parent, Abc::IObject& obj, bool create_node)
    : m_obj(obj)
    , m_scene(ABCIScene::getCurrent())
    , m_parent(parent)
{
    if (m_parent)
        m_parent->m_children.push_back(this);
    if (create_node)
        setNode(CreateNode<Node>(parent, obj));
}

ABCINode::~ABCINode()
{
}

void ABCINode::beforeRead()
{
}

void ABCINode::read(double /*time*/)
{
}

void ABCINode::setNode(Node* node)
{
    m_node = node;
    m_node->impl = this;
}

std::string ABCINode::getPath() const
{
    return m_obj.getFullName();
}


ABCIRootNode::ABCIRootNode(Abc::IObject obj)
    : super(nullptr, obj, false)
{
    setNode(new RootNode());
}


ABCIXformNode::ABCIXformNode(ABCINode* parent, Abc::IObject& obj)
    : super(parent, obj, false)
{
    m_schema = AbcGeom::IXform(m_obj).getSchema();
    setNode(CreateNode<XformNode>(parent, obj));
}

void ABCIXformNode::read(double time)
{
    super::read(time);
    auto& dst = *getNode<XformNode>();

    if (m_schema.getNumSamples() == 0)
        return;

    Abc::ISampleSelector iss(time);
    m_schema.get(m_sample, iss);

    auto matd = m_sample.getMatrix();
    dst.local_matrix.assign((double4x4&)matd);

    UpdateGlobalMatrix(dst);
}


ABCIMeshNode::ABCIMeshNode(ABCINode* parent, Abc::IObject& obj)
    : super(parent, obj, false)
{
    m_schema = AbcGeom::IPolyMesh(m_obj).getSchema();
    setNode(CreateNode<MeshNode>(parent, obj));
}

void ABCIMeshNode::beforeRead()
{
    super::beforeRead();
    auto& dst = *getNode<MeshNode>();

    // find vertex color and material ids params
    auto params = m_schema.getArbGeomParams();
    if (!FindProperty(m_rgba_param, params, nullptr))
        FindProperty(m_rgb_param, params, nullptr);
    FindProperty(m_mids_prop, params, sgabcAttrMaterialID);

    // face sets & materials
    {
        std::vector<std::string> fs_names;
        m_schema.getFaceSetNames(fs_names);
        m_facesets.resize(fs_names.size());
        dst.facesets.resize(fs_names.size());
        each_with_index(fs_names, [this, &dst](std::string& name, int i) {
            auto& data = m_facesets[i];
            data.faceset = m_schema.getFaceSet(name).getSchema();

            auto& faceset = dst.facesets[i];
            if (!faceset)
                faceset = std::make_shared<FaceSet>();
            data.dst = faceset;

            AbcGeom::IStringProperty binding;
            if (FindProperty(binding, data.faceset.getArbGeomParams(), sgabcAttrMaterialBinding)) {
                std::string path;
                binding.get(path);
                faceset->material = m_scene->findNode<MaterialNode>(path);
            }
        });
    }
}

void ABCIMeshNode::read(double time)
{
    super::read(time);
    auto& dst = *getNode<MeshNode>();

    // alembic's mesh is not xformable, but USD's and our intermediate data is.
    // so, need to update global matrix.
    UpdateGlobalMatrix(dst);

    if (m_schema.getNumSamples() == 0)
        return;

    Abc::ISampleSelector iss(time);
    m_schema.get(m_sample, iss);

    {
        // m_sample holds counts/indices/points sample pointers. so, no need to hold these by myself.
        auto counts = m_sample.getFaceCounts();
        dst.counts.share(counts->get(), counts->size());

        auto indices = m_sample.getFaceIndices();
        dst.indices.share(indices->get(), indices->size());

        auto points = m_sample.getPositions();
        dst.points.share((float3*)points->get(), points->size());
    }

    auto get_expanded_data = [this, &iss, &dst](auto param, auto& sample, auto& dst_data) -> bool {
        if (!param || param.getNumSamples() == 0)
            return false;

        using src_t = typename std::remove_reference_t<decltype(param)>::value_type;
        using dst_t = typename std::remove_reference_t<decltype(dst_data)>::value_type;
        static_assert(sizeof(src_t) == sizeof(dst_t), "data size mismatch");

        param.getExpanded(sample, iss);
        const auto& values = *sample.getVals();

        size_t nindices = dst.indices.size();
        const auto* src = (const dst_t*)values.get();
        if (values.size() == nindices) {
            dst_data.share(src, values.size());
            return true;
        }
        else if (values.size() == dst.points.size()) {
            dst_data.resize_discard(nindices);
            mu::CopyWithIndices(dst_data.data(), src, dst.indices);
            return true;
        }
        else {
            return false;
        }
    };

    // normals
    get_expanded_data(m_schema.getNormalsParam(), m_normals, dst.normals);

    // uvs
    get_expanded_data(m_schema.getUVsParam(), m_uvs, dst.uvs);

    // colors
    if (!get_expanded_data(m_rgba_param, m_rgba, dst.colors)) {
        // convert rgb to rgba if the param exists
        if (get_expanded_data(m_rgb_param, m_rgb, m_tmp_rgb))
            transform_container(dst.colors, m_tmp_rgb, [](float4& d, const float3& s) { d = to_vec4(s, 1.0f); });
    }

    // material ids
    if (m_mids_prop && m_mids_prop.getNumSamples() != 0) {
        m_mids_prop.get(m_material_ids, iss);
        dst.material_ids.share(m_material_ids->get(), m_material_ids->size());
    }
    else {
        dst.facesets.resize(m_facesets.size());
        each_with_index(m_facesets, [&dst, &iss](auto& fs, int i) {
            fs.faceset.get(fs.sample, iss);
            auto sp = fs.sample.getFaces();
            fs.dst->faces.share(sp->get(), sp->size());
            dst.facesets[i] = fs.dst;
        });
    }

    // validate
    dst.validate();
}


ABCIMaterialNode::ABCIMaterialNode(ABCINode* parent, Abc::IObject& obj)
    : super(parent, obj, false)
{
    m_schema = AbcMaterial::IMaterial(m_obj).getSchema();
    setNode(CreateNode<MaterialNode>(parent, obj));
}

void ABCIMaterialNode::beforeRead()
{
    super::beforeRead();
    auto& dst = *getNode<MaterialNode>();

    std::vector<std::string> shader_types;
    m_schema.getShaderTypesForTarget(mqabcMaterialTarget, shader_types);

    if (!shader_types.empty()) {
        ShaderType st = ToShaderType(shader_types.front());
        if (st != ShaderType::Unknown) {
            dst.shader_type = st;
            m_shader_params = m_schema.getShaderParameters(mqabcMaterialTarget, shader_types.front());

            auto find_prop = [this](auto& prop, const char* name) {
                return FindProperty(prop, m_shader_params, name);
            };

            find_prop(m_use_vertex_color_prop, sgabcAttrUseVertexColor);
            find_prop(m_double_sided_prop, sgabcAttrDoubleSided);
            find_prop(m_diffuse_color_prop, sgabcAttrDiffuseColor);
            find_prop(m_diffuse_prop, sgabcAttrDiffuse);
            find_prop(m_opacity_prop, sgabcAttrOpacity);
            find_prop(m_roughness_prop, sgabcAttrRoughness);
            find_prop(m_ambient_color_prop, sgabcAttrAmbientColor);
            find_prop(m_specular_color_prop, sgabcAttrSpecularColor);
            find_prop(m_emissive_color_prop, sgabcAttrEmissiveColor);

            find_prop(m_diffuse_texture_prop, sgabcAttrDiffuseTexture);
            find_prop(m_opacity_texture_prop, sgabcAttrOpacityTexture);
            find_prop(m_bump_texture_prop, sgabcAttrBumpTexture);
        }
    }
}

void ABCIMaterialNode::read(double time)
{
    super::read(time);
    auto& dst = *getNode<MaterialNode>();

    if (!m_shader_params)
        return;

    Abc::ISampleSelector iss(time);
    GetValue(m_use_vertex_color_prop, dst.use_vertex_color, iss);
    GetValue(m_double_sided_prop, dst.double_sided, iss);
    GetValue(m_diffuse_color_prop, dst.diffuse_color, iss);
    GetValue(m_diffuse_prop, dst.diffuse, iss);
    GetValue(m_opacity_prop, dst.opacity, iss);
    GetValue(m_roughness_prop, dst.roughness, iss);
    GetValue(m_ambient_color_prop, dst.ambient_color, iss);
    GetValue(m_specular_color_prop, dst.specular_color, iss);
    GetValue(m_emissive_color_prop, dst.emissive_color, iss);

    GetValue(m_diffuse_texture_prop, dst.diffuse_texture, iss);
    GetValue(m_opacity_texture_prop, dst.opacity_texture, iss);
    GetValue(m_bump_texture_prop, dst.bump_texture, iss);
}


static thread_local ABCIScene* g_current_scene;

ABCIScene* ABCIScene::getCurrent()
{
    return g_current_scene;
}

ABCIScene::ABCIScene(Scene* scene)
    : m_scene(scene)
{
}

ABCIScene::~ABCIScene()
{
    close();
}

bool ABCIScene::open(const char* path)
{
    try
    {
        // Abc::IArchive doesn't accept wide string path. so create file stream with wide string path and pass it.
        // (VisualC++'s std::ifstream accepts wide string)
        m_stream.reset(new std::fstream());
#ifdef WIN32
        auto wpath = mu::ToWCS(path);
        m_stream->open(wpath.c_str(), std::ios::in | std::ios::binary);
#else
        m_stream->open(path, std::ios::in | std::ios::binary);
#endif
        if (!m_stream->is_open()) {
            close();
            return false;
        }

        std::vector< std::istream*> streams{ m_stream.get() };
        Alembic::AbcCoreOgawa::ReadArchive archive_reader(streams);
        m_archive = Abc::IArchive(archive_reader(path), Abc::kWrapExisting, Abc::ErrorHandler::kThrowPolicy);
    }
    catch (Alembic::Util::Exception e)
    {
        close();

#ifdef mqabcEnableHDF5
        try
        {
            m_archive = Abc::IArchive(AbcCoreHDF5::ReadArchive(), path);
        }
        catch (Alembic::Util::Exception e2)
        {
            close();
            sgDbgPrint(
                "failed to open %s\n"
                "it may not an alembic file"
                , path);
        }
#else
        sgDbgPrint(
            "failed to open %s\n"
            "it may not an alembic file or not in Ogawa format (HDF5 is not supported)"
            , path);
#endif
    }

    if (m_archive.valid()) {
        try {
            g_current_scene = this;
            m_abc_path = path;
            m_root = new ABCIRootNode(m_archive.getTop());
            m_scene->root_node = m_root->getNode<RootNode>();
            constructTree(m_root);
            for (auto& node : m_nodes)
                node->beforeRead();
        }
        catch (Alembic::Util::Exception e3) {
            close();
            sgDbgPrint(
                "failed to read %s\n"
                "it seems an alembic file but probably broken"
                , path);
        }
    }

    if (!m_archive.valid())
        return false;

    setupTimeRange();
    return true;
}

bool ABCIScene::create(const char* /*path*/)
{
    // not supported
    return false;
}

bool ABCIScene::save()
{
    // not supported
    return true;
}

void ABCIScene::close()
{
    m_root = nullptr;
    m_node_table.clear();
    m_nodes.clear();

    m_archive.reset();
    m_stream.reset();
    m_abc_path.clear();
}

void ABCIScene::read()
{
    g_current_scene = this;
    double time = m_scene->time_current;
    if (IsDefaultTime(time))
        time = 0.0;

    for (auto& n : m_nodes)
        n->read(time);
    ++m_read_count;
}

void ABCIScene::write()
{
    // not supported
}

Node* ABCIScene::createNode(Node* /*parent*/, const char* /*name*/, Node::Type /*type*/)
{
    // not supported
    return nullptr;
}

bool ABCIScene::wrapNode(Node* /*node*/)
{
    // not supported
    return false;
}

ABCINode* ABCIScene::findABCNodeImpl(const std::string& path)
{
    auto it = m_node_table.find(path);
    return it == m_node_table.end() ? nullptr : it->second;
}

Node* ABCIScene::findNodeImpl(const std::string& path)
{
    if (ABCINode* n = findABCNodeImpl(path))
        return n->m_node;
    return nullptr;
}

void ABCIScene::setupTimeRange()
{
    bool first = true;

    uint32_t nt = m_archive.getNumTimeSamplings();
    for (uint32_t ti = 1; ti < nt; ++ti) {
        double time_start = -1.0;
        double time_end = -1.0;
        double frame_rate = 1.0;

        auto ts = m_archive.getTimeSampling(ti);
        auto tst = ts->getTimeSamplingType();
        if (tst.isUniform() || tst.isCyclic()) {
            auto start = ts->getStoredTimes()[0];
            auto max_num_samples = m_archive.getMaxNumSamplesForTimeSamplingIndex(ti);
            auto samples_per_cycle = tst.getNumSamplesPerCycle();
            auto time_per_cycle = tst.getTimePerCycle();
            auto num_cycles = max_num_samples / samples_per_cycle;

            if (tst.isUniform()) {
                time_start = start;
                time_end = num_cycles > 0 ? start + (time_per_cycle * (num_cycles - 1)) : start;
                frame_rate = 1.0 / time_per_cycle;
            }
            else if (tst.isCyclic()) {
                auto& s = ts->getStoredTimes();
                if (!s.empty()) {
                    time_start = start + (s.front() - time_per_cycle);
                    time_end = start + (s.back() - time_per_cycle) + (time_per_cycle * num_cycles);
                    frame_rate = (double)(s.size() * num_cycles) / (time_end - time_start);
                }
            }
        }
        else if (tst.isAcyclic()) {
            auto& s = ts->getStoredTimes();
            if (!s.empty()) {
                time_start = s.front();
                time_end = s.back();
                frame_rate = (double)s.size() / (time_end - time_start);
            }
        }

        if (time_start != -1.0) {
            if (first) {
                first = false;
                m_scene->time_start = time_start;
                m_scene->time_end = time_end;
                m_scene->frame_rate = frame_rate;
            }
            else {
                m_scene->time_start = std::min(m_scene->time_start, time_start);
                m_scene->time_end = std::max(m_scene->time_end, time_end);
                m_scene->frame_rate = std::max(m_scene->frame_rate, frame_rate);
            }
        }
    }
}

void ABCIScene::registerNode(ABCINode* n)
{
    if (n) {
        m_nodes.push_back(ABCINodePtr(n));
        m_node_table[n->getPath()] = n;
        m_scene->registerNode(n->m_node);
    }
}

void ABCIScene::constructTree(ABCINode* n)
{
#ifdef mqusdDebug
    sgDbgPrint("%s\n", n->getPath().c_str());
#endif
    registerNode(n);

    auto& obj = n->m_obj;
    size_t nchildren = obj.getNumChildren();
    for (size_t ci = 0; ci < nchildren; ++ci) {
        auto child = obj.getChild(ci);
        const auto& metadata = child.getMetaData();

        ABCINode* c = nullptr;
        if (AbcGeom::IXformSchema::matches(metadata))
            c = new ABCIXformNode(n, child);
        else if (AbcGeom::IPolyMeshSchema::matches(metadata))
            c = new ABCIMeshNode(n, child);
        else if (AbcMaterial::IMaterialSchema::matches(metadata))
            c = new ABCIMaterialNode(n, child);
        else
            c = new ABCINode(n, child, true);

        constructTree(c);
    }
}

ScenePtr CreateABCIScene()
{
    auto ret = new Scene();
    ret->impl.reset(new ABCIScene(ret));
    return ScenePtr(ret);
}

} // namespace sg
