#include "pch.h"
#include "mqusd.h"
#include "mqabcRecorderPlugin.h"
#include "mqabcRecorderWindow.h"

namespace mqusd {

mqabcRecorderPlugin::mqabcRecorderPlugin()
{
    mqabcInitialize();
}

mqabcRecorderPlugin::~mqabcRecorderPlugin()
{
    mqabcFinalize();
}

#if defined(__APPLE__) || defined(__linux__)
MQBasePlugin* mqabcRecorderPlugin::CreateNewPlugin()
{
    return new mqabcRecorderPlugin();
}
#endif

void mqabcRecorderPlugin::GetPlugInID(DWORD *Product, DWORD *ID)
{
    *Product = mqabcPluginProduct;
    *ID = mqabcPluginID;
}

const char *mqabcRecorderPlugin::GetPlugInName(void)
{
    return "Alembic Recorder (version " mqusdVersionString ") " mqusdCopyRight;
}

#if MQPLUGIN_VERSION >= 0x0470
const wchar_t *mqabcRecorderPlugin::EnumString(void)
{
    return L"Alembic Recorder";
}
#else
const char *mqabcRecorderPlugin::EnumString(void)
{
    return "Alembic Recorder";
}
#endif


const char *mqabcRecorderPlugin::EnumSubCommand(int index)
{
    return nullptr;
}

const wchar_t *mqabcRecorderPlugin::GetSubCommandString(int index)
{
    return nullptr;
}

BOOL mqabcRecorderPlugin::Initialize()
{
    return TRUE;
}

void mqabcRecorderPlugin::Exit()
{
    delete m_window;
    m_window = nullptr;
}

BOOL mqabcRecorderPlugin::Activate(MQDocument doc, BOOL flag)
{
    bool active = flag ? true : false;
    if (!m_window)
        m_window = mqabcRecorderWindow::create(this);
    if (m_window)
        m_window->SetVisible(active);
    return active;
}

BOOL mqabcRecorderPlugin::IsActivated(MQDocument doc)
{
    return m_window && m_window->GetVisible();
}

void mqabcRecorderPlugin::OnMinimize(MQDocument doc, BOOL flag)
{
}

BOOL mqabcRecorderPlugin::OnSubCommand(MQDocument doc, int index)
{
    return FALSE;
}

void mqabcRecorderPlugin::OnDraw(MQDocument doc, MQScene scene, int width, int height)
{
    if (m_window)
        m_window->CaptureFrame(doc);
}


void mqabcRecorderPlugin::OnNewDocument(MQDocument doc, const char *filename, NEW_DOCUMENT_PARAM& param)
{
    m_mqo_path = filename ? filename : "";

    if (m_window)
        m_window->MarkSceneDirty();
}

void mqabcRecorderPlugin::OnEndDocument(MQDocument doc)
{
    m_mqo_path.clear();
}

void mqabcRecorderPlugin::OnSaveDocument(MQDocument doc, const char *filename, SAVE_DOCUMENT_PARAM& param)
{
    m_mqo_path = filename ? filename : "";
}

BOOL mqabcRecorderPlugin::OnUndo(MQDocument doc, int undo_state)
{
    if (m_window)
        m_window->MarkSceneDirty();
    return TRUE;
}

BOOL mqabcRecorderPlugin::OnRedo(MQDocument doc, int redo_state)
{
    if (m_window)
        m_window->MarkSceneDirty();
    return TRUE;
}

void mqabcRecorderPlugin::OnUpdateUndo(MQDocument doc, int undo_state, int undo_size)
{
    if (m_window)
        m_window->MarkSceneDirty();
}

bool mqabcRecorderPlugin::ExecuteCallback(MQDocument /*doc*/, void* /*option*/)
{
    return false;
}

void mqabcRecorderPlugin::LogInfo(const char* message)
{
}

const std::string& mqabcRecorderPlugin::GetMQOPath() const
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
}

} // namespace mqusd


mqusdAPI MQBasePlugin* mqabcGetRecorderPlugin()
{
    static mqusd::mqabcRecorderPlugin s_inst;
    return &s_inst;
}
