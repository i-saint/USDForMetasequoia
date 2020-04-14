#include "pch.h"
#include "mqusd.h"
#include "mqabcImporterWindow.h"

namespace mqusd {

class mqabcImporterPlugin : public MQImportPlugin
{
public:
    mqabcImporterPlugin();
    ~mqabcImporterPlugin();
    void GetPlugInID(DWORD* Product, DWORD* ID) override;
    const char* GetPlugInName(void) override;
    const char* EnumFileType(int index) override;
    const char* EnumFileExt(int index) override;
    BOOL ImportFile(int index, const wchar_t* filename, MQDocument doc) override;

private:
    mqabcImporterWindow* m_window = nullptr;
};


mqabcImporterPlugin::mqabcImporterPlugin()
{
    mqabcInitialize();
}

mqabcImporterPlugin::~mqabcImporterPlugin()
{
    mqabcFinalize();
}

void mqabcImporterPlugin::GetPlugInID(DWORD* Product, DWORD* ID)
{
    *Product = mqabcPluginProduct;
    *ID = mqabcImportPluginID;
}

const char* mqabcImporterPlugin::GetPlugInName(void)
{
    return "Alembic Importer (version " mqusdVersionString ") " mqusdCopyRight;
}

const char* mqabcImporterPlugin::EnumFileType(int index)
{
    switch (index) {
    case 0: return "Alembic (*.abc)";
    default: return nullptr;
    }
}

const char* mqabcImporterPlugin::EnumFileExt(int index)
{
    switch (index) {
    case 0: return "abc";
    default: return nullptr;
    }
}

BOOL mqabcImporterPlugin::ImportFile(int /*index*/, const wchar_t* filename, MQDocument doc)
{
    if(!m_window)
        m_window = new mqabcImporterWindow(this);
    m_window->SetVisible(true);
    return m_window->Open(doc, mu::ToMBS(filename).c_str());
}

} // namespace mqusd

mqabcAPI MQBasePlugin* mqabcGetImporterPlugin()
{
    static mqusd::mqabcImporterPlugin s_inst;
    return &s_inst;
}
