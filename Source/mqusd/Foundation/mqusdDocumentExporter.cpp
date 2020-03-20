#include "pch.h"
#include "mqusdDocumentExporter.h"

namespace mqusd {

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
    if (m_options->export_materials)
        writeMaterials();

    // flush archive
    m_scene->save();
}


static inline std::string GetName(MQObject obj)
{
    char buf[256];
    obj->GetName(buf, sizeof(buf));
    return buf;
}

bool DocumentExporter::write(MQDocument doc, bool one_shot)
{
    waitFlush();

    // handle time & frame
    auto t = mu::Now();
    if (one_shot) {
        m_time = std::numeric_limits<double>::quiet_NaN();
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
            auto skel_root = (SkelRootNode*)m_scene->createNode(m_root, "Model", Node::Type::SkelRoot);
            skel_root->skeleton= (SkeletonNode*)m_scene->createNode(skel_root, "Skel", Node::Type::Skeleton);
            m_root = skel_root;
            m_skeleton = skel_root->skeleton;
        }
#endif
        if (m_options->merge_meshes) {
            // create nodes
            auto node_name = mu::GetFilename_NoExtension(m_scene->path.c_str());
            if (node_name.empty())
                node_name = "Untitled";
            m_merged_mesh = (MeshNode*)m_scene->createNode(m_root, node_name.c_str(), Node::Type::Mesh);
        }
    }

    // prepare
    int nobjects = doc->GetObjectCount();
    m_obj_records.resize(nobjects);
    for (int oi = 0; oi < nobjects; ++oi) {
        auto& rec = m_obj_records[oi];
        auto obj = doc->GetObject(oi);

        if (!rec.mesh) {
            if (m_options->merge_meshes) {
                rec.mesh_data = std::make_shared<MeshNode>();
                rec.mesh = rec.mesh_data.get();
            }
            else {
                rec.mesh = (MeshNode*)m_scene->createNode(m_root, GetName(obj).c_str(), Node::Type::Mesh);
            }

            if (!rec.mesh) {
                mqusdLog("failed to create mesh node.");
                return false;
            }
        }

        if ((obj->GetMirrorType() != MQOBJECT_MIRROR_NONE && m_options->freeze_mirror) ||
            (obj->GetLatheType() != MQOBJECT_LATHE_NONE && m_options->freeze_lathe) ||
            (obj->GetPatchType() != MQOBJECT_PATCH_NONE && m_options->freeze_subdiv))
        {
            // make clones to get meshes that are applied mirror/lathe/subdiv
            rec.mqobject = obj->Clone();

            DWORD freeze_flags = 0;
            if (m_options->freeze_mirror)
                freeze_flags |= MQOBJECT_FREEZE_MIRROR;
            if (m_options->freeze_lathe)
                freeze_flags |= MQOBJECT_FREEZE_LATHE;
            if (m_options->freeze_subdiv)
                freeze_flags |= MQOBJECT_FREEZE_PATCH;
            rec.mqobject->Freeze(freeze_flags);

            rec.need_release = true;
        }
        else {
            rec.mqobject = obj;
        }
    }

    // extract material data
    int nmaterials = doc->GetMaterialCount();
    m_material_records.resize(nmaterials);
    for (int mi = 0; mi < nmaterials; ++mi) {
        auto& rec = m_material_records[mi];
        rec.mqmaterial = doc->GetMaterial(mi);
        extractMaterial(rec.mqmaterial, rec.material);
    }

    // skeleton
#if MQPLUGIN_VERSION >= 0x0470
    if (m_skeleton) {
        extractSkeleton(doc, *m_skeleton);
    }
#endif

    // extract mesh data
    mu::parallel_for(0, nobjects, [this](int oi) {
        auto& rec = m_obj_records[oi];
        extractMesh(rec.mqobject, *rec.mesh);
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
        total_vertices += (int)rec.mesh->points.size();
        total_faces += (int)rec.mesh->counts.size();
    }
    mqusdLog("frame %d: %d vertices, %d faces",
        m_frame - 1, total_vertices, total_faces);
    return true;
}

bool DocumentExporter::extractMesh(MQObject obj, MeshNode& dst)
{
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

    dst.name = obj->GetName();

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
    mu::InvertV(dst.uvs.data(), dst.uvs.size());

    if (nfaces != fc) {
        // refit
        dst.counts.resize(fc);
        dst.material_ids.resize(fc);
    }

#if MQPLUGIN_VERSION >= 0x0470
    if (m_options->export_skeletons) {
        // bone weights

        RawVector<int> joint_counts;
        RawVector<int> joint_indices;
        RawVector<float> joint_weights;
        int nbones = m_bone_manager->GetBoneNum();

        {
            std::vector<UINT> bone_ids;
            std::vector<int> bone_id_to_index;
            int min_bone_id;
            int max_bone_id;

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

    dst.convert(*m_options);

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

bool DocumentExporter::extractSkeleton(MQDocument doc, SkeletonNode& dst)
{
#if MQPLUGIN_VERSION >= 0x0470
    std::vector<UINT> bone_ids;
    m_bone_manager->EnumBoneID(bone_ids);
    for (auto bid : bone_ids) {
        auto path = mu::ToMBS(getBonePath(bid));
        auto joint = dst.addJoint(path);

        MQPoint base_pos;
        MQMatrix base_matrix;
        m_bone_manager->GetBasePos(bid, base_pos);
        m_bone_manager->GetBaseMatrix(bid, base_matrix);
        // todo
    }

    return true;
#else
    return false;
#endif
}

bool DocumentExporter::extractMaterial(MQMaterial mtl, MaterialNode& dst)
{
    char buf[256];
    mtl->GetName(buf, sizeof(buf));

    dst.name = buf;
    dst.shader = mtl->GetShader();
    dst.use_vertex_color = mtl->GetVertexColor() == MQMATERIAL_VERTEXCOLOR_DIFFUSE;
    dst.double_sided = mtl->GetDoubleSided();
    dst.color = to_float3(mtl->GetColor());
    dst.diffuse = mtl->GetDiffuse();
    dst.alpha = mtl->GetAlpha();
    dst.ambient_color = to_float3(mtl->GetAmbientColor());
    dst.specular_color = to_float3(mtl->GetSpecularColor());
    dst.emission_color = to_float3(mtl->GetEmissionColor());
    return true;
}

void DocumentExporter::flush()
{
    if (m_options->merge_meshes) {
        // make merged mesh
        auto& dst = *m_merged_mesh;
        dst.clear();
        for (auto& rec : m_obj_records)
            dst.merge(rec.mesh);
    }

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
            rec.mqobject->DeleteThis();
            rec.mqobject = nullptr;
        }
    }
}

void DocumentExporter::writeMaterials()
{
    // todo
}

} // namespace mqusd
