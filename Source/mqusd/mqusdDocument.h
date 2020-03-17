#pragma once
#include "mqusdSceneGraph.h"

namespace mqusd {

struct ImportOptions : public ConvertOptions
{
    bool import_skeletons = true;
    bool import_materials = true;
    bool merge_meshes = true;

    ImportOptions()
    {
        scale_factor = 20.0f;
    }
};

class DocumentImporter
{
public:
    DocumentImporter(MQBasePlugin* plugin, Scene* scene, const ImportOptions* options);
    bool initialize(MQDocument doc);
    bool read(MQDocument doc, double t);

private:
    bool updateMesh(MQObject obj, const MeshNode& src);
    bool updateSkeleton(MQDocument obj, const SkeletonNode& src);
    bool updateMaterials(MQDocument doc);

private:
    const ImportOptions* m_options;
    MQBasePlugin* m_plugin = nullptr;

    Scene* m_scene = nullptr;

    int m_mqobj_id = 0;
    MeshNode m_mesh_merged;
};
using DocumentImporterPtr = std::shared_ptr<DocumentImporter>;



struct ExportOptions : public ConvertOptions
{
    bool export_uvs = true;
    bool export_normals = true;
    bool export_colors = false;
    bool export_material_ids = true;
    bool export_materials = true;
    bool export_skeletons = true;
    bool freeze_mirror = true;
    bool freeze_lathe = true;
    bool freeze_subdiv = false;
    bool merge_meshes = false;
    bool keep_time = false;
    float frame_rate = 30.0f; // relevant only when keep_time is false
    float time_scale = 1.0f; // relevant only when keep_time is true
    double capture_interval = 5.0; // in seconds

    ExportOptions()
    {
        scale_factor = 0.05f;
    }
};

class DocumentExporter
{
public:
    DocumentExporter(MQBasePlugin* plugin, Scene* scene, const ExportOptions* options);
    ~DocumentExporter();
    bool write(MQDocument doc, bool one_shot);

private:
    struct ObjectRecord
    {
        MQObject mqobject;
        bool need_release = false;
        MeshNode mesh;
    };

    struct MaterialRecord
    {
        MQMaterial mqmaterial;
        MaterialNode material;
    };

    bool extractMesh(MQObject obj, MeshNode& dst);
    bool extractSkeleton(MQDocument obj, SkeletonNode& dst);
    bool extractMaterial(MQMaterial obj, MaterialNode& dst);

    void flush();
    void waitFlush();
    void writeMaterials();

private:
    const ExportOptions* m_options;
    MQBasePlugin* m_plugin = nullptr;

    Scene* m_scene = nullptr;
    RootNode* m_root_node = nullptr;
    MeshNode* m_mesh_node = nullptr;

    mu::nanosec m_start_time = 0;
    mu::nanosec m_last_write = 0;
    int m_frame = 0;
    double m_time = 0.0;
    std::vector<ObjectRecord> m_obj_records;
    std::vector<MaterialRecord> m_material_records;
    std::future<void> m_task_write;
};
using DocumentExporterPtr = std::shared_ptr<DocumentExporter>;

} // namespace mqusd
