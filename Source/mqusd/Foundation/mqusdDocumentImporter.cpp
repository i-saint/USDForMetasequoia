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
        rec.mesh = m_scene->mesh_nodes[oi];
        rec.blendshape_ids.resize(rec.mesh->blendshapes.size());
    }

    if (m_options->import_materials)
        updateMaterials(doc);

    read(doc, 0.0);
    return true;
}

static MQObject FindOrCreateMQObject(MQDocument doc, UINT& id, MQObject parent, bool& created)
{
    auto mqo = doc->GetObjectFromUniqueID(id);
    if (!mqo) {
        mqo = MQ_CreateObject();
        if (parent) {
            doc->InsertObject(mqo, parent);
            mqo->SetDepth(parent->GetDepth() + 1);
        }
        else {
            doc->AddObject(mqo);
        }
        id = mqo->GetUniqueID();
        created = true;
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

    // skeleton
    if (m_options->import_skeletons) {
        for (auto n : m_scene->skeleton_nodes)
            updateSkeleton(doc, *n);
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
        auto obj = FindOrCreateMQObject(doc, m_mqobj_id, nullptr, created);
        if (created) {
            auto name = mu::GetFilename_NoExtension(m_scene->path.c_str());
            obj->SetName(name.c_str());
        }
        obj->Clear();

        updateMesh(obj, mesh);
    }
    else {
#if MQPLUGIN_VERSION >= 0x0470
        MQMorphManager morph_manager(m_plugin, doc);
#endif
        for (auto& rec : m_obj_records) {
            // mesh
            bool created;
            auto obj = FindOrCreateMQObject(doc, rec.mqobj_id, nullptr, created);
            if (created) {
                auto name = mu::ToWCS(rec.mesh->name);
                obj->SetName(name.c_str());
            }
            updateMesh(obj, *rec.mesh);

            // blendshapes
            if (m_options->import_blendshapes) {
                size_t nbs = rec.mesh->blendshapes.size();
                for (size_t bi = 0; bi < nbs; ++bi) {
                    auto blendshape = rec.mesh->blendshapes[bi];
                    blendshape->makeMesh(rec.tmp_mesh, *rec.mesh);

                    auto bs = FindOrCreateMQObject(doc, rec.blendshape_ids[bi], obj, created);
                    updateMesh(bs, rec.tmp_mesh);
                    if (created) {
                        auto name = mu::ToWCS(blendshape->name);
                        bs->SetName(name.c_str());
                        bs->SetVisible(0);
#if MQPLUGIN_VERSION >= 0x0470
                        morph_manager.BindTargetObject(obj, bs);
#endif
                    }
                }
            }
        }
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
