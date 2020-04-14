#include "pch.h"
#include "mqusd.h"
#include "mqusdRecorderWindow.h"
#include "mqusdRecorderPlugin.h"

namespace mqusd {

mqusdRecorderPlugin::mqusdRecorderPlugin()
{
    mqusd::mqusdInitialize();
}

mqusdRecorderPlugin::~mqusdRecorderPlugin()
{
    muvgPrintRecords();
}

#if defined(__APPLE__) || defined(__linux__)
MQBasePlugin* mqusdRecorderPlugin::CreateNewPlugin()
{
    return new mqusdRecorderPlugin();
}
#endif

void mqusdRecorderPlugin::GetPlugInID(DWORD *Product, DWORD *ID)
{
    *Product = mqusdPluginProduct;
    *ID = mqusdPluginID;
}

const char *mqusdRecorderPlugin::GetPlugInName(void)
{
    return "USD Recorder (version " mqusdVersionString ") " mqusdCopyRight;
}

#if MQPLUGIN_VERSION >= 0x0470
const wchar_t *mqusdRecorderPlugin::EnumString(void)
{
    return L"USD Recorder";
}
#else
const char *mqusdRecorderPlugin::EnumString(void)
{
    return "USD Recorder";
}
#endif


const char *mqusdRecorderPlugin::EnumSubCommand(int index)
{
    return nullptr;
}

const wchar_t *mqusdRecorderPlugin::GetSubCommandString(int index)
{
    return nullptr;
}

BOOL mqusdRecorderPlugin::Initialize()
{
    return TRUE;
}

void mqusdRecorderPlugin::Exit()
{
    delete m_window;
    m_window = nullptr;
}

BOOL mqusdRecorderPlugin::Activate(MQDocument doc, BOOL flag)
{
    bool active = flag ? true : false;
    if (!m_window)
        m_window = new mqusdRecorderWindow(this);
    m_window->SetVisible(active);
    return active;
}

BOOL mqusdRecorderPlugin::IsActivated(MQDocument doc)
{
    return m_window && m_window->GetVisible();
}

void mqusdRecorderPlugin::OnMinimize(MQDocument doc, BOOL flag)
{
}

void mqusdRecorderPlugin::OnDraw(MQDocument doc, MQScene scene, int width, int height)
{
    if (m_window)
        m_window->CaptureFrame(doc);
}


void mqusdRecorderPlugin::OnNewDocument(MQDocument doc, const char *filename, NEW_DOCUMENT_PARAM& param)
{
    m_mqo_path = filename ? filename : "";

    if (m_window)
        m_window->MarkSceneDirty();
}

void mqusdRecorderPlugin::OnEndDocument(MQDocument doc)
{
    m_mqo_path.clear();
}

void mqusdRecorderPlugin::OnSaveDocument(MQDocument doc, const char *filename, SAVE_DOCUMENT_PARAM& param)
{
    m_mqo_path = filename ? filename : "";
}

BOOL mqusdRecorderPlugin::OnUndo(MQDocument doc, int undo_state)
{
    if (m_window)
        m_window->MarkSceneDirty();
    return TRUE;
}

BOOL mqusdRecorderPlugin::OnRedo(MQDocument doc, int redo_state)
{
    if (m_window)
        m_window->MarkSceneDirty();
    return TRUE;
}

void mqusdRecorderPlugin::OnUpdateUndo(MQDocument doc, int undo_state, int undo_size)
{
    if (m_window)
        m_window->MarkSceneDirty();
}

bool mqusdRecorderPlugin::ExecuteCallback(MQDocument /*doc*/, void* /*option*/)
{
    return false;
}


void mqusdRecorderPlugin::LogInfo(const char* message)
{
}

const std::string& mqusdRecorderPlugin::GetMQOPath() const
{
    return m_mqo_path;
}

} // namespace mqusd


mqusdAPI MQBasePlugin* mqusdGetRecorderPlugin()
{
    static mqusd::mqusdRecorderPlugin s_inst;
    return &s_inst;
}
