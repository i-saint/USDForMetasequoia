#include "pch.h"
#include "mqusd.h"
#include "mqusdExporterWindow.h"

namespace mqusd {

class mqusdExporterPlugin : public MQExportPlugin
{
public:
    mqusdExporterPlugin();
    void GetPlugInID(DWORD* Product, DWORD* ID) override;
    const char* GetPlugInName(void) override;
    const char* EnumFileType(int index) override;
    const char* EnumFileExt(int index) override;
    BOOL ExportFile(int index, const wchar_t* filename, MQDocument doc) override;

private:
    mqusdExporterWindow* m_window = nullptr;
};


mqusdExporterPlugin::mqusdExporterPlugin()
{
    mqusd::mqusdInitialize();
    MQSetLanguage(this);
}

void mqusdExporterPlugin::GetPlugInID(DWORD* Product, DWORD* ID)
{
    *Product = mqusdPluginProduct;
    *ID = mqusdExportPluginID;
}

const char* mqusdExporterPlugin::GetPlugInName(void)
{
    return "USD Exporter (version " mqusdVersionString ") " mqusdCopyRight;
}

const char* mqusdExporterPlugin::EnumFileType(int index)
{
    // note: USDZ is not supported for export
    switch (index) {
    case 0: return "USD (*.usd)";
    case 1: return "USD Ascii (*.usda)";
    case 2: return "USD Crate (*.usdc)";
    default: return nullptr;
    }
}

const char* mqusdExporterPlugin::EnumFileExt(int index)
{
    switch (index) {
    case 0: return "usd";
    case 1: return "usda";
    case 2: return "usdc";
    default: return nullptr;
    }
}

BOOL mqusdExporterPlugin::ExportFile(int /*index*/, const wchar_t* filename, MQDocument /*doc*/)
{
    if (!m_window)
        m_window = new mqusdExporterWindow(this);
    m_window->SetOutputPath(filename);
    m_window->SetVisible(true);
    return true;
}

} // namespace mqusd

mqusdAPI MQBasePlugin* mqusdGetExporterPlugin()
{
    static mqusd::mqusdExporterPlugin s_inst;
    return &s_inst;
}
