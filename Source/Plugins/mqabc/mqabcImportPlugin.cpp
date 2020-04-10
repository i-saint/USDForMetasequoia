#include "pch.h"
#include "mqusd.h"
#include "mqCommon/mqDocumentImporter.h"
#include "SceneGraph/ABC/sgabc.h"

namespace mqusd {

class mqabcImportPlugin : public MQImportPlugin
{
public:
    void GetPlugInID(DWORD* Product, DWORD* ID) override;
    const char* GetPlugInName(void) override;
    const char* EnumFileType(int index) override;
    const char* EnumFileExt(int index) override;
    BOOL ImportFile(int index, const wchar_t* filename, MQDocument doc) override;
};


void mqabcImportPlugin::GetPlugInID(DWORD* Product, DWORD* ID)
{
    *Product = mqabcPluginProduct;
    *ID = mqabcImportPluginID;
}

const char* mqabcImportPlugin::GetPlugInName(void)
{
    return "Alembic Importer (version " mqusdVersionString ") " mqusdCopyRight;
}

const char* mqabcImportPlugin::EnumFileType(int index)
{
    switch (index) {
    case 0: return "Alembic (*.abc)";
    default: return nullptr;
    }
}

const char* mqabcImportPlugin::EnumFileExt(int index)
{
    switch (index) {
    case 0: return "abc";
    default: return nullptr;
    }
}

BOOL mqabcImportPlugin::ImportFile(int /*index*/, const wchar_t* filename, MQDocument doc)
{
    ScenePtr scene = CreateABCIScene();
    if (!scene)
        return false;

    if (!scene->open(mu::ToMBS(filename).c_str()))
        return false;

    ImportOptions options;
    auto importer = std::make_shared<DocumentImporter>(this, scene.get(), &options);
    importer->initialize(doc, true);
    importer->read(doc, scene->time_start);

    return true;
}

} // namespace mqusd

mqabcAPI MQBasePlugin* mqabcGetImportPlugin()
{
    static mqusd::mqabcImportPlugin s_inst;
    return &s_inst;
}
