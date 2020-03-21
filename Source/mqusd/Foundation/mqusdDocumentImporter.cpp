#include "pch.h"
#include "mqusdDocumentImporter.h"

namespace mqusd {

DocumentImporter::DocumentImporter(MQBasePlugin* plugin, Scene* scene, const ImportOptions* options)
    : m_plugin(plugin)
    , m_scene(scene)
    , m_options(options)
{
}

bool DocumentImporter::initialize(MQDocument doc)
{
    size_t nobjs = m_scene->mesh_nodes.size();
    m_obj_records.resize(nobjs);
    for (size_t oi = 0; oi < nobjs; ++oi) {
        auto& rec = m_obj_records[oi];
        rec.node = m_scene->mesh_nodes[oi];
        rec.node->userdata = &rec;
        rec.blendshape_ids.resize(rec.node->blendshapes.size());
    }

    size_t nskels = m_scene->skeleton_nodes.size();
    m_skel_records.resize(nskels);
    for (size_t si = 0; si < nskels; ++si) {
        auto& rec = m_skel_records[si];
        rec.node = m_scene->skeleton_nodes[si];
        rec.node->userdata = &rec;

        size_t njoints = rec.node->joints.size();
        rec.joints.resize(njoints);
        for (size_t ji = 0; ji < njoints; ++ji) {
            auto& jrec = rec.joints[ji];
            jrec.joint = rec.node->joints[ji].get();
            jrec.joint->userdata = &jrec;
        }
    }

    if (m_options->import_materials)
        updateMaterials(doc);

    read(doc, 0.0);
    return true;
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

bool DocumentImporter::read(MQDocument doc, double t)
{
    if (!m_scene)
        return false;

    // read scene
    m_scene->read(t);
    mu::parallel_for_each(m_scene->mesh_nodes.begin(), m_scene->mesh_nodes.end(), [this](MeshNode* n) {
        n->toWorldSpace();
    });
    mu::parallel_for_each(m_scene->nodes.begin(), m_scene->nodes.end(), [this](NodePtr& n) {
        n->convert(*m_options);
    });

    // reserve materials
    {
        int nmaterials = 0;
        for (auto n : m_scene->mesh_nodes)
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
    if (m_options->import_skeletons) {
        for (auto& rec : m_skel_records)
            updateSkeleton(doc, *rec.node);
    }

    // update mq object
    if (m_options->merge_meshes) {
        // build merged mesh
        auto& mesh = m_mesh_merged;
        mesh.clear();
        for (auto n : m_scene->mesh_nodes)
            mesh.merge(*n);
        mesh.validate();

        bool created;
        auto obj = findOrCreateMQObject(doc, m_mqobj_id, 0, created);
        if (created) {
            auto name = mu::GetFilename_NoExtension(m_scene->path.c_str());
            obj->SetName(name.c_str());
        }
        obj->Clear();

        updateMesh(doc, obj, mesh);
    }
    else {
        for (auto& rec : m_obj_records) {
            UINT parent_id = 0;
            if (auto pmesh = rec.node->findParentMesh())
                parent_id = ((ObjectRecord*)pmesh->userdata)->mqid;

            bool created;
            auto obj = findOrCreateMQObject(doc, rec.mqid, parent_id, created);
            if (created) {
                auto name = mu::ToWCS(rec.node->name);
                obj->SetName(name.c_str());
            }
            updateMesh(doc, obj, *rec.node);

            // blendshapes
            if (m_options->import_blendshapes) {
                size_t nbs = rec.node->blendshapes.size();
                for (size_t bi = 0; bi < nbs; ++bi) {
                    auto blendshape = rec.node->blendshapes[bi];
                    blendshape->makeMesh(rec.tmp_mesh, *rec.node);

                    auto bs = findOrCreateMQObject(doc, rec.blendshape_ids[bi], rec.mqid, created);
                    updateMesh(doc, bs, rec.tmp_mesh);
                    if (created) {
                        auto name = mu::ToWCS(blendshape->name);
                        bs->SetName(name.c_str());
                        bs->SetVisible(0);
#if MQPLUGIN_VERSION >= 0x0470
                        m_morph_manager->BindTargetObject(obj, bs);
#endif
                    }
                }
            }
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

bool DocumentImporter::updateMesh(MQDocument doc, MQObject obj, const MeshNode& src)
{
    obj->Clear();

    int nfaces = (int)src.counts.size();
    int npoints = (int)src.points.size();

    // points
    {
        for (auto& p : src.points)
            obj->AddVertex((MQPoint&)p);
    }

    // faces
    {
        int ii = 0;
        auto* data = (int*)src.indices.cdata();
        for (auto c : src.counts) {
            obj->AddFace(c, data);
            data += c;
        }
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
    if (m_options->import_skeletons && src.skeleton) {
        m_bone_manager->AddSkinObject(obj);

        std::vector<UINT> joint_ids;
        if (!src.joints.empty()) {
            for (auto& path : src.joints) {
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

bool DocumentImporter::updateSkeleton(MQDocument doc, const SkeletonNode& src)
{
#if MQPLUGIN_VERSION >= 0x0470
    MQBoneManager bone_manager(m_plugin, doc);
    bone_manager.BeginImport();

    auto& rec = *(SkeletonRecord*)src.userdata;
    size_t njoints = rec.joints.size();
    for (size_t ji = 0; ji < njoints; ++ji) {
        auto& joint = *src.joints[ji];
        auto& jrec = rec.joints[ji];

        float3 t; quatf r; float3 s;
        std::tie(t, r, s) = mu::extract_trs(joint.bindpose);

        if (jrec.mqid == 0) {
            MQBoneManager::ADD_BONE_PARAM param;
            param.name = mu::ToWCS(joint.name);
            param.pos = (MQPoint&)t;
            if (joint.parent)
                param.parent_id = ((JointRecord*)joint.parent->userdata)->mqid;
            jrec.mqid = bone_manager.AddBone(param);
        }
        else {
            bone_manager.SetBasePos(jrec.mqid, (MQPoint&)t);
        }
    }

    bone_manager.EndImport();
    return true;
#else
    return false;
#endif
}

bool DocumentImporter::updateMaterials(MQDocument doc)
{
    auto& material_nodes = m_scene->material_nodes;
    int nmaterials = (int)material_nodes.size();
    for (int mi = 0; mi < nmaterials; ++mi) {
        auto& src = *material_nodes[mi];
        MQMaterial mqmat = nullptr;
        if (mi < doc->GetMaterialCount()) {
            mqmat = doc->GetMaterial(mi);
            mqmat->SetName(src.name.c_str());
        }
        else {
            mqmat = MQ_CreateMaterial();
            mqmat->SetName(src.name.c_str());
            doc->AddMaterial(mqmat);
        }
        mqmat->SetShader(src.shader);
        mqmat->SetVertexColor(src.use_vertex_color ? MQMATERIAL_VERTEXCOLOR_DIFFUSE : MQMATERIAL_VERTEXCOLOR_DISABLE);
        mqmat->SetDoubleSided(src.double_sided);
        mqmat->SetColor((MQColor&)src.color);
        mqmat->SetDiffuse(src.diffuse);
        mqmat->SetAlpha(src.alpha);
        mqmat->SetAmbientColor((MQColor&)src.ambient_color);
        mqmat->SetSpecularColor((MQColor&)src.specular_color);
        mqmat->SetEmissionColor((MQColor&)src.emission_color);
    }
    return true;
}


} // namespace mqusd
