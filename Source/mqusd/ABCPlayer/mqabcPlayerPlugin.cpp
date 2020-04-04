#include "pch.h"
#include "mqusd.h"
#include "mqabcPlayerPlugin.h"
#include "mqabcPlayerWindow.h"
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

static mqabcPlayerPlugin g_plugin;

// Constructor
// コンストラクタ
mqabcPlayerPlugin::mqabcPlayerPlugin()
{
}

// Destructor
// デストラクタ
mqabcPlayerPlugin::~mqabcPlayerPlugin()
{
}

#if defined(__APPLE__) || defined(__linux__)
// Create a new plugin class for another document.
// 別のドキュメントのための新しいプラグインクラスを作成する。
MQBasePlugin* mqabcPlayerPlugin::CreateNewPlugin()
{
    return new mqabcPlayerPlugin();
}
#endif

//---------------------------------------------------------------------------
//  GetPlugInID
//    プラグインIDを返す。
//    この関数は起動時に呼び出される。
//---------------------------------------------------------------------------
void mqabcPlayerPlugin::GetPlugInID(DWORD *Product, DWORD *ID)
{
    // プロダクト名(制作者名)とIDを、全部で64bitの値として返す
    // 値は他と重複しないようなランダムなもので良い
    *Product = mqabcPluginProduct;
    *ID = mqabcPluginID;
}

//---------------------------------------------------------------------------
//  GetPlugInName
//    プラグイン名を返す。
//    この関数は[プラグインについて]表示時に呼び出される。
//---------------------------------------------------------------------------
const char *mqabcPlayerPlugin::GetPlugInName(void)
{
    return "Alembic for Metasequoia (version " mqusdVersionString ") " mqusdCopyRight;
}

//---------------------------------------------------------------------------
//  EnumString
//    ポップアップメニューに表示される文字列を返す。
//    この関数は起動時に呼び出される。
//---------------------------------------------------------------------------
#if MQPLUGIN_VERSION >= 0x0470
const wchar_t *mqabcPlayerPlugin::EnumString(void)
{
    return L"Alembic for Metasequoia";
}
#else
const char *mqabcPlayerPlugin::EnumString(void)
{
    return "Alembic for Metasequoia";
}
#endif



//---------------------------------------------------------------------------
//  EnumSubCommand
//    サブコマンド前を列挙
//---------------------------------------------------------------------------
const char *mqabcPlayerPlugin::EnumSubCommand(int index)
{
    return NULL;
}

//---------------------------------------------------------------------------
//  GetSubCommandString
//    サブコマンドの文字列を列挙
//---------------------------------------------------------------------------
const wchar_t *mqabcPlayerPlugin::GetSubCommandString(int index)
{
    return NULL;
}

//---------------------------------------------------------------------------
//  Initialize
//    アプリケーションの初期化
//---------------------------------------------------------------------------
BOOL mqabcPlayerPlugin::Initialize()
{
    auto parent = MQWindow::GetMainWindow();
    if (!m_player) {
        m_player = new mqabcPlayerWindow(this, parent);
    }
    if (!m_recorder) {
        m_recorder = new mqabcRecorderWindow(this, parent);
    }
    return TRUE;
}

//---------------------------------------------------------------------------
//  Exit
//    アプリケーションの終了
//---------------------------------------------------------------------------
void mqabcPlayerPlugin::Exit()
{
    CloseAll();
}

//---------------------------------------------------------------------------
//  Activate
//    表示・非表示切り替え要求
//---------------------------------------------------------------------------
BOOL mqabcPlayerPlugin::Activate(MQDocument doc, BOOL flag)
{
    bool active = flag ? true : false;
    if (m_player) {
        m_player->SetVisible(active);
    }
    if (m_recorder) {
        m_recorder->SetVisible(active);
    }

    return active;
}

//---------------------------------------------------------------------------
//  IsActivated
//    表示・非表示状態の返答
//---------------------------------------------------------------------------
BOOL mqabcPlayerPlugin::IsActivated(MQDocument doc)
{
    return
        (m_player && m_player->GetVisible()) ||
        (m_recorder && m_recorder->GetVisible());
}

//---------------------------------------------------------------------------
//  OnMinimize
//    ウインドウの最小化への返答
//---------------------------------------------------------------------------
void mqabcPlayerPlugin::OnMinimize(MQDocument doc, BOOL flag)
{
}

//---------------------------------------------------------------------------
//  OnReceiveUserMessage
//    プラグイン独自のメッセージの受け取り
//---------------------------------------------------------------------------
int mqabcPlayerPlugin::OnReceiveUserMessage(MQDocument doc, DWORD src_product, DWORD src_id, const char *description, void *message)
{
    return 0;
}

//---------------------------------------------------------------------------
//  OnSubCommand
//    A message for calling a sub comand
//    サブコマンドの呼び出し
//---------------------------------------------------------------------------
BOOL mqabcPlayerPlugin::OnSubCommand(MQDocument doc, int index)
{
    return FALSE;
}

