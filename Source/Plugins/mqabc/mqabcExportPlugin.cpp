#include "pch.h"
#include "mqusd.h"
#include "mqabcExportWindow.h"

namespace mqusd {

class mqabcExportPlugin : public MQExportPlugin
{
public:
    mqabcExportPlugin();
    ~mqabcExportPlugin();
    void GetPlugInID(DWORD* Product, DWORD* ID) override;
    const char* GetPlugInName(void) override;
    const char* EnumFileType(int index) override;
    const char* EnumFileExt(int index) override;
    BOOL ExportFile(int index, const wchar_t* filename, MQDocument doc) override;
};


mqabcExportPlugin::mqabcExportPlugin()
{
    mqabcInitialize();
}

mqabcExportPlugin::~mqabcExportPlugin()
{
    mqabcFinalize();
}

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
    auto w = mqabcExportWindow::create(this);
    w->SetOutputPath(filename);
    return true;
}

} // namespace mqusd

mqabcAPI MQBasePlugin* mqabcGetExportPlugin()
{
    static mqusd::mqabcExportPlugin s_inst;
    return &s_inst;
}
