#pragma once
#include "mqusdSceneGraph.h"

namespace mqusd {


struct ExportOptions : public ConvertOptions
{
    bool export_uvs = true;
    bool export_normals = true;
    bool export_colors = false;
    bool export_material_ids = true;
    bool export_materials = true;
    bool export_blendshapes = true;
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
        MQObject mqobj = nullptr;
        MQObject mqobj_orig = nullptr;
        bool need_release = false;

        MeshNode* mesh = nullptr;
        BlendshapeNode* blendshape = nullptr;
        std::shared_ptr<MeshNode> mesh_data;

        ObjectRecord* bs_base = nullptr;
    };

    struct MaterialRecord
    {
        MQMaterial mqmaterial;
        MaterialNode material;
    };

    ObjectRecord* findRecord(MQObject obj);
    bool extractMesh(MQObject obj, MeshNode& dst);
    bool extractSkeleton(MQDocument obj, SkeletonNode& dst);
    bool extractMaterial(MQMaterial obj, MaterialNode& dst);

    void flush();
    void waitFlush();
    void writeMaterials();

    std::wstring getBonePath(UINT bone_id);

private:
    const ExportOptions* m_options;
    MQBasePlugin* m_plugin = nullptr;
#if MQPLUGIN_VERSION >= 0x0470
    MQBoneManager* m_bone_manager = nullptr;
    MQMorphManager* m_morph_manager = nullptr;
#endif

    Scene* m_scene = nullptr;
    Node* m_root = nullptr;
    SkeletonNode* m_skeleton = nullptr;
    MeshNode* m_merged_mesh = nullptr;

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
