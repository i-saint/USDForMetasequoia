#include "pch.h"
#include "mqusd.h"
#include "mqCommon/mqDocumentExporter.h"
#include "SceneGraph/ABC/sgabc.h"

namespace mqusd {

class mqabcExportPlugin : public MQExportPlugin
{
public:
    void GetPlugInID(DWORD* Product, DWORD* ID) override;
    const char* GetPlugInName(void) override;
    const char* EnumFileType(int index) override;
    const char* EnumFileExt(int index) override;
    BOOL ExportFile(int index, const wchar_t* filename, MQDocument doc) override;
};


void mqabcExportPlugin::GetPlugInID(DWORD* Product, DWORD* ID)
{
    *Product = mqabcPluginProduct;
    *ID = mqabcExportPluginID;
}

const char* mqabcExportPlugin::GetPlugInName(void)
{
    return "Alembic Exporter (version " mqusdVersionString ") " mqusdCopyRight;
}

const char* mqabcExportPlugin::EnumFileType(int index)
{
    // note: USDZ is not supported for export
    switch (index) {
    case 0: return "Alembic (*.abc)";
    default: return nullptr;
    }
}

const char* mqabcExportPlugin::EnumFileExt(int index)
{
    switch (index) {
    case 0: return "abc";
    default: return nullptr;
    }
}

BOOL mqabcExportPlugin::ExportFile(int /*index*/, const wchar_t* filename, MQDocument doc)
{
    ScenePtr scene = CreateABCOScene();
    if (!scene)
        return false;

    if (!scene->create(mu::ToMBS(filename).c_str()))
        return false;

    ExportOptions options;
    auto exporter = std::make_shared<DocumentExporter>(this, scene.get(), &options);
    exporter->initialize(doc);
    exporter->write(doc, true);

    return true;
}

} // namespace mqusd

mqabcAPI MQBasePlugin* mqabcGetExportPlugin()
{
    static mqusd::mqabcExportPlugin s_inst;
    return &s_inst;
}