//---------------------------------------------------------------------------
//  OnDraw
//    描画時の処理
//---------------------------------------------------------------------------
void mqabcPlayerPlugin::OnDraw(MQDocument doc, MQScene scene, int width, int height)
{
    mqabcRecorderWindow::each([doc](auto* w) {
        w->CaptureFrame(doc);
    });
}


//---------------------------------------------------------------------------
//  OnNewDocument
//    ドキュメント初期化時
//---------------------------------------------------------------------------
void mqabcPlayerPlugin::OnNewDocument(MQDocument doc, const char *filename, NEW_DOCUMENT_PARAM& param)
{
    m_mqo_path = filename ? filename : "";

    mqabcRecorderWindow::each([doc](auto* w) {
        w->MarkSceneDirty();
    });
}

//---------------------------------------------------------------------------
//  OnEndDocument
//    ドキュメント終了時
//---------------------------------------------------------------------------
void mqabcPlayerPlugin::OnEndDocument(MQDocument doc)
{
    m_mqo_path.clear();
    CloseAll();
}

//---------------------------------------------------------------------------
//  OnSaveDocument
//    ドキュメント保存時
//---------------------------------------------------------------------------
void mqabcPlayerPlugin::OnSaveDocument(MQDocument doc, const char *filename, SAVE_DOCUMENT_PARAM& param)
{
    m_mqo_path = filename ? filename : "";
}

//---------------------------------------------------------------------------
//  OnUndo
//    アンドゥ実行時
//---------------------------------------------------------------------------
BOOL mqabcPlayerPlugin::OnUndo(MQDocument doc, int undo_state)
{
    mqabcRecorderWindow::each([doc](auto* w) {
        w->MarkSceneDirty();
    });
    return TRUE;
}

//---------------------------------------------------------------------------
//  OnRedo
//    リドゥ実行時
//---------------------------------------------------------------------------
BOOL mqabcPlayerPlugin::OnRedo(MQDocument doc, int redo_state)
{
    mqabcRecorderWindow::each([doc](auto* w) {
        w->MarkSceneDirty();
    });
    return TRUE;
}

//---------------------------------------------------------------------------
//  OnUpdateUndo
//    アンドゥ状態更新時
//---------------------------------------------------------------------------
void mqabcPlayerPlugin::OnUpdateUndo(MQDocument doc, int undo_state, int undo_size)
{
    mqabcRecorderWindow::each([doc](auto* w) {
        w->MarkSceneDirty();
    });
}

//---------------------------------------------------------------------------
//  OnObjectModified
//    オブジェクトの編集時
//---------------------------------------------------------------------------
void mqabcPlayerPlugin::OnObjectModified(MQDocument doc)
{
}

//---------------------------------------------------------------------------
//  OnObjectSelected
//    オブジェクトの選択状態の変更時
//---------------------------------------------------------------------------
void mqabcPlayerPlugin::OnObjectSelected(MQDocument doc)
{
}

//---------------------------------------------------------------------------
//  OnUpdateObjectList
//    カレントオブジェクトの変更時
//---------------------------------------------------------------------------
void mqabcPlayerPlugin::OnUpdateObjectList(MQDocument doc)
{
}

//---------------------------------------------------------------------------
//  OnMaterialModified
//    マテリアルのパラメータ変更時
//---------------------------------------------------------------------------
void mqabcPlayerPlugin::OnMaterialModified(MQDocument doc)
{
}

//---------------------------------------------------------------------------
//  OnUpdateMaterialList
//    カレントマテリアルの変更時
//---------------------------------------------------------------------------
void mqabcPlayerPlugin::OnUpdateMaterialList(MQDocument doc)
{
}

//---------------------------------------------------------------------------
//  OnUpdateScene
//    シーン情報の変更時
//---------------------------------------------------------------------------
void mqabcPlayerPlugin::OnUpdateScene(MQDocument doc, MQScene scene)
{
}

//---------------------------------------------------------------------------
//  ExecuteCallback
//    コールバックに対する実装部
//---------------------------------------------------------------------------
bool mqabcPlayerPlugin::ExecuteCallback(MQDocument doc, void *option)
{
    CallbackInfo *info = (CallbackInfo*)option;
    return ((*this).*info->proc)(doc);
}

// コールバックの呼び出し
void mqabcPlayerPlugin::Execute(ExecuteCallbackProc proc)
{
    CallbackInfo info;
    info.proc = proc;
    BeginCallback(&info);
}

void mqabcPlayerPlugin::LogInfo(const char* message)
{
    if (m_player)
        m_player->LogInfo(message);
}

const std::string& mqabcPlayerPlugin::GetMQOPath() const
{
    return m_mqo_path;
}

void mqabcPlayerPlugin::CloseAll()
{
    if (m_player) {
        delete m_player;
        m_player = nullptr;
    }
    if (m_recorder) {
        delete m_recorder;
        m_recorder = nullptr;
    }
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
