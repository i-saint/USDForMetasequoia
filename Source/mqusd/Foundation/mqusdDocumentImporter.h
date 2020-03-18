#pragma once
#include "mqusdSceneGraph.h"

namespace mqusd {

struct ImportOptions : public ConvertOptions
{
    bool import_blendshapes = true;
    bool import_skeletons = true;
    bool import_materials = true;
    bool merge_meshes = false;

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
    struct ObjectRecord
    {
        MeshNode* mesh = nullptr;
        UINT mqobj_id = 0;
        std::vector<UINT> blendshape_ids;
        MeshNode tmp_mesh;
    };

    bool updateMesh(MQObject obj, const MeshNode& src);
    bool updateSkeleton(MQDocument obj, const SkeletonNode& src);
    bool updateMaterials(MQDocument doc);

private:
    const ImportOptions* m_options;
    MQBasePlugin* m_plugin = nullptr;

    Scene* m_scene = nullptr;
    std::vector<ObjectRecord> m_obj_records;

    UINT m_mqobj_id = 0;
    MeshNode m_mesh_merged;
};
using DocumentImporterPtr = std::shared_ptr<DocumentImporter>;

} // namespace mqusd