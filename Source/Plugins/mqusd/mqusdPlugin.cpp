#include "pch.h"
#include "mqusd.h"
#include "mqusdPlugin.h"
#include "mqusdWindow.h"
#include "mqusdImportWindow.h"
#include "mqusdExportWindow.h"
#include "mqusdRecorderWindow.h"

namespace mqusd {

static mqusdPlugin g_plugin;

mqusdPlugin::mqusdPlugin()
{
    muvgInitialize();
#ifdef _WIN32
    sg::SetUSDModuleDir(GetPluginsDir() + "Misc" muPathSep "mqusd");
#endif
}

mqusdPlugin::~mqusdPlugin()
{
    muvgPrintRecords();
}

#if defined(__APPLE__) || defined(__linux__)
MQBasePlugin* mqusdPlugin::CreateNewPlugin()
{
    return new mqusdPlugin();
}
#endif

void mqusdPlugin::GetPlugInID(DWORD *Product, DWORD *ID)
{
    *Product = mqusdPluginProduct;
    *ID = mqusdPluginID;
}

const char *mqusdPlugin::GetPlugInName(void)
{
    return "USD For Metasequoia (version " mqusdVersionString ") " mqusdCopyRight;
}

#if MQPLUGIN_VERSION >= 0x0470
const wchar_t *mqusdPlugin::EnumString(void)
{
    return L"USD For Metasequoia";
}
#else
const char *mqusdPlugin::EnumString(void)
{
    return "USD For Metasequoia";
}
#endif


const char *mqusdPlugin::EnumSubCommand(int index)
{
    switch (index) {
    case 0: return "Import USD";
    case 1: return "Insert USD";
    case 2: return "Export USD";
    case 3: return "Recording USD";
    default: return nullptr;
    }
}

const wchar_t *mqusdPlugin::GetSubCommandString(int index)
{
    switch (index) {
    case 0: return L"Import USD";
    case 1: return L"Insert USD";
    case 2: return L"Export USD";
    case 3: return L"Recording USD";
    default: return nullptr;
    }
}

BOOL mqusdPlugin::Initialize()
{
    if (!m_window) {
        auto parent = MQWindow::GetMainWindow();
        m_window = new mqusdWindow(this, parent);
    }
    return TRUE;
}

void mqusdPlugin::Exit()
{
    mqusdImportWindow::each([](auto* w) { delete w; });
    mqusdExportWindow::each([](auto* w) { delete w; });
    mqusdRecorderWindow::each([](auto* w) { delete w; });
    delete m_window;
    m_window = nullptr;
}

BOOL mqusdPlugin::Activate(MQDocument doc, BOOL flag)
{
    bool active = flag ? true : false;
    if (m_window)
        m_window->SetVisible(active);
    return active;
}

BOOL mqusdPlugin::IsActivated(MQDocument doc)
{
    return m_window && m_window->GetVisible();
}

void mqusdPlugin::OnMinimize(MQDocument doc, BOOL flag)
{
}

int mqusdPlugin::OnReceiveUserMessage(MQDocument doc, DWORD src_product, DWORD src_id, const char *description, void *message)
{
    return 0;
}

BOOL mqusdPlugin::OnSubCommand(MQDocument doc, int index)
{
    return FALSE;
}

void mqusdPlugin::OnDraw(MQDocument doc, MQScene scene, int width, int height)
{
    mqusdRecorderWindow::each([doc](auto* w) {
        w->CaptureFrame(doc);
    });
}


void mqusdPlugin::OnNewDocument(MQDocument doc, const char *filename, NEW_DOCUMENT_PARAM& param)
{
    m_mqo_path = filename ? filename : "";

    mqusdRecorderWindow::each([doc](auto* w) {
        w->MarkSceneDirty();
    });
}

void mqusdPlugin::OnEndDocument(MQDocument doc)
{
    m_mqo_path.clear();
}

void mqusdPlugin::OnSaveDocument(MQDocument doc, const char *filename, SAVE_DOCUMENT_PARAM& param)
{
    m_mqo_path = filename ? filename : "";
}

BOOL mqusdPlugin::OnUndo(MQDocument doc, int undo_state)
{
    mqusdRecorderWindow::each([doc](auto* w) {
        w->MarkSceneDirty();
    });
    return TRUE;
}

BOOL mqusdPlugin::OnRedo(MQDocument doc, int redo_state)
{
    mqusdRecorderWindow::each([doc](auto* w) {
        w->MarkSceneDirty();
    });
    return TRUE;
}

void mqusdPlugin::OnUpdateUndo(MQDocument doc, int undo_state, int undo_size)
{
    mqusdRecorderWindow::each([doc](auto* w) {
        w->MarkSceneDirty();
    });
}

bool mqusdPlugin::ExecuteCallback(MQDocument /*doc*/, void* /*option*/)
{
    return false;
}


void mqusdPlugin::LogInfo(const char* message)
{
}

const std::string& mqusdPlugin::GetMQOPath() const
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
    static mqusd::mqusdPlugin s_inst;
    return &s_inst;
}
