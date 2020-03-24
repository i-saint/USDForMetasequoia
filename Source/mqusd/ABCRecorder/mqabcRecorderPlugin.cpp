#include "pch.h"
#include "mqusd.h"
#include "mqabcRecorderPlugin.h"
#include "mqabcRecorderWindow.h"
#include "ABCCore/mqabc.h"

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

static mqabcRecorderPlugin g_plugin;

// Constructor
// コンストラクタ
mqabcRecorderPlugin::mqabcRecorderPlugin()
{
    m_options.export_skeletons = false;
    m_options.export_blendshapes = false;
}

// Destructor
// デストラクタ
mqabcRecorderPlugin::~mqabcRecorderPlugin()
{
}

#if defined(__APPLE__) || defined(__linux__)
// Create a new plugin class for another document.
// 別のドキュメントのための新しいプラグインクラスを作成する。
MQBasePlugin* mqabcRecorderPlugin::CreateNewPlugin()
{
    return new mqabcRecorderPlugin();
}
#endif

//---------------------------------------------------------------------------
//  GetPlugInID
//    プラグインIDを返す。
//    この関数は起動時に呼び出される。
//---------------------------------------------------------------------------
void mqabcRecorderPlugin::GetPlugInID(DWORD *Product, DWORD *ID)
{
    // プロダクト名(制作者名)とIDを、全部で64bitの値として返す
    // 値は他と重複しないようなランダムなもので良い
    *Product = mqabcPluginProduct;
    *ID = mqabcRecorderPluginID;
}

//---------------------------------------------------------------------------
//  GetPlugInName
//    プラグイン名を返す。
//    この関数は[プラグインについて]表示時に呼び出される。
//---------------------------------------------------------------------------
const char *mqabcRecorderPlugin::GetPlugInName(void)
{
    return "Alembic Recorder (version " mqusdVersionString ") " mqusdCopyRight;
}

//---------------------------------------------------------------------------
//  EnumString
//    ポップアップメニューに表示される文字列を返す。
//    この関数は起動時に呼び出される。
//---------------------------------------------------------------------------
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



//---------------------------------------------------------------------------
//  EnumSubCommand
//    サブコマンド前を列挙
//---------------------------------------------------------------------------
const char *mqabcRecorderPlugin::EnumSubCommand(int index)
{
    return NULL;
}

//---------------------------------------------------------------------------
//  GetSubCommandString
//    サブコマンドの文字列を列挙
//---------------------------------------------------------------------------
const wchar_t *mqabcRecorderPlugin::GetSubCommandString(int index)
{
    return NULL;
}

//---------------------------------------------------------------------------
//  Initialize
//    アプリケーションの初期化
//---------------------------------------------------------------------------
BOOL mqabcRecorderPlugin::Initialize()
{
    if (!m_window) {
        auto parent = MQWindow::GetMainWindow();
        m_window = new mqabcRecorderWindow(this, parent);
    }
    return TRUE;
}

//---------------------------------------------------------------------------
//  Exit
//    アプリケーションの終了
//---------------------------------------------------------------------------
void mqabcRecorderPlugin::Exit()
{
    if (m_window) {
        delete m_window;
        m_window = nullptr;
    }
    CloseABC();
}

//---------------------------------------------------------------------------
//  Activate
//    表示・非表示切り替え要求
//---------------------------------------------------------------------------
BOOL mqabcRecorderPlugin::Activate(MQDocument doc, BOOL flag)
{
    if (!m_window) {
        return FALSE;
    }

    bool active = flag ? true : false;
    m_window->SetVisible(active);
    return active;
}

//---------------------------------------------------------------------------
//  IsActivated
//    表示・非表示状態の返答
//---------------------------------------------------------------------------
BOOL mqabcRecorderPlugin::IsActivated(MQDocument doc)
{
    if (!m_window) {
        return FALSE;
    }
    return m_window->GetVisible();
}

//---------------------------------------------------------------------------
//  OnMinimize
//    ウインドウの最小化への返答
//---------------------------------------------------------------------------
void mqabcRecorderPlugin::OnMinimize(MQDocument doc, BOOL flag)
{
}

//---------------------------------------------------------------------------
//  OnReceiveUserMessage
//    プラグイン独自のメッセージの受け取り
//---------------------------------------------------------------------------
int mqabcRecorderPlugin::OnReceiveUserMessage(MQDocument doc, DWORD src_product, DWORD src_id, const char *description, void *message)
{
    return 0;
}

//---------------------------------------------------------------------------
//  OnSubCommand
//    A message for calling a sub comand
//    サブコマンドの呼び出し
//---------------------------------------------------------------------------
BOOL mqabcRecorderPlugin::OnSubCommand(MQDocument doc, int index)
{
    return FALSE;
}

//---------------------------------------------------------------------------
//  OnDraw
//    描画時の処理
//---------------------------------------------------------------------------
void mqabcRecorderPlugin::OnDraw(MQDocument doc, MQScene scene, int width, int height)
{
    CaptureFrame(doc);
}


//---------------------------------------------------------------------------
//  OnNewDocument
//    ドキュメント初期化時
//---------------------------------------------------------------------------
void mqabcRecorderPlugin::OnNewDocument(MQDocument doc, const char *filename, NEW_DOCUMENT_PARAM& param)
{
    m_mqo_path = filename ? filename : "";
    MarkSceneDirty();
}

//---------------------------------------------------------------------------
//  OnEndDocument
//    ドキュメント終了時
//---------------------------------------------------------------------------
void mqabcRecorderPlugin::OnEndDocument(MQDocument doc)
{
    m_mqo_path.clear();
    CloseABC();
}

