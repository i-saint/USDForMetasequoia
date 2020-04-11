#include "pch.h"
#include "mqusd.h"
#include "mqabcImportWindow.h"

namespace mqusd {

class mqabcImportPlugin : public MQImportPlugin
{
public:
    mqabcImportPlugin();
    ~mqabcImportPlugin();
    void GetPlugInID(DWORD* Product, DWORD* ID) override;
    const char* GetPlugInName(void) override;
    const char* EnumFileType(int index) override;
    const char* EnumFileExt(int index) override;
    BOOL ImportFile(int index, const wchar_t* filename, MQDocument doc) override;
};


mqabcImportPlugin::mqabcImportPlugin()
{
    mqabcInitialize();
}

mqabcImportPlugin::~mqabcImportPlugin()
{
    mqabcFinalize();
}

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
    auto w = mqabcImportWindow::create(this);
    return w->Open(doc, mu::ToMBS(filename).c_str());
}

} // namespace mqusd

mqabcAPI MQBasePlugin* mqabcGetImportPlugin()
{
    static mqusd::mqabcImportPlugin s_inst;
    return &s_inst;
}
