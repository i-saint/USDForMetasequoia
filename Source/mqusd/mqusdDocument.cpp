#include "pch.h"
#include "mqusd.h"
#include "mqusdDocument.h"

namespace mqusd {


DocumentImporter::DocumentImporter(MQBasePlugin* plugin, Scene* scene, const ImportOptions* options)
    : m_plugin(plugin)
    , m_scene(scene)
    , m_options(options)
{
}

bool DocumentImporter::read(MQDocument doc, double t)
{
    if (!m_scene)
        return false;

    // read scene
    m_scene->read(t);
    mu::parallel_for_each(m_scene->mesh_nodes.begin(), m_scene->mesh_nodes.end(), [this](MeshNode* n) {
        n->toWorldSpace();
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

    // update mq object
    if (m_options->merge_meshes) {
        // build merged mesh
        auto& mesh = m_mesh_merged;
        mesh.clear();
        for (auto n : m_scene->mesh_nodes)
            mesh.merge(*n);
        mesh.validate();

        auto obj = doc->GetObjectFromUniqueID(m_mqobj_id);
        if (!obj) {
            obj = MQ_CreateObject();
            doc->AddObject(obj);
            m_mqobj_id = obj->GetUniqueID();

            auto name = mu::GetFilename_NoExtension(m_scene->path.c_str());
            obj->SetName(name.c_str());
        }
        obj->Clear();

        updateMesh(obj, mesh);
    }
    else {
        // todo
    }

    return true;
}

bool DocumentImporter::updateMesh(MQObject obj, const MeshNode& src)
{
    obj->Clear();

    int nfaces = (int)src.counts.size();

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
    return true;
}

bool DocumentImporter::updateSkeleton(MQDocument doc, const SkeletonNode& src)
{
#if MQPLUGIN_VERSION >= 0x0470
    MQBoneManager bone_manager(m_plugin, doc);
    bone_manager.BeginImport();

    std::vector<UINT> bone_ids;
    std::vector<UINT> vertex_ids;
    std::vector<float> weights;
    int nobjects = doc->GetObjectCount();

    bone_manager.EnumBoneID(bone_ids);
    for (auto bid : bone_ids) {
        MQPoint base_pos;
        bone_manager.GetBasePos(bid, base_pos);
        // todo

        for (int oi = 0; oi < nobjects; ++oi) {
            auto obj = doc->GetObject(oi);
            int nweights = bone_manager.GetWeightedVertexArray(bid, obj, vertex_ids, weights);
            if (nweights == 0)
                continue;
            // todo
        }
    }

    bone_manager.EndImport();
    return true;
#else
    return false;
#endif
}



DocumentExporter::DocumentExporter(MQBasePlugin* plugin, Scene* scene, const ExportOptions* options)
    : m_plugin(plugin)
    , m_scene(scene)
    , m_options(options)
{
    // create nodes
    auto node_name = mu::GetFilename_NoExtension(scene->path.c_str());
    if (node_name.empty())
        node_name = "Untitled";

    auto root_node = m_scene->root_node;
    auto mesh_node = m_scene->createNode(root_node, node_name.c_str(), Node::Type::Mesh);
    m_mesh_node = static_cast<MeshNode*>(mesh_node);
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

    // prepare
    int nobjects = doc->GetObjectCount();
    m_obj_records.resize(nobjects);
    for (int oi = 0; oi < nobjects; ++oi) {
        auto& rec = m_obj_records[oi];
        auto obj = doc->GetObject(oi);
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

    // extract mesh data
    mu::parallel_for(0, nobjects, [this](int oi) {
        auto& rec = m_obj_records[oi];
        extractMesh(rec.mqobject, rec.mesh);
    });


    // flush async
    m_task_write = std::async(std::launch::async, [this]() { flush(); });
    m_last_write = t;

    // log
    int total_vertices = 0;
    int total_faces = 0;
    for (auto& rec : m_obj_records) {
        total_vertices += (int)rec.mesh.points.size();
        total_faces += (int)rec.mesh.counts.size();
    }
    mqusdLog("frame %d: %d vertices, %d faces",
        m_frame - 1, total_vertices, total_faces);
    return true;
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

    dst.convert(*m_options);

    return true;
}

bool DocumentExporter::extractSkeleton(MQDocument doc, SkeletonNode& mesh)
{
#if MQPLUGIN_VERSION >= 0x0470
    MQBoneManager bone_manager(m_plugin, doc);

    std::vector<UINT> bone_ids;
    std::vector<UINT> vertex_ids;
    std::vector<float> weights;
    int nobjects = doc->GetObjectCount();

    bone_manager.EnumBoneID(bone_ids);
    for (auto bid : bone_ids) {
        MQPoint base_pos;
        bone_manager.GetBasePos(bid, base_pos);
        // todo

        for (int oi = 0; oi < nobjects; ++oi) {
            auto obj = doc->GetObject(oi);
            int nweights = bone_manager.GetWeightedVertexArray(bid, obj, vertex_ids, weights);
            if (nweights == 0)
                continue;
            // todo
        }
    }

    return true;
#else
    return false;
#endif
}

void DocumentExporter::flush()
{
    // make merged mesh
    auto& dst = *m_mesh_node;
    dst.clear();
    for (auto& rec : m_obj_records)
        dst.merge(rec.mesh);

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