//---------------------------------------------------------------------------
//  OnSaveDocument
//    ドキュメント保存時
//---------------------------------------------------------------------------
void mqabcRecorderPlugin::OnSaveDocument(MQDocument doc, const char *filename, SAVE_DOCUMENT_PARAM& param)
{
    m_mqo_path = filename ? filename : "";
}

//---------------------------------------------------------------------------
//  OnUndo
//    アンドゥ実行時
//---------------------------------------------------------------------------
BOOL mqabcRecorderPlugin::OnUndo(MQDocument doc, int undo_state)
{
    MarkSceneDirty();
    return TRUE;
}

//---------------------------------------------------------------------------
//  OnRedo
//    リドゥ実行時
//---------------------------------------------------------------------------
BOOL mqabcRecorderPlugin::OnRedo(MQDocument doc, int redo_state)
{
    MarkSceneDirty();
    return TRUE;
}

//---------------------------------------------------------------------------
//  OnUpdateUndo
//    アンドゥ状態更新時
//---------------------------------------------------------------------------
void mqabcRecorderPlugin::OnUpdateUndo(MQDocument doc, int undo_state, int undo_size)
{
    MarkSceneDirty();
}

//---------------------------------------------------------------------------
//  OnObjectModified
//    オブジェクトの編集時
//---------------------------------------------------------------------------
void mqabcRecorderPlugin::OnObjectModified(MQDocument doc)
{
}

//---------------------------------------------------------------------------
//  OnObjectSelected
//    オブジェクトの選択状態の変更時
//---------------------------------------------------------------------------
void mqabcRecorderPlugin::OnObjectSelected(MQDocument doc)
{
}

//---------------------------------------------------------------------------
//  OnUpdateObjectList
//    カレントオブジェクトの変更時
//---------------------------------------------------------------------------
void mqabcRecorderPlugin::OnUpdateObjectList(MQDocument doc)
{
}

//---------------------------------------------------------------------------
//  OnMaterialModified
//    マテリアルのパラメータ変更時
//---------------------------------------------------------------------------
void mqabcRecorderPlugin::OnMaterialModified(MQDocument doc)
{
}

//---------------------------------------------------------------------------
//  OnUpdateMaterialList
//    カレントマテリアルの変更時
//---------------------------------------------------------------------------
void mqabcRecorderPlugin::OnUpdateMaterialList(MQDocument doc)
{
}

//---------------------------------------------------------------------------
//  OnUpdateScene
//    シーン情報の変更時
//---------------------------------------------------------------------------
void mqabcRecorderPlugin::OnUpdateScene(MQDocument doc, MQScene scene)
{
}

//---------------------------------------------------------------------------
//  ExecuteCallback
//    コールバックに対する実装部
//---------------------------------------------------------------------------
bool mqabcRecorderPlugin::ExecuteCallback(MQDocument doc, void *option)
{
    CallbackInfo *info = (CallbackInfo*)option;
    return ((*this).*info->proc)(doc);
}

// コールバックの呼び出し
void mqabcRecorderPlugin::Execute(ExecuteCallbackProc proc)
{
    CallbackInfo info;
    info.proc = proc;
    BeginCallback(&info);
}

void mqabcRecorderPlugin::LogInfo(const char* message)
{
    if (m_window)
        m_window->LogInfo(message);
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


// impl

bool mqabcRecorderPlugin::OpenABC(MQDocument doc, const std::string& path)
{
    CloseABC();

    m_scene = CreateABCOScene();
    if (!m_scene)
        return false;

    if (!m_scene->create(path.c_str())) {
        m_scene = {};
        return false;
    }

    m_exporter.reset(new DocumentExporter(this, m_scene.get(), &m_options));
    m_exporter->initialize(doc);

    m_recording = true;
    m_dirty = true;

    mqusdLog("succeeded to open %s\nrecording started", path.c_str());
    return true;
}

bool mqabcRecorderPlugin::CloseABC()
{
    if (m_recording) {
        m_exporter = {};
        m_scene = {};
        m_recording = false;

        mqusdLog("recording finished");
    }
    return true;
}

void mqabcRecorderPlugin::CaptureFrame(MQDocument doc)
{
    if (!IsRecording() || !m_dirty)
        return;

    if (m_exporter->write(doc, true)) {
        m_dirty = false;
    }
}

const std::string& mqabcRecorderPlugin::GetMQOPath() const
{
    return m_mqo_path;
}
const std::string& mqabcRecorderPlugin::GetABCPath() const
{
    return m_scene->path;
}

bool mqabcRecorderPlugin::IsArchiveOpened() const
{
    return m_scene != nullptr;
}

bool mqabcRecorderPlugin::IsRecording() const
{
    return m_exporter && m_recording;
}
void mqabcRecorderPlugin::SetRecording(bool v)
{
    m_recording = v;
}

ExportOptions& mqabcRecorderPlugin::GetOptions()
{
    return m_options;
}

void mqabcRecorderPlugin::MarkSceneDirty()
{
    m_dirty = true;
}


} // namespace mqusd

MQBasePlugin* GetPluginClass()
{
    return &mqusd::g_plugin;
}


#ifdef _WIN32
//---------------------------------------------------------------------------
//  DllMain
//---------------------------------------------------------------------------
BOOL APIENTRY DllMain(HINSTANCE hInstance,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        HRESULT hRes = ::CoInitialize(NULL);

        return SUCCEEDED(hRes);
    }

    if (ul_reason_for_call == DLL_PROCESS_DETACH)
    {
        ::CoUninitialize();
    }

    return TRUE;
}
#endif
