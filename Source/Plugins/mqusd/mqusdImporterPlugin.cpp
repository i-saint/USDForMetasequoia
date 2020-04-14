#include "pch.h"
#include "mqusd.h"
#include "mqusdImporterWindow.h"

namespace mqusd {

class mqusdImporterPlugin : public MQImportPlugin
{
public:
    mqusdImporterPlugin();
    void GetPlugInID(DWORD* Product, DWORD* ID) override;
    const char* GetPlugInName(void) override;
    const char* EnumFileType(int index) override;
    const char* EnumFileExt(int index) override;
    BOOL ImportFile(int index, const wchar_t* filename, MQDocument doc) override;

private:
    mqusdImporterWindow* m_window = nullptr;
};


mqusdImporterPlugin::mqusdImporterPlugin()
{
    mqusd::mqusdInitialize();
}

void mqusdImporterPlugin::GetPlugInID(DWORD* Product, DWORD* ID)
{
    *Product = mqusdPluginProduct;
    *ID = mqusdImportPluginID;
}

const char* mqusdImporterPlugin::GetPlugInName(void)
{
    return "USD Importer (version " mqusdVersionString ") " mqusdCopyRight;
}

const char* mqusdImporterPlugin::EnumFileType(int index)
{
    switch (index) {
    case 0: return "USD (*.usd)";
    case 1: return "USD Ascii (*.usda)";
    case 2: return "USD Crate (*.usdc)";
    case 3: return "USDZ (*.usdz)";
    default: return nullptr;
    }
}

const char* mqusdImporterPlugin::EnumFileExt(int index)
{
    switch (index) {
    case 0: return "usd";
    case 1: return "usda";
    case 2: return "usdc";
    case 3: return "usdz";
    default: return nullptr;
    }
}

BOOL mqusdImporterPlugin::ImportFile(int /*index*/, const wchar_t* filename, MQDocument doc)
{
    if (!m_window)
        m_window = new mqusdImporterWindow(this);
    m_window->SetVisible(true);
    return m_window->Open(doc, mu::ToMBS(filename).c_str());
}

} // namespace mqusd

mqusdAPI MQBasePlugin* mqusdGetImporterPlugin()
{
    static mqusd::mqusdImporterPlugin s_inst;
    return &s_inst;
}
