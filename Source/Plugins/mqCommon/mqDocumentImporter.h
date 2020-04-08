#pragma once
#include "mqusd.h"

namespace mqusd {

struct ImportOptions : public ConvertOptions
{
    bool import_blendshapes = true;
    bool import_skeletons = true;
    bool import_materials = true;
    bool merge_meshes = false;
    bool bake_meshes = false;

    ImportOptions();
    bool operator==(const ImportOptions& v) const;
    bool operator!=(const ImportOptions& v) const;
    bool mayChangeAffectsNodeStructure(const ImportOptions& v) const;
};

class DocumentImporter
{
public:
    DocumentImporter(MQBasePlugin* plugin, Scene* scene, const ImportOptions* options);
    bool initialize(MQDocument doc, bool insert = false);
    bool read(MQDocument doc, double t);

private:
    struct ObjectRecord
    {
        MeshNode* node = nullptr;
        UINT mqid = 0;
        std::vector<UINT> blendshape_ids;
        MeshNode tmp_mesh;
    };

    struct InstancerRecord
    {
        InstancerNode* node = nullptr;
        UINT mqid = 0;
        MeshNode tmp_mesh;
    };

    struct JointRecord
    {
        Joint* joint = nullptr;
        UINT mqid = 0;
    };

    struct SkeletonRecord
    {
        SkeletonNode* node = nullptr;
        std::vector<JointRecord> joints;
    };

    struct MaterialRecord
    {
        MaterialNode* node = nullptr;
        UINT mqid = 0;
    };

    void clearDocument(MQDocument doc);
    std::string makeUniqueObjectName(MQDocument doc, const std::string& name);
    std::string makeUniqueMaterialName(MQDocument doc, const std::string& name);
    ObjectRecord* findRecord(UINT mqid);
    MQObject findOrCreateMQObject(MQDocument doc, UINT& id, UINT parent_id, bool& created);
    bool deleteMQObject(MQDocument doc, UINT id);
    bool updateMesh(MQDocument doc, MQObject obj, const MeshNode& src);
    bool updateSkeleton(MQDocument doc, const SkeletonNode& src);
    bool updateMaterials(MQDocument doc);

private:
    const ImportOptions* m_options;
    MQBasePlugin* m_plugin = nullptr;
#if MQPLUGIN_VERSION >= 0x0470
    MQBoneManager* m_bone_manager = nullptr;
    MQMorphManager* m_morph_manager = nullptr;
#endif

    Scene* m_scene = nullptr;
    std::vector<MeshNode*> m_mesh_nodes;

    std::vector<ObjectRecord> m_obj_records;
    std::vector<InstancerRecord> m_inst_records;
    std::vector<SkeletonRecord> m_skel_records;
    std::vector<MaterialRecord> m_material_records;

    UINT m_merged_mqobj_id = 0;
    MeshNode m_merged_mesh;

    double m_prev_time = mqusd::default_time;
    ImportOptions m_prev_options;
    bool m_option_changed = false;
    bool m_insert = false;
};
using DocumentImporterPtr = std::shared_ptr<DocumentImporter>;

} // namespace mqusd
