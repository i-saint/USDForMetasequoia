#include "pch.h"
#include "mqDocumentImporter.h"
#include "mqUtils.h"

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

bool DocumentImporter::initialize(MQDocument doc, bool additive)
{
    if (!additive)
        clearDocument(doc);

    m_mesh_nodes = m_scene->getNodes<MeshNode>();
    transform_container(m_obj_records, m_mesh_nodes, [](auto& rec, MeshNode* node) {
        rec.node = node;
        rec.node->userdata = &rec;
        rec.blendshape_ids.resize(rec.node->blendshapes.size());
    });

    transform_container(m_inst_records, m_scene->getNodes<InstancerNode>(), [](auto& rec, InstancerNode* node) {
        rec.node = node;
        rec.node->userdata = &rec;
    });
    
    transform_container(m_skel_records, m_scene->getNodes<SkeletonNode>(), [](auto& rec, SkeletonNode* node) {
        rec.node = node;
        rec.node->userdata = &rec;

        size_t njoints = rec.node->joints.size();
        rec.joints.resize(njoints);
        for (size_t ji = 0; ji < njoints; ++ji) {
            auto& jrec = rec.joints[ji];
            jrec.joint = rec.node->joints[ji].get();
            jrec.joint->userdata = &jrec;
        }
    });

    transform_container(m_material_records, m_scene->getNodes<MaterialNode>(), [](auto& rec, MaterialNode* node) {
        rec.node = node;
        rec.node->userdata = &rec;
    });

    read(doc, mqusd::default_time);
    return true;
}

void DocumentImporter::clearDocument(MQDocument doc)
{
    each_object(doc, [doc](MQObject /*obj*/, int i) {
        doc->DeleteObject(i);
    });
    each_material(doc, [doc](MQMaterial /*mat*/, int i) {
        doc->DeleteMaterial(i);
    });
    doc->Compact();
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

    // materials
    if (m_options->import_materials)
        updateMaterials(doc);

    mu::parallel_for_each(m_mesh_nodes.begin(), m_mesh_nodes.end(), [this](MeshNode* n) {
        n->buildMaterialIDs();
    });

    // reserve materials
    {
        int nmaterials = 0;
        for (auto n : m_mesh_nodes)
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
        if (n) {
            if (!n->skeleton)
                n->toWorldSpace();
            n->bake(dst);
        }
    };

    auto merge_mesh = [](MeshNode& dst, MeshNode* n) {
        if (n) {
            n->toWorldSpace();
            dst.merge(*n);
        }
    };

    // update mq object
    if (m_options->merge_meshes) {
        // build merged mesh
        m_merged_mesh.clear();
        if (m_options->bake_meshes) {
            for (auto n : m_mesh_nodes)
                bake_mesh(m_merged_mesh, n);
        }
        else {
            for (auto n : m_mesh_nodes)
                merge_mesh(m_merged_mesh, n);
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
            if (auto pmesh = rec.node->template findParent<MeshNode>())
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
        for (int fi = 0; fi < nfaces; ++fi) {
            int mid = data[fi];
            if (mid >= 0)
                obj->SetFaceMaterial(fi, mid);
        }
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

static int ToMQShader(ShaderType v)
{
    switch (v) {
    case ShaderType::MQClassic: return MQMATERIAL_SHADER_CLASSIC;
    case ShaderType::MQConstant:return MQMATERIAL_SHADER_CONSTANT;
    case ShaderType::MQLambert: return MQMATERIAL_SHADER_LAMBERT;
    case ShaderType::MQPhong:   return MQMATERIAL_SHADER_PHONG;
    case ShaderType::MQBlinn:   return MQMATERIAL_SHADER_BLINN;
    case ShaderType::MQHLSL:    return MQMATERIAL_SHADER_HLSL;
    default: return MQMATERIAL_SHADER_LAMBERT;
    }
}

static int ToMQWrapMode(WrapMode v)
{
    switch (v) {
    case WrapMode::Repeat: return MQMATERIAL_WRAP_REPEAT;
    case WrapMode::Mirror: return MQMATERIAL_WRAP_MIRROR;
    case WrapMode::Clamp: return MQMATERIAL_WRAP_CLAMP;
    default: return MQMATERIAL_WRAP_REPEAT;
    }
}

bool DocumentImporter::updateMaterials(MQDocument doc)
{
    // create / update materials
    for (auto& rec : m_material_records) {
        auto& src = *rec.node;

        MQMaterial mqmat = rec.mqid != 0 ? doc->GetMaterialFromUniqueID(rec.mqid) : nullptr;
        if (!mqmat) {
            mqmat = MQ_CreateMaterial();
            mqmat->SetName(makeUniqueMaterialName(doc, src.getName()).c_str());
            src.index = doc->AddMaterial(mqmat);
            rec.mqid = mqmat->GetUniqueID();
        }

        mqmat->SetShader(ToMQShader(src.shader_type));
        mqmat->SetVertexColor(src.use_vertex_color ? MQMATERIAL_VERTEXCOLOR_DIFFUSE : MQMATERIAL_VERTEXCOLOR_DISABLE);
        mqmat->SetDoubleSided(src.double_sided);
        mqmat->SetColor((MQColor&)src.diffuse_color);
        mqmat->SetDiffuse(src.diffuse);
        mqmat->SetAlpha(src.opacity);
        mqmat->SetAmbientColor((MQColor&)src.ambient_color);
        mqmat->SetSpecularColor((MQColor&)src.specular_color);
        mqmat->SetEmissionColor((MQColor&)src.emissive_color);

        if (src.diffuse_texture) {
            mqmat->SetTextureName(src.diffuse_texture->file_path.c_str());
            if (src.diffuse_texture->wrap_s != WrapMode::Unknown)
                mqmat->SetWrapModeU(ToMQWrapMode(src.diffuse_texture->wrap_s));
            if (src.diffuse_texture->wrap_t != WrapMode::Unknown)
                mqmat->SetWrapModeV(ToMQWrapMode(src.diffuse_texture->wrap_t));
        }
        if (src.opacity_texture)
            mqmat->SetAlphaName(src.opacity_texture->file_path.c_str());
        if (src.bump_texture)
            mqmat->SetBumpName(src.bump_texture->file_path.c_str());
    }

    // update indices
    each_material(doc, [this](MQMaterial mat, int mi) {
        UINT mqid = mat->GetUniqueID();
        for (auto& rec : m_material_records) {
            if (rec.mqid == mqid) {
                if (rec.node)
                    rec.node->index = mi;
                return;
            }
        }
    });

    return true;
}


} // namespace mqusd
