#include "pch.h"
#include "mqusd.h"
#include "mqabcPlugin.h"
#include "mqabcWindow.h"
#include "mqabcImportWindow.h"
#include "mqabcExportWindow.h"
#include "mqabcRecorderWindow.h"

#ifdef _WIN32
    #pragma comment(lib, "Half-2_4.lib")
    #pragma comment(lib, "Iex-2_4.lib")
    #pragma comment(lib, "IexMath-2_4.lib")
    #pragma comment(lib, "Imath-2_4.lib")
    #pragma comment(lib, "Alembic.lib")
    #pragma comment(lib, "libhdf5.lib")
    #pragma comment(lib, "libszip.lib")
    #pragma comment(lib, "zlib.lib")
#endif // _WIN32

namespace mqusd {

static mqabcPlugin g_plugin;

mqabcPlugin::mqabcPlugin()
{
    muvgInitialize();
}

mqabcPlugin::~mqabcPlugin()
{
    muvgPrintRecords();
}

#if defined(__APPLE__) || defined(__linux__)
MQBasePlugin* mqabcPlugin::CreateNewPlugin()
{
    return new mqabcPlugin();
}
#endif

void mqabcPlugin::GetPlugInID(DWORD *Product, DWORD *ID)
{
    *Product = mqabcPluginProduct;
    *ID = mqabcPluginID;
}

const char *mqabcPlugin::GetPlugInName(void)
{
    return "Alembic for Metasequoia (version " mqusdVersionString ") " mqusdCopyRight;
}

#if MQPLUGIN_VERSION >= 0x0470
const wchar_t *mqabcPlugin::EnumString(void)
{
    return L"Alembic for Metasequoia";
}
#else
const char *mqabcPlugin::EnumString(void)
{
    return "Alembic for Metasequoia";
}
#endif


const char *mqabcPlugin::EnumSubCommand(int index)
{
    switch (index) {
    case 0: return "Import Alembic";
    case 1: return "Insert Alembic";
    case 2: return "Export Alembic";
    case 3: return "Recording Alembic";
    default: return nullptr;
    }
}

const wchar_t *mqabcPlugin::GetSubCommandString(int index)
{
    switch (index) {
    case 0: return L"Import Alembic";
    case 1: return L"Insert Alembic";
    case 2: return L"Export Alembic";
    case 3: return L"Recording Alembic";
    default: return nullptr;
    }
}

BOOL mqabcPlugin::Initialize()
{
    if (!m_window) {
        auto parent = MQWindow::GetMainWindow();
        m_window = new mqabcWindow(this, parent);
    }
    return TRUE;
}

void mqabcPlugin::Exit()
{
    mqabcImportWindow::each([](auto* w) { delete w; });
    mqabcExportWindow::each([](auto* w) { delete w; });
    mqabcRecorderWindow::each([](auto* w) { delete w; });
    delete m_window;
    m_window = nullptr;
}

BOOL mqabcPlugin::Activate(MQDocument doc, BOOL flag)
{
    bool active = flag ? true : false;
    if (m_window)
        m_window->SetVisible(active);
    return active;
}

BOOL mqabcPlugin::IsActivated(MQDocument doc)
{
    return m_window && m_window->GetVisible();
}

void mqabcPlugin::OnMinimize(MQDocument doc, BOOL flag)
{
}

BOOL mqabcPlugin::OnSubCommand(MQDocument doc, int index)
{
    return FALSE;
}

void mqabcPlugin::OnDraw(MQDocument doc, MQScene scene, int width, int height)
{
    mqabcRecorderWindow::each([doc](auto* w) {
        w->CaptureFrame(doc);
    });
}


void mqabcPlugin::OnNewDocument(MQDocument doc, const char *filename, NEW_DOCUMENT_PARAM& param)
{
    m_mqo_path = filename ? filename : "";

    mqabcRecorderWindow::each([doc](auto* w) {
        w->MarkSceneDirty();
    });
}

void mqabcPlugin::OnEndDocument(MQDocument doc)
{
    m_mqo_path.clear();
}

void mqabcPlugin::OnSaveDocument(MQDocument doc, const char *filename, SAVE_DOCUMENT_PARAM& param)
{
    m_mqo_path = filename ? filename : "";
}

BOOL mqabcPlugin::OnUndo(MQDocument doc, int undo_state)
{
    mqabcRecorderWindow::each([doc](auto* w) {
        w->MarkSceneDirty();
    });
    return TRUE;
}

BOOL mqabcPlugin::OnRedo(MQDocument doc, int redo_state)
{
    mqabcRecorderWindow::each([doc](auto* w) {
        w->MarkSceneDirty();
    });
    return TRUE;
}

void mqabcPlugin::OnUpdateUndo(MQDocument doc, int undo_state, int undo_size)
{
    mqabcRecorderWindow::each([doc](auto* w) {
        w->MarkSceneDirty();
    });
}

bool mqabcPlugin::ExecuteCallback(MQDocument /*doc*/, void* /*option*/)
{
    return false;
}

void mqabcPlugin::LogInfo(const char* message)
{
}

const std::string& mqabcPlugin::GetMQOPath() const
{
    return m_mqo_path;
}

void mqusdLog(const char* fmt, ...)
{
    const size_t bufsize = 1024 * 16;
    static char* s_buf = new char[bufsize];
    va_list args;
    va_start(args, fmt);
    vsnprintf(s_buf, bufsize, fmt, args);
    va_end(args);

    g_plugin.LogInfo(s_buf);
}

} // namespace mqusd


MQBasePlugin* GetPluginClass()
{
    return &mqusd::g_plugin;
}


mqusdAPI MQBasePlugin* mqusdGetStationPlugin()
{
    static mqusd::mqabcPlugin s_inst;
    return &s_inst;
}
