#include "pch.h"
#include "mqusdDocumentImporter.h"

namespace mqusd {

ImportOptions::ImportOptions()
{
    scale_factor = 20.0f;
    flip_v = true;
}

bool ImportOptions::operator==(const ImportOptions& v) const
{
    return
        ConvertOptions::operator==(v) &&
        import_blendshapes == v.import_blendshapes &&
        import_skeletons == v.import_skeletons &&
        import_materials == v.import_materials &&
        merge_meshes == v.merge_meshes &&
        bake_meshes == v.bake_meshes;
}

bool ImportOptions::operator!=(const ImportOptions& v) const
{
    return !(*this == v);
}

bool ImportOptions::mayChangeAffectsNodeStructure(const ImportOptions& v) const
{
    return
        import_blendshapes != v.import_blendshapes ||
        import_skeletons != v.import_skeletons ||
        import_materials != v.import_materials ||
        merge_meshes != v.merge_meshes ||
        bake_meshes != v.bake_meshes;
}


DocumentImporter::DocumentImporter(MQBasePlugin* plugin, Scene* scene, const ImportOptions* options)
    : m_plugin(plugin)
    , m_scene(scene)
    , m_options(options)
{
}

bool DocumentImporter::initialize(MQDocument doc)
{
    {
        auto mesh_nodes = m_scene->getNodes<MeshNode>();
        size_t nobjs = mesh_nodes.size();
        m_obj_records.resize(nobjs);
        for (size_t oi = 0; oi < nobjs; ++oi) {
            auto& rec = m_obj_records[oi];
            rec.node = mesh_nodes[oi];
            rec.node->userdata = &rec;
            rec.blendshape_ids.resize(rec.node->blendshapes.size());
        }
    }
    {
        auto inst_nodes = m_scene->getNodes<InstancerNode>();
        size_t ninsts = inst_nodes.size();
        m_inst_records.resize(ninsts);
        for (size_t oi = 0; oi < ninsts; ++oi) {
            auto& rec = m_inst_records[oi];
            rec.node = inst_nodes[oi];
            rec.node->userdata = &rec;
        }
    }
    {
        auto skel_nodes = m_scene->getNodes<SkeletonNode>();
        size_t nskels = skel_nodes.size();
        m_skel_records.resize(nskels);
        for (size_t si = 0; si < nskels; ++si) {
            auto& rec = m_skel_records[si];
            rec.node = skel_nodes[si];
            rec.node->userdata = &rec;

            size_t njoints = rec.node->joints.size();
            rec.joints.resize(njoints);
            for (size_t ji = 0; ji < njoints; ++ji) {
                auto& jrec = rec.joints[ji];
                jrec.joint = rec.node->joints[ji].get();
                jrec.joint->userdata = &jrec;
            }
        }
    }

    if (m_options->import_materials)
        updateMaterials(doc);

    read(doc, mqusd::default_time);
    return true;
}

std::string DocumentImporter::makeUniqueObjectName(MQDocument doc, const std::string& name)
{
    std::string base = name;
    std::string ret = base;
    for (int i = 1; ; ++i) {
        bool ok = true;
        each_object(doc, [&ret, &ok](MQObject obj) {
            char buf[512];
            obj->GetName(buf, sizeof(buf));
            if (ret == buf) {
                ok = false;
                return false;
            }
            return true;
        });
        if (ok)
            break;

        char buf[16];
        sprintf(buf, "_%d", i);
        ret = base + buf;
    }
    return ret;
}

std::string DocumentImporter::makeUniqueMaterialName(MQDocument doc, const std::string& name)
{
    std::string base = name;
    std::string ret = base;
    for (int i = 1; ; ++i) {
        bool ok = true;
        each_material(doc, [&ret, &ok](MQMaterial obj) {
            char buf[512];
            obj->GetName(buf, sizeof(buf));
            if (ret == buf) {
                ok = false;
                return false;
            }
            return true;
        });
        if (ok)
            break;

        char buf[16];
        sprintf(buf, "_%d", i);
        ret = base + buf;
    }
    return ret;
}

DocumentImporter::ObjectRecord* DocumentImporter::findRecord(UINT mqid)
{
    if (mqid == 0)
        return nullptr;
    auto it = std::find_if(m_obj_records.begin(), m_obj_records.end(), [mqid](auto& rec) {
        return rec.mqid == mqid;
    });
    return it == m_obj_records.end() ? nullptr : &(*it);
}

MQObject DocumentImporter::findOrCreateMQObject(MQDocument doc, UINT& id, UINT parent_id, bool& created)
{
    MQObject mqo = id != 0 ? doc->GetObjectFromUniqueID(id) : nullptr;
    if (!mqo) {
        mqo = MQ_CreateObject();
        doc->AddObject(mqo);
        id = mqo->GetUniqueID();
        created = true;

        if (auto prec = findRecord(parent_id)) {
            auto parent = doc->GetObjectFromUniqueID(prec->mqid);
            mqo->SetDepth(parent->GetDepth() + 1);
        }
    }
    else {
        created = false;
    }
    return mqo;
}

bool DocumentImporter::deleteMQObject(MQDocument doc, UINT id)
{
    MQObject mqo = id != 0 ? doc->GetObjectFromUniqueID(id) : nullptr;
    if (mqo) {
        doc->DeleteObject(doc->GetObjectIndex(mqo));
        return true;
    }
    return false;
}

bool DocumentImporter::read(MQDocument doc, double t)
{
    if (!m_scene)
        return false;

    if (m_options->mayChangeAffectsNodeStructure(m_prev_options)) {
        // clear existing objects when option changed
        for (auto& rec : m_obj_records) {
            deleteMQObject(doc, rec.mqid);
            rec.mqid = 0;
            for (UINT& bsid : rec.blendshape_ids) {
                deleteMQObject(doc, bsid);
                bsid = 0;
            }
        }
        for (auto& rec : m_inst_records) {
            deleteMQObject(doc, rec.mqid);
            rec.mqid = 0;
        }
        {
            deleteMQObject(doc, m_merged_mqobj_id);
            m_merged_mqobj_id = 0;
        }
        doc->Compact();
    }
    m_option_changed = *m_options != m_prev_options;
    m_prev_options = *m_options;
    m_prev_time = t;


    // read scene
    m_scene->read(t);

    // convert
    mu::parallel_for_each(m_scene->nodes.begin(), m_scene->nodes.end(), [this](NodePtr& n) {
        n->convert(*m_options);
    });

    auto mesh_nodes = m_scene->getNodes<MeshNode>();
    mu::parallel_for_each(mesh_nodes.begin(), mesh_nodes.end(), [this](MeshNode* n) {
        n->buildMaterialIDs();
    });

    // reserve materials
    {
        int nmaterials = 0;
        for (auto n : mesh_nodes)
            nmaterials = std::max(nmaterials, n->getMaxMaterialID());

        int mi = 0;
        while (doc->GetMaterialCount() <= nmaterials) {
            const size_t buf_len = 128;
            wchar_t buf[buf_len];
            swprintf(buf, buf_len, L"mat%d", mi++);

            auto mat = MQ_CreateMaterial();
            mat->SetName(buf);
            doc->AddMaterial(mat);
        }
    }

#if MQPLUGIN_VERSION >= 0x0470
    MQMorphManager morph_manager(m_plugin, doc);
    if (m_options->import_blendshapes) {
        m_morph_manager = &morph_manager;
    }

    MQBoneManager bone_manager(m_plugin, doc);
    if (m_options->import_skeletons) {
        m_bone_manager = &bone_manager;
        m_bone_manager->BeginImport();
    }
#endif

    // skeleton
    if (!m_options->bake_meshes && m_options->import_skeletons) {
        for (auto& rec : m_skel_records)
            updateSkeleton(doc, *rec.node);
    }

    auto bake_mesh = [](MeshNode& dst, MeshNode* n) {
        if (!n->skeleton)
            n->toWorldSpace();
        n->bake(dst);
    };

    auto merge_mesh = [](MeshNode& dst, MeshNode* n) {
        n->toWorldSpace();
        dst.merge(*n);
    };

    // update mq object
    if (m_options->merge_meshes) {
        // build merged mesh
        m_merged_mesh.clear();
        if (m_options->bake_meshes) {
            for (auto n : mesh_nodes)
                bake_mesh(m_merged_mesh, n);
        }
        else {
            for (auto n : mesh_nodes)
                m_merged_mesh.merge(*n);
        }
        for (auto& rec : m_inst_records) {
            rec.node->bake(m_merged_mesh, rec.node->global_matrix);
        }
        m_merged_mesh.validate();

        bool created;
        auto obj = findOrCreateMQObject(doc, m_merged_mqobj_id, 0, created);
        if (created) {
            auto name = mu::GetFilename_NoExtension(m_scene->path.c_str());
            obj->SetName(makeUniqueObjectName(doc, name).c_str());
        }

        updateMesh(doc, obj, m_merged_mesh);
    }
    else {
        auto handle_blendshape = [this, doc](ObjectRecord& rec, MQObject obj) {
            size_t nbs = rec.node->blendshapes.size();
            for (size_t bi = 0; bi < nbs; ++bi) {
                auto blendshape = rec.node->blendshapes[bi];
                blendshape->makeMesh(rec.tmp_mesh, *rec.node);

                bool created;
                auto bs = findOrCreateMQObject(doc, rec.blendshape_ids[bi], rec.mqid, created);
                updateMesh(doc, bs, rec.tmp_mesh);
                if (created) {
                    bs->SetName(makeUniqueObjectName(doc, blendshape->getName()).c_str());
                    bs->SetVisible(0);
#if MQPLUGIN_VERSION >= 0x0470
                    m_morph_manager->BindTargetObject(obj, bs);
#endif
                }
            }
        };

        auto handle_mqobject = [this, doc](auto& rec) {
            UINT parent_id = 0;
            if (auto pmesh = rec.node->findParent<MeshNode>())
                parent_id = ((ObjectRecord*)pmesh->userdata)->mqid;

            bool created;
            auto obj = findOrCreateMQObject(doc, rec.mqid, parent_id, created);
            if (created)
                obj->SetName(makeUniqueObjectName(doc, rec.node->getName()).c_str());
            return obj;
        };

        for (auto& rec : m_obj_records) {
            auto obj = handle_mqobject(rec);
            if (m_options->bake_meshes) {
                rec.tmp_mesh.clear();
                bake_mesh(rec.tmp_mesh, rec.node);
                updateMesh(doc, obj, rec.tmp_mesh);
            }
            else {
                rec.node->toWorldSpace();
                updateMesh(doc, obj, *rec.node);
                // blendshapes
                if (m_options->import_blendshapes)
                    handle_blendshape(rec, obj);
            }
        }

        for (auto& rec : m_inst_records) {
            auto obj = handle_mqobject(rec);
            rec.tmp_mesh.clear();
            rec.node->bake(rec.tmp_mesh, rec.node->global_matrix);
            updateMesh(doc, obj, rec.tmp_mesh);
        }
    }

#if MQPLUGIN_VERSION >= 0x0470
    if (m_options->import_skeletons)
        m_bone_manager->EndImport();
    m_bone_manager = nullptr;
    m_morph_manager = nullptr;
#endif
    return true;
}

bool DocumentImporter::updateMesh(MQDocument /*doc*/, MQObject obj, const MeshNode& src)
{
    // transform
    {
        float3 t; quatf r; float3 s;
        mu::extract_trs(src.global_matrix, t, r, s);
        obj->SetTranslation(to_point(t));
        obj->SetRotation(to_angle(r));
        obj->SetScaling(to_point(s));
    }


    int npoints = (int)src.points.size();
    int nfaces = (int)src.counts.size();

    if (m_option_changed || obj->GetVertexCount() != npoints || obj->GetFaceCount() != nfaces) {
        // option or topology changed. re-create mesh.
        obj->Clear();

        // reserve space
        obj->ReserveVertex(npoints);
        obj->ReserveFace(nfaces);

        // points
        for (auto& p : src.points)
            obj->AddVertex((MQPoint&)p);

        // faces
        {
            auto* data = (int*)src.indices.cdata();
            for (auto c : src.counts) {
                obj->AddFace(c, data);
                data += c;
            }
        }
    }
    else {
        // just update existing vertices.
        for (int pi = 0; pi < npoints; ++pi)
            obj->SetVertex(pi, (MQPoint&)src.points[pi]);
    }

    // uvs
    if (!src.uvs.empty()) {
        auto* data = (MQCoordinate*)src.uvs.cdata();
        for (int fi = 0; fi < nfaces; ++fi) {
            int c = src.counts[fi];
            obj->SetFaceCoordinateArray(fi, data);
            data += c;
        }
    }

    // normals
#if MQPLUGIN_VERSION >= 0x0460
    if (!src.normals.empty()) {
        auto* data = (const MQPoint*)src.normals.cdata();
        for (int fi = 0; fi < nfaces; ++fi) {
            int c = src.counts[fi];
            for (int ci = 0; ci < c; ++ci)
                obj->SetFaceVertexNormal(fi, ci, MQOBJECT_NORMAL_NONE, data[ci]);
            data += c;
        }
    }
#endif

    // colors
    if (!src.colors.empty()) {
        auto* data = src.colors.cdata();
        for (int fi = 0; fi < nfaces; ++fi) {
            int c = src.counts[fi];
            for (int ci = 0; ci < c; ++ci)
                obj->SetFaceVertexColor(fi, ci, mu::Float4ToColor32(data[ci]));
            data += c;
        }
    }

    // material ids
    if (!src.material_ids.empty()) {
        auto* data = src.material_ids.cdata();
        for (int fi = 0; fi < nfaces; ++fi)
            obj->SetFaceMaterial(fi, data[fi]);
    }

#if MQPLUGIN_VERSION >= 0x0470
    // bone weights
    if (!m_options->bake_meshes && m_options->import_skeletons && src.skeleton) {
        m_bone_manager->AddSkinObject(obj);

        std::vector<UINT> joint_ids;
        if (!src.joint_paths.empty()) {
            for (auto& path : src.joint_paths) {
                auto joint = src.skeleton->findJointByPath(path);
                if (joint)
                    joint_ids.push_back(((JointRecord*)joint->userdata)->mqid);
                else
                    joint_ids.push_back(0); // should not be here
            }
        }
        else {
            for (auto& joint : src.skeleton->joints) {
                joint_ids.push_back(((JointRecord*)joint->userdata)->mqid);
            }
        }

        int jpv = src.joints_per_vertex;
        auto* indices = src.joint_indices.cdata();
        auto* weights = src.joint_weights.cdata();

        auto assign_weight = [&](int pi) {
            for (int ji = 0; ji < jpv; ++ji) {
                float weight = weights[ji];
                if (weight > 0) {
                    m_bone_manager->SetVertexWeight(obj, obj->GetVertexUniqueID(pi), joint_ids[indices[ji]], weight);
                }
            }
        };

        if (src.joint_indices.size() == jpv && src.joint_weights.size() == jpv) {
            for (int pi = 0; pi < npoints; ++pi) {
                assign_weight(pi);
            }
        }
        else if (src.joint_indices.size() == npoints * jpv && src.joint_weights.size() == npoints * jpv) {
            for (int pi = 0; pi < npoints; ++pi) {
                assign_weight(pi);
                indices += jpv;
                weights += jpv;
            }
        }
        else {
            // invalid data
        }
    }
#endif

    return true;
}

bool DocumentImporter::updateSkeleton(MQDocument /*doc*/, const SkeletonNode& src)
{
#if MQPLUGIN_VERSION >= 0x0470
    m_bone_manager->BeginImport();

    auto& rec = *(SkeletonRecord*)src.userdata;
    size_t njoints = rec.joints.size();
    for (size_t ji = 0; ji < njoints; ++ji) {
        auto& joint = *src.joints[ji];
        auto& jrec = rec.joints[ji];

        float3 t; quatf r; float3 s;
        std::tie(t, r, s) = mu::extract_trs(joint.bindpose);

        if (jrec.mqid == 0) {
            MQBoneManager::ADD_BONE_PARAM param;
            param.name = mu::ToWCS(joint.getName());
            param.pos = (MQPoint&)t;
            if (joint.parent)
                param.parent_id = ((JointRecord*)joint.parent->userdata)->mqid;
            jrec.mqid = m_bone_manager->AddBone(param);
        }
        else {
            m_bone_manager->SetBasePos(jrec.mqid, (MQPoint&)t);
        }
    }

    m_bone_manager->EndImport();
    return true;
#else
    return false;
#endif
}

bool DocumentImporter::updateMaterials(MQDocument doc)
{
    auto material_nodes = m_scene->getNodes<MaterialNode>();
    m_material_records.resize(material_nodes.size());
    each_with_index(material_nodes, [&](MaterialNode* node, int mi) {
        auto& src = *node;
        auto& rec = m_material_records[mi];
        rec.node = node;

        MQMaterial mqmat = rec.mqid != 0 ? doc->GetMaterialFromUniqueID(rec.mqid) : nullptr;
        if (!mqmat) {
            mqmat = MQ_CreateMaterial();
            mqmat->SetName(makeUniqueMaterialName(doc, src.getName()).c_str());
            rec.mqindex = doc->AddMaterial(mqmat);
            rec.mqid = mqmat->GetUniqueID();
            src.index = rec.mqindex;
        }

        int shader = MQMATERIAL_SHADER_LAMBERT;
        switch (src.shader_type) {
        case ShaderType::MQClassic: shader = MQMATERIAL_SHADER_CLASSIC; break;
        case ShaderType::MQConstant:shader = MQMATERIAL_SHADER_CONSTANT; break;
        case ShaderType::MQLambert: shader = MQMATERIAL_SHADER_LAMBERT; break;
        case ShaderType::MQPhong:   shader = MQMATERIAL_SHADER_PHONG; break;
        case ShaderType::MQBlinn:   shader = MQMATERIAL_SHADER_BLINN; break;
        case ShaderType::MQHLSL:    shader = MQMATERIAL_SHADER_HLSL; break;
        default: break;
        }
        mqmat->SetShader(shader);

        mqmat->SetVertexColor(src.use_vertex_color ? MQMATERIAL_VERTEXCOLOR_DIFFUSE : MQMATERIAL_VERTEXCOLOR_DISABLE);
        mqmat->SetDoubleSided(src.double_sided);
        mqmat->SetColor((MQColor&)src.diffuse_color);
        mqmat->SetDiffuse(src.diffuse);
        mqmat->SetAlpha(src.opacity);
        mqmat->SetAmbientColor((MQColor&)src.ambient_color);
        mqmat->SetSpecularColor((MQColor&)src.specular_color);
        mqmat->SetEmissionColor((MQColor&)src.emissive_color);

        auto from_wrap_mode = [](WrapMode v) {
            switch (v) {
            case WrapMode::Repeat: return MQMATERIAL_WRAP_REPEAT;
            case WrapMode::Mirror: return MQMATERIAL_WRAP_MIRROR;
            case WrapMode::Clamp: return MQMATERIAL_WRAP_CLAMP;
            }
            return MQMATERIAL_WRAP_REPEAT;
        };

        if (src.diffuse_texture) {
            mqmat->SetTextureName(src.diffuse_texture->file_path.c_str());
            if (src.diffuse_texture->wrap_s != WrapMode::Unknown)
                mqmat->SetWrapModeU(from_wrap_mode(src.diffuse_texture->wrap_s));
            if (src.diffuse_texture->wrap_t != WrapMode::Unknown)
                mqmat->SetWrapModeV(from_wrap_mode(src.diffuse_texture->wrap_t));
        }
        if (src.opacity_texture)
            mqmat->SetAlphaName(src.opacity_texture->file_path.c_str());
        if (src.bump_texture)
            mqmat->SetBumpName(src.bump_texture->file_path.c_str());
    });
    return true;
}


} // namespace mqusd
