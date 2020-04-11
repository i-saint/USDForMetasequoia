#include "pch.h"
#include "mqusd.h"
#include "mqusdImportWindow.h"

namespace mqusd {

class mqusdImportPlugin : public MQImportPlugin
{
public:
    void GetPlugInID(DWORD* Product, DWORD* ID) override;
    const char* GetPlugInName(void) override;
    const char* EnumFileType(int index) override;
    const char* EnumFileExt(int index) override;
    BOOL ImportFile(int index, const wchar_t* filename, MQDocument doc) override;
};


void mqusdImportPlugin::GetPlugInID(DWORD* Product, DWORD* ID)
{
    *Product = mqusdPluginProduct;
    *ID = mqusdImportPluginID;
}

const char* mqusdImportPlugin::GetPlugInName(void)
{
    return "USD Importer (version " mqusdVersionString ") " mqusdCopyRight;
}

const char* mqusdImportPlugin::EnumFileType(int index)
{
    switch (index) {
    case 0: return "USD (*.usd)";
    case 1: return "USD Ascii (*.usda)";
    case 2: return "USD Crate (*.usdc)";
    case 3: return "USDZ (*.usdz)";
    default: return nullptr;
    }
}

const char* mqusdImportPlugin::EnumFileExt(int index)
{
    switch (index) {
    case 0: return "usd";
    case 1: return "usda";
    case 2: return "usdc";
    case 3: return "usdz";
    default: return nullptr;
    }
}

BOOL mqusdImportPlugin::ImportFile(int /*index*/, const wchar_t* filename, MQDocument doc)
{
    auto w = mqusdImportWindow::create(this);
    return w->Open(doc, mu::ToMBS(filename).c_str());
}

} // namespace mqusd

mqusdAPI MQBasePlugin* mqusdGetImportPlugin()
{
    static mqusd::mqusdImportPlugin s_inst;
    return &s_inst;
}
