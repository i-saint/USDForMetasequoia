#include "pch.h"
#include "mqusd.h"
#include "mqusdPlayerPlugin.h"




bool mqusdPlayerPlugin::OpenUSD(const std::string& path)
{
    m_scene = CreateScene();
    if (!m_scene)
        return false;

    if (!m_scene->open(path.c_str())) {
        LogInfo(
            "failed to open %s\n"
            "it may not an usd file"
            , path.c_str());
        m_scene = {};
        return false;
    }
    return true;
}

bool mqusdPlayerPlugin::CloseUSD()
{
    m_scene = {};

    m_seek_time = 0;
    m_mqobj_id = 0;

    return true;
}

void mqusdPlayerPlugin::ImportMaterials(MQDocument doc)
{
    if (!m_scene)
        return;

    auto& material_nodes = m_scene->material_nodes;
    int nmaterials = (int)material_nodes.size();
    for (int mi = 0; mi < nmaterials; ++mi) {
        auto& src= material_nodes[mi]->material;
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
}

void mqusdPlayerPlugin::Seek(MQDocument doc, double t)
{
    if (!m_scene)
        return;

    m_seek_time = t;
    m_scene->read(t);
    mu::parallel_for_each(m_scene->mesh_nodes.begin(), m_scene->mesh_nodes.end(), [this](MeshNode* n) {
        n->convert(m_settings);
    });

    // build merged mesh
    auto& mesh = m_mesh_merged;
    mesh.clear();
    for (auto n : m_scene->mesh_nodes)
        mesh.merge(n->mesh);
    mesh.clearInvalidComponent();

    // reserve materials
    {
        int mi = 0;
        int nmaterials = mesh.getMaxMaterialID();
        while (doc->GetMaterialCount() <= nmaterials) {
            const size_t buf_len = 128;
            wchar_t buf[buf_len];
            swprintf(buf, buf_len, L"usdmat%d", mi++);

            auto mat = MQ_CreateMaterial();
            mat->SetName(buf);

            doc->AddMaterial(mat);
        }
    }


    // update mq object
    auto obj = doc->GetObjectFromUniqueID(m_mqobj_id);
    if (!obj) {
        obj = MQ_CreateObject();
        doc->AddObject(obj);
        m_mqobj_id = obj->GetUniqueID();

        auto name = mu::GetFilename_NoExtension(m_scene->path.c_str());
        obj->SetName(name.c_str());
    }
    obj->Clear();


    int nfaces = (int)mesh.counts.size();

    // points
    {
        for (auto& p : mesh.points)
            obj->AddVertex((MQPoint&)p);
    }

    // faces
    {
        int ii = 0;
        auto* data = (int*)mesh.indices.cdata();
        for (auto c : mesh.counts) {
            obj->AddFace(c, data);
            data += c;
        }
    }

    // uvs
    if (!mesh.uvs.empty()) {
        auto* data = (MQCoordinate*)mesh.uvs.cdata();
        for (int fi = 0; fi < nfaces; ++fi) {
            int c = mesh.counts[fi];
            obj->SetFaceCoordinateArray(fi, data);
            data += c;
        }
    }

    // normals
#if MQPLUGIN_VERSION >= 0x0460
    if (!mesh.normals.empty()) {
        auto* data = (const MQPoint*)mesh.normals.cdata();
        for (int fi = 0; fi < nfaces; ++fi) {
            int c = mesh.counts[fi];
            for (int ci = 0; ci < c; ++ci)
                obj->SetFaceVertexNormal(fi, ci, MQOBJECT_NORMAL_NONE, data[ci]);
            data += c;
        }
    }
#endif

    // colors
    if (!mesh.colors.empty()) {
        auto* data = mesh.colors.cdata();
        for (int fi = 0; fi < nfaces; ++fi) {
            int c = mesh.counts[fi];
            for (int ci = 0; ci < c; ++ci)
                obj->SetFaceVertexColor(fi, ci, mu::Float4ToColor32(data[ci]));
            data += c;
        }
    }

    // material ids
    if (!mesh.material_ids.empty()) {
        auto* data = mesh.material_ids.cdata();
        for (int fi = 0; fi < nfaces; ++fi)
            obj->SetFaceMaterial(fi, data[fi]);
    }

    // repaint
    MQ_RefreshView(nullptr);
}

void mqusdPlayerPlugin::Refresh(MQDocument doc)
{
    Seek(doc, m_seek_time);
}

mqusdPlayerSettings& mqusdPlayerPlugin::GetSettings()
{
    return m_settings;
}

bool mqusdPlayerPlugin::IsArchiveOpened() const
{
    return m_scene != nullptr;
}

double mqusdPlayerPlugin::GetTimeStart() const
{
    return m_scene->time_start;
}
double mqusdPlayerPlugin::GetTimeEnd() const
{
    return m_scene->time_end;
}
