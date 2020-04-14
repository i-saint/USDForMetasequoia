#include "pch.h"
#include "mqusd.h"
#include "mqabcExporterWindow.h"

namespace mqusd {

class mqabcExporterPlugin : public MQExportPlugin
{
public:
    mqabcExporterPlugin();
    ~mqabcExporterPlugin();
    void GetPlugInID(DWORD* Product, DWORD* ID) override;
    const char* GetPlugInName(void) override;
    const char* EnumFileType(int index) override;
    const char* EnumFileExt(int index) override;
    BOOL ExportFile(int index, const wchar_t* filename, MQDocument doc) override;

private:
    mqabcExporterWindow* m_window = nullptr;
};


mqabcExporterPlugin::mqabcExporterPlugin()
{
    mqabcInitialize();
}

mqabcExporterPlugin::~mqabcExporterPlugin()
{
    mqabcFinalize();
}

void mqabcExporterPlugin::GetPlugInID(DWORD* Product, DWORD* ID)
{
    *Product = mqabcPluginProduct;
    *ID = mqabcExportPluginID;
}

const char* mqabcExporterPlugin::GetPlugInName(void)
{
    return "Alembic Exporter (version " mqusdVersionString ") " mqusdCopyRight;
}

const char* mqabcExporterPlugin::EnumFileType(int index)
{
    // note: USDZ is not supported for export
    switch (index) {
    case 0: return "Alembic (*.abc)";
    default: return nullptr;
    }
}

const char* mqabcExporterPlugin::EnumFileExt(int index)
{
    switch (index) {
    case 0: return "abc";
    default: return nullptr;
    }
}

BOOL mqabcExporterPlugin::ExportFile(int /*index*/, const wchar_t* filename, MQDocument doc)
{
    if (!m_window)
        m_window = new mqabcExporterWindow(this);
    m_window->SetOutputPath(filename);
    return true;
}

} // namespace mqusd

mqabcAPI MQBasePlugin* mqabcGetExporterPlugin()
{
    static mqusd::mqabcExporterPlugin s_inst;
    return &s_inst;
}
