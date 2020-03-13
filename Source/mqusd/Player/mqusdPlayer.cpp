#include "pch.h"
#include "mqusd.h"
#include "mqusdPlayerPlugin.h"
#include "mqusdNodeInternal.h"




bool mqusdPlayerPlugin::OpenUSD(const std::string& path)
{
    m_stage = UsdStage::Open(path);
    if (!m_stage) {
        LogInfo(
            "failed to open %s\n"
            "it may not an usd file"
            , path.c_str());
        return false;
    }

//    auto masters = m_stage->GetMasters();
//    for (auto& m : masters) {
//        m_masters.push_back(createSchemaRecursive(nullptr, m));
//}
    auto root = m_stage->GetPseudoRoot();
    if (root.IsValid()) {
        m_root_node = new RootNode_(root);
        ConstructTree(m_root_node);
    }

    return true;
}

bool mqusdPlayerPlugin::CloseUSD()
{
    m_root_node = nullptr;
    m_mesh_nodes.clear();
    m_material_nodes.clear();
    m_nodes.clear();

    m_stage = {};
    m_usd_path.clear();

    m_seek_time = 0;
    m_mqobj_id = 0;

    return true;
}

void mqusdPlayerPlugin::ConstructTree(Node* n)
{
    m_nodes.push_back(NodePtr(n));

    auto& prim = *n->getPrim();
    auto children = prim.GetChildren();
    for (auto cprim : children) {
        Node* c = nullptr;

        if (!c) {
            UsdGeomMesh mesh(cprim);
            if (mesh) {
                auto mn = new MeshNode_(n, cprim);
                m_mesh_nodes.push_back(mn);
                c = mn;
            }
        }
        // xform must be last because Mesh is also Xformable
        if (!c) {
            UsdGeomXformable xform(cprim);
            if (xform) {
                c = new XformNode_(n, cprim);
            }
        }
        if (!c) {
            c = new Node_(n, cprim);
        }

        ConstructTree(c);
    }
}

void mqusdPlayerPlugin::ImportMaterials(MQDocument doc)
{
    int nmaterials = (int)m_material_nodes.size();
    for (int mi = 0; mi < nmaterials; ++mi) {
        auto& src= m_material_nodes[mi]->material;
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
    if (!m_stage)
        return;

    m_seek_time = t;

    // read abc
    m_root_node->update(t);
    mu::parallel_for_each(m_mesh_nodes.begin(), m_mesh_nodes.end(), [this](MeshNode* n) {
        n->convert(m_settings);
    });

    // build merged mesh
    auto& mesh = m_mesh_merged;
    mesh.clear();
    for (auto n : m_mesh_nodes)
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

        auto name = mu::GetFilename_NoExtension(m_usd_path.c_str());
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
    return m_stage;
}

double mqusdPlayerPlugin::GetTimeStart() const
{
    return m_stage->GetStartTimeCode();
}
double mqusdPlayerPlugin::GetTimeEnd() const
{
    return m_stage->GetEndTimeCode();
}
