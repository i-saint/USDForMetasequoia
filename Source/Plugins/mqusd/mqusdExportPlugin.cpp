#include "pch.h"
#include "mqusd.h"
#include "mqCommon/mqDocumentExporter.h"

namespace mqusd {

class mqusdExportPlugin : public MQExportPlugin
{
public:
    void GetPlugInID(DWORD* Product, DWORD* ID) override;
    const char* GetPlugInName(void) override;
    const char* EnumFileType(int index) override;
    const char* EnumFileExt(int index) override;
    BOOL ExportFile(int index, const wchar_t* filename, MQDocument doc) override;
};


void mqusdExportPlugin::GetPlugInID(DWORD* Product, DWORD* ID)
{
    *Product = mqusdPluginProduct;
    *ID = mqusdExportPluginID;
}

const char* mqusdExportPlugin::GetPlugInName(void)
{
    return "USD Exporter (version " mqusdVersionString ") " mqusdCopyRight;
}

const char* mqusdExportPlugin::EnumFileType(int index)
{
    // note: USDZ is not supported for export
    switch (index) {
    case 0: return "USD (*.usd)";
    case 1: return "USD Ascii (*.usda)";
    case 2: return "USD Crate (*.usdc)";
    default: return nullptr;
    }
}

const char* mqusdExportPlugin::EnumFileExt(int index)
{
    switch (index) {
    case 0: return "usd";
    case 1: return "usda";
    case 2: return "usdc";
    default: return nullptr;
    }
}

BOOL mqusdExportPlugin::ExportFile(int /*index*/, const wchar_t* filename, MQDocument doc)
{
    ScenePtr scene = CreateUSDScene();
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

mqusdAPI MQBasePlugin* mqusdGetExportPlugin()
{
    static mqusd::mqusdExportPlugin s_inst;
    return &s_inst;
}
