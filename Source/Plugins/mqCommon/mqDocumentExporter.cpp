#include "pch.h"
#include "mqDocumentExporter.h"
#include "mqUtils.h"

namespace mqusd {

ExportOptions::ExportOptions()
{
    scale_factor = 0.05f;
    flip_v = true;
}


DocumentExporter::DocumentExporter(MQBasePlugin* plugin, Scene* scene, const ExportOptions* options)
    : m_plugin(plugin)
    , m_scene(scene)
    , m_options(options)
{
}

DocumentExporter::~DocumentExporter()
{
    if (!m_scene)
        return;

    waitFlush();

    // flush archive
    m_scene->save();
}

bool DocumentExporter::initialize(MQDocument /*doc*/)
{
    m_scene->up_axis = UpAxis::Y;
    return true;
}


Node* DocumentExporter::findOrCreateNode(UINT mqid)
{
    auto rec = findRecord(mqid);
    if (!rec)
        return nullptr;

    Node* ret = nullptr;
    if (m_options->merge_meshes) {
        if (!rec->bs_base) {
            rec->mesh_data = std::make_shared<MeshNode>();
            rec->xform = rec->mesh = rec->mesh_data.get();
        }
        ret = rec->xform;
    }
    else {
        if (rec->bs_base) {
            if (m_options->export_blendshapes && !rec->blendshape) {
                auto base_mesh = rec->bs_base->mesh;
                rec->mesh_data = std::make_shared<MeshNode>();
                rec->xform = rec->mesh = rec->mesh_data.get();
                rec->blendshape = m_scene->createNode<BlendshapeNode>(base_mesh, rec->name.c_str());
                base_mesh->blendshapes.push_back(rec->blendshape);
            }
            ret = rec->blendshape;
        }
        else {
            if (!rec->xform) {
                auto parent = findOrCreateNode(rec->mqparentid);
                if (!parent)
                    parent = m_root;

                if (m_options->separate_xform) {
                    rec->xform = m_scene->createNode<XformNode>(parent, rec->name.c_str());
                    rec->mesh = m_scene->createNode<MeshNode>(rec->xform, (rec->name + "_Mesh").c_str());
                }
                else {
                    rec->xform = rec->mesh = m_scene->createNode<MeshNode>(parent, rec->name.c_str());
                }
            }
            ret = rec->xform;
        }
    }
    return ret;
}

bool DocumentExporter::write(MQDocument doc, bool one_shot)
{
    waitFlush();

    m_one_shot = one_shot;
    doc->Compact();

    // handle time & frame
    auto t = mu::Now();
    if (one_shot) {
        m_time = default_time;
    }
    else {
        auto diff = mu::NS2Sd(t - m_last_write);
        if (diff < m_options->capture_interval)
            return false;

        if (m_start_time == 0)
            m_start_time = t;
        if (m_options->keep_time)
            m_time = mu::NS2Sd(t - m_start_time) * m_options->time_scale;
        else
            m_time = (1.0 / m_options->frame_rate) * m_frame;
        ++m_frame;
    }

#if MQPLUGIN_VERSION >= 0x0470
    MQMorphManager morph_manager(m_plugin, doc);
    m_morph_manager = &morph_manager;

    MQBoneManager bone_manager(m_plugin, doc);
    m_bone_manager = &bone_manager;
#endif

    if (!m_root) {
        m_root = m_scene->root_node;
#if MQPLUGIN_VERSION >= 0x0470
        if (m_options->export_skeletons && m_bone_manager->GetBoneNum() != 0) {
            auto skel_root = m_scene->createNode<SkelRootNode>(m_root, "Model");
            skel_root->skeleton= m_scene->createNode<SkeletonNode>(skel_root, "Skel");
            m_root = skel_root;
            m_skeleton = skel_root->skeleton;
        }
#endif
        if (m_options->merge_meshes) {
            // create nodes
            auto node_name = mu::GetFilename_NoExtension(m_scene->path.c_str());
            if (node_name.empty())
                node_name = "Untitled";
            m_merged_mesh = m_scene->createNode<MeshNode>(m_root, node_name.c_str());
        }
    }

    // prepare
    int nobjects = doc->GetObjectCount();
    m_obj_records.resize(nobjects);
    for (int oi = 0; oi < nobjects; ++oi) {
        auto& rec = m_obj_records[oi];
        auto obj = doc->GetObject(oi);
        if (!obj)
            continue;

        rec.name = GetName(obj);
        rec.mqid = obj->GetUniqueID();
        if (auto parent = doc->GetParentObject(obj))
            rec.mqparentid = parent->GetUniqueID();
        rec.mqobj_orig = obj;

        if ((obj->GetMirrorType() != MQOBJECT_MIRROR_NONE && m_options->freeze_mirror) ||
            (obj->GetLatheType() != MQOBJECT_LATHE_NONE && m_options->freeze_lathe) ||
            (obj->GetPatchType() != MQOBJECT_PATCH_NONE && m_options->freeze_subdiv))
        {
            // make clones to get meshes that are applied mirror/lathe/subdiv
            rec.mqobj = obj->Clone();
            rec.need_release = true;

            DWORD freeze_flags = 0;
            if (m_options->freeze_mirror)
                freeze_flags |= MQOBJECT_FREEZE_MIRROR;
            if (m_options->freeze_lathe)
                freeze_flags |= MQOBJECT_FREEZE_LATHE;
            if (m_options->freeze_subdiv)
                freeze_flags |= MQOBJECT_FREEZE_PATCH;
            rec.mqobj->Freeze(freeze_flags);
        }
        else {
            rec.mqobj = obj;
        }
    }

#if MQPLUGIN_VERSION >= 0x0470
    // handle blendshapes
    std::vector<MQObject> bs_bases;
    std::vector<MQObject> bs_targets;
    m_morph_manager->EnumBaseObjects(bs_bases);
    for (auto base : bs_bases) {
        auto rec = findRecord(base->GetUniqueID());
        m_morph_manager->GetTargetObjects(base, bs_targets);
        for (auto target : bs_targets) {
            auto trec = findRecord(target->GetUniqueID());
            trec->bs_base = rec;
        }
    }
#endif

    // create nodes
    for(auto& rec : m_obj_records)
        findOrCreateNode(rec.mqid);


    // extract material data
    if (m_options->export_materials) {
        m_material_nodes.resize(doc->GetMaterialCount());
        fill(m_material_nodes, nullptr);

        for (auto& kvp : m_material_records)
            kvp.second.updated = false;
        each_material(doc, [this](MQMaterial mat, int mi) {
            auto& rec = m_material_records[mat->GetUniqueID()];
            rec.updated = true;
            if (!rec.node)
                rec.node = m_scene->createNode<MaterialNode>(m_root, GetName(mat).c_str());
            rec.node->index = mi;
            m_material_nodes[mi] = rec.node;
            extractMaterial(mat, *rec.node);
        });
        erase_if(m_material_records, [](auto& rec) {
            if (!rec.updated) {
                if (rec.node)
                    rec.node->removed = true;
                return true;
            }
            return false;
        });
    }

    // skeleton
    if (m_skeleton)
        extractSkeleton(doc, *m_skeleton);

    // extract mesh data
    mu::parallel_for(0, nobjects, [this](int oi) {
        auto& rec = m_obj_records[oi];
        if (rec.mesh)
            extractMesh(rec.mqobj, *rec.mesh, *rec.xform);
    });

    // make blendshape offsets
    mu::parallel_for(0, nobjects, [this](int oi) {
        auto& rec = m_obj_records[oi];
        if (rec.blendshape)
            rec.blendshape->addTarget(*rec.mesh, *rec.bs_base->mesh);
    });

    // calculate local matrices
    auto update_local_matrix = [](XformNode* xform) {
        xform->local_matrix = xform->global_matrix;
        if (xform->parent_xform)
            xform->local_matrix *= mu::invert(xform->parent_xform->global_matrix);
    };
    if (m_options->separate_xform)
        m_scene->eachNode<XformNode>(Node::Type::Xform, update_local_matrix);
    else
        m_scene->eachNode<XformNode>(update_local_matrix);

    // to local space
    mu::parallel_for(0, nobjects, [this](int oi) {
        auto& rec = m_obj_records[oi];
        if (rec.mesh && !rec.blendshape)
            rec.mesh->toLocalSpace();
    });

    // flush async
    m_task_write = std::async(std::launch::async, [this]() { flush(); });
    m_last_write = t;

#if MQPLUGIN_VERSION >= 0x0470
    m_morph_manager = nullptr;
    m_bone_manager = nullptr;
#endif

    // log
    int total_vertices = 0;
    int total_faces = 0;
    for (auto& rec : m_obj_records) {
        if (rec.mesh) {
            total_vertices += (int)rec.mesh->points.size();
            total_faces += (int)rec.mesh->counts.size();
        }
    }
    mqusdLog("frame %d: %d vertices, %d faces",
        m_frame - 1, total_vertices, total_faces);
    return true;
}

DocumentExporter::ObjectRecord* DocumentExporter::findRecord(UINT mqid)
{
    if (mqid == 0)
        return nullptr;
    auto it = std::find_if(m_obj_records.begin(), m_obj_records.end(), [mqid](auto& rec) {
        return rec.mqid == mqid;
    });
    return it == m_obj_records.end() ? nullptr : &(*it);
}

bool DocumentExporter::extractMesh(MQObject obj, MeshNode& dst, XformNode& xf)
{
    // will be called in parallel

    // transform
    // local matrix will be updated later on write()
    xf.global_matrix = mu::transform(
        to_float3(obj->GetTranslation()),
        to_quat(obj->GetRotation()),
        to_float3(obj->GetScaling()));
    if (m_options->separate_xform) {
        dst.local_matrix = float4x4::identity();
        dst.global_matrix = xf.global_matrix;
    }

    // visibility
    dst.visibility = obj->GetVisible() != 0;


    int nfaces = obj->GetFaceCount();
    int npoints = obj->GetVertexCount();
    int nindices = 0;
    for (int fi = 0; fi < nfaces; ++fi)
        nindices += obj->GetFacePointCount(fi);

    dst.points.resize_discard(npoints);
    dst.normals.resize_discard(nindices);
    dst.uvs.resize_discard(nindices);
    dst.colors.resize_discard(nindices);
    dst.material_ids.resize_discard(nfaces);
    dst.counts.resize_discard(nfaces);
    dst.indices.resize_discard(nindices);

    auto dst_points = dst.points.data();
    auto dst_normals = dst.normals.data();
    auto dst_uvs = dst.uvs.data();
    auto dst_colors = dst.colors.data();
    auto dst_mids = dst.material_ids.data();
    auto dst_counts = dst.counts.data();
    auto dst_indices = dst.indices.data();

    // points
    obj->GetVertexArray((MQPoint*)dst_points);

    int fc = 0; // 'actual' face count
    for (int fi = 0; fi < nfaces; ++fi) {
        // counts
        // GetFacePointCount() may return 0. skip it.
        int count = obj->GetFacePointCount(fi);
        if (count == 0)
            continue;
        dst_counts[fc] = count;

        // material IDs
        dst_mids[fc] = obj->GetFaceMaterial(fi);

        // indices
        obj->GetFacePointArray(fi, dst_indices);
        dst_indices += count;

        // uv
        obj->GetFaceCoordinateArray(fi, (MQCoordinate*)dst_uvs);
        dst_uvs += count;

        for (int ci = 0; ci < count; ++ci) {
            // vertex color
            *(dst_colors++) = mu::Color32ToFloat4(obj->GetFaceVertexColor(fi, ci));

#if MQPLUGIN_VERSION >= 0x0460
            // normal
            BYTE flags;
            obj->GetFaceVertexNormal(fi, ci, flags, (MQPoint&)*(dst_normals++));
#endif
        }

        ++fc;
    }

    if (nfaces != fc) {
        // refit
        dst.counts.resize(fc);
        dst.material_ids.resize(fc);
    }

#if MQPLUGIN_VERSION >= 0x0470
    if (m_skeleton) {
        // bone weights

        RawVector<int> joint_counts;
        RawVector<int> joint_indices;
        RawVector<float> joint_weights;
        int nbones = m_bone_manager->GetBoneNum();

        {
            std::vector<UINT> bone_ids;
            std::vector<int> bone_id_to_index;
            int min_bone_id = 0;
            int max_bone_id = 0;

            // build bone id -> index table
            m_bone_manager->EnumBoneID(bone_ids);
            mu::MinMax((const int*)bone_ids.data(), bone_ids.size(), min_bone_id, max_bone_id);
            bone_id_to_index.resize(max_bone_id + 1);
            for (int bi = 0; bi < nbones; ++bi)
                bone_id_to_index[bone_ids[bi]] = bi;

            // get bone indices & weights
            joint_counts.resize_zeroclear(npoints);
            joint_indices.resize_zeroclear(npoints * nbones);
            joint_weights.resize_zeroclear(npoints * nbones);
            auto* ji = joint_indices.data();
            auto* jw = joint_weights.data();
            for (int pi = 0; pi < npoints; ++pi) {
                UINT vid = obj->GetVertexUniqueID(pi);
                int c = m_bone_manager->GetVertexWeightArray(obj, vid, nbones, (UINT*)ji, jw);
                for (int bi = 0; bi < c; ++bi)
                    ji[bi] = bone_id_to_index[ji[bi]];
                joint_counts[pi] = c;
                ji += nbones;
                jw += nbones;
            }
            // 0-100 -> 0.00-1.00
            mu::Scale(joint_weights.data(), 0.01f, joint_weights.size());
        }

        {
            int min_bone_count;
            int max_bone_count;
            mu::MinMax(joint_counts.cdata(), joint_counts.size(), min_bone_count, max_bone_count);
            dst.joints_per_vertex = max_bone_count;
            dst.joint_indices.resize_zeroclear(npoints * max_bone_count);
            dst.joint_weights.resize_zeroclear(npoints * max_bone_count);
            auto* ji = dst.joint_indices.data();
            auto* jw = dst.joint_weights.data();
            for (int pi = 0; pi < npoints; ++pi) {
                joint_indices.copy_to(ji, max_bone_count, nbones * pi);
                joint_weights.copy_to(jw, max_bone_count, nbones * pi);
                ji += max_bone_count;
                jw += max_bone_count;
            }
        }
    }
#endif

    dst.materials = m_material_nodes;
    dst.buildFaceSets();

    return true;
}

std::wstring DocumentExporter::getBonePath(UINT bone_id)
{
    std::wstring ret;

    UINT parent_id;
    if (m_bone_manager->GetParent(bone_id, parent_id)) {
        ret += getBonePath(parent_id);
        ret += L'/';
    }

    std::wstring name;
    m_bone_manager->GetName(bone_id, name);
    ret += name;
    return ret;
}

bool DocumentExporter::extractSkeleton(MQDocument /*doc*/, SkeletonNode& dst)
{
#if MQPLUGIN_VERSION >= 0x0470
    // get global joint matrices
    std::vector<UINT> bone_ids;
    m_bone_manager->EnumBoneID(bone_ids);
    for (auto bid : bone_ids) {
        auto path = mu::ToMBS(getBonePath(bid));
        auto joint = dst.addJoint(path);

        MQPoint base_pos;
        MQMatrix base_matrix;
        m_bone_manager->GetBasePos(bid, base_pos);
        m_bone_manager->GetBaseMatrix(bid, base_matrix);

        float4x4 base_pose = float4x4::identity();
        //float4x4 base_pose = to_float4x4(base_matrix);
        (float3&)base_pose[3] = to_float3(base_pos);
        joint->global_matrix = base_pose;
    }

    // setup bindpose & restpose
    for (auto& joint : dst.joints) {
        if (joint->parent)
            joint->local_matrix = joint->global_matrix * mu::invert(joint->parent->global_matrix);
        else
            joint->local_matrix = joint->global_matrix;
        joint->bindpose = joint->global_matrix;
        joint->restpose = joint->local_matrix;
    }

    return true;
#else
    return false;
#endif
}

static ShaderType ToShaderType(int v)
{
    switch (v) {
    case MQMATERIAL_SHADER_CLASSIC: return ShaderType::MQClassic;
    case MQMATERIAL_SHADER_CONSTANT:return ShaderType::MQConstant;
    case MQMATERIAL_SHADER_LAMBERT: return ShaderType::MQLambert;
    case MQMATERIAL_SHADER_PHONG:   return ShaderType::MQPhong;
    case MQMATERIAL_SHADER_BLINN:   return ShaderType::MQBlinn;
    case MQMATERIAL_SHADER_HLSL:    return ShaderType::MQHLSL;
    default: return ShaderType::Unknown;
    }
}

static WrapMode ToWrapMode(int v)
{
    switch (v) {
    case MQMATERIAL_WRAP_REPEAT: return WrapMode::Repeat;
    case MQMATERIAL_WRAP_MIRROR: return WrapMode::Mirror;
    case MQMATERIAL_WRAP_CLAMP: return WrapMode::Clamp;
    default: return WrapMode::Unknown;
    }
}

bool DocumentExporter::extractMaterial(MQMaterial mtl, MaterialNode& dst)
{
    dst.shader_type = ToShaderType(mtl->GetShader());
    dst.use_vertex_color = mtl->GetVertexColor() == MQMATERIAL_VERTEXCOLOR_DIFFUSE;
    dst.double_sided = mtl->GetDoubleSided();
    dst.diffuse_color = to_float3(mtl->GetColor());
    dst.diffuse = mtl->GetDiffuse();
    dst.opacity = mtl->GetAlpha();
    dst.ambient_color = to_float3(mtl->GetAmbientColor());
    dst.specular_color = to_float3(mtl->GetSpecularColor());
    dst.emissive_color = to_float3(mtl->GetEmissionColor());

    auto wrap_s = ToWrapMode(mtl->GetWrapModeU());
    auto wrap_t = ToWrapMode(mtl->GetWrapModeV());
    char buf[1024];
    mtl->GetTextureName(buf, sizeof(buf));
    if (buf[0] != '\0') {
        dst.diffuse_texture = std::make_shared<Texture>();
        dst.diffuse_texture->file_path = buf;
        dst.diffuse_texture->wrap_s = wrap_s;
        dst.diffuse_texture->wrap_t = wrap_t;
    }

    mtl->GetAlphaName(buf, sizeof(buf));
    if (buf[0] != '\0') {
        dst.opacity_texture = std::make_shared<Texture>();
        dst.opacity_texture->file_path = buf;
        dst.opacity_texture->wrap_s = wrap_s;
        dst.opacity_texture->wrap_t = wrap_t;
    }

    mtl->GetBumpName(buf, sizeof(buf));
    if (buf[0] != '\0') {
        dst.bump_texture = std::make_shared<Texture>();
        dst.bump_texture->file_path = buf;
        dst.bump_texture->wrap_s = wrap_s;
        dst.bump_texture->wrap_t = wrap_t;
    }
    return true;
}

void DocumentExporter::flush()
{
    if (m_options->merge_meshes) {
        // make merged mesh
        auto& dst = *m_merged_mesh;
        dst.clear();
        for (auto& rec : m_obj_records)
            dst.merge(*rec.mesh);
    }

    // convert. this must be after merge.
    mu::parallel_for_each(m_scene->nodes.begin(), m_scene->nodes.end(), [this](NodePtr& n) {
        n->convert(*m_options);
    });

    // do write
    m_scene->write(m_time);
}

void DocumentExporter::waitFlush()
{
    if (!m_task_write.valid())
        return;

    m_task_write.wait();
    m_task_write = {};

    // release cloned objects
    for (auto& rec : m_obj_records) {
        if (rec.need_release) {
            rec.need_release = false;
            rec.mqobj->DeleteThis();
            rec.mqobj = nullptr;
        }
    }
}

} // namespace mqusd
