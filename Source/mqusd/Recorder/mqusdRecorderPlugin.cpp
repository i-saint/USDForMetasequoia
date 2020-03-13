#include "pch.h"
#include "mqusd.h"
#include "mqusdRecorderPlugin.h"
#include "mqusdRecorderWindow.h"

static mqusdRecorderPlugin g_plugin;

// Constructor
// コンストラクタ
mqusdRecorderPlugin::mqusdRecorderPlugin()
{
}

// Destructor
// デストラクタ
mqusdRecorderPlugin::~mqusdRecorderPlugin()
{
}

#if defined(__APPLE__) || defined(__linux__)
// Create a new plugin class for another document.
// 別のドキュメントのための新しいプラグインクラスを作成する。
MQBasePlugin* mqusdRecorderPlugin::CreateNewPlugin()
{
    return new mqusdRecorderPlugin();
}
#endif

//---------------------------------------------------------------------------
//  GetPlugInID
//    プラグインIDを返す。
//    この関数は起動時に呼び出される。
//---------------------------------------------------------------------------
void mqusdRecorderPlugin::GetPlugInID(DWORD *Product, DWORD *ID)
{
    // プロダクト名(制作者名)とIDを、全部で64bitの値として返す
    // 値は他と重複しないようなランダムなもので良い
    *Product = mqusdPluginProduct;
    *ID = mqusdRecorderPluginID;
}

//---------------------------------------------------------------------------
//  GetPlugInName
//    プラグイン名を返す。
//    この関数は[プラグインについて]表示時に呼び出される。
//---------------------------------------------------------------------------
const char *mqusdRecorderPlugin::GetPlugInName(void)
{
    return "Alembic Recorder (version " mqusdVersionString ") " mqusdCopyRight;
}

//---------------------------------------------------------------------------
//  EnumString
//    ポップアップメニューに表示される文字列を返す。
//    この関数は起動時に呼び出される。
//---------------------------------------------------------------------------
#if MQPLUGIN_VERSION >= 0x0470
const wchar_t *mqusdRecorderPlugin::EnumString(void)
{
    return L"Alembic Recorder";
}
#else
const char *mqusdRecorderPlugin::EnumString(void)
{
    return "Alembic Recorder";
}
#endif



//---------------------------------------------------------------------------
//  EnumSubCommand
//    サブコマンド前を列挙
//---------------------------------------------------------------------------
const char *mqusdRecorderPlugin::EnumSubCommand(int index)
{
    return NULL;
}

//---------------------------------------------------------------------------
//  GetSubCommandString
//    サブコマンドの文字列を列挙
//---------------------------------------------------------------------------
const wchar_t *mqusdRecorderPlugin::GetSubCommandString(int index)
{
    return NULL;
}

//---------------------------------------------------------------------------
//  Initialize
//    アプリケーションの初期化
//---------------------------------------------------------------------------
BOOL mqusdRecorderPlugin::Initialize()
{
    if (!m_window) {
        auto parent = MQWindow::GetMainWindow();
        m_window = new mqusdRecorderWindow(this, parent);
    }
    return TRUE;
}

//---------------------------------------------------------------------------
//  Exit
//    アプリケーションの終了
//---------------------------------------------------------------------------
void mqusdRecorderPlugin::Exit()
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
BOOL mqusdRecorderPlugin::Activate(MQDocument doc, BOOL flag)
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
BOOL mqusdRecorderPlugin::IsActivated(MQDocument doc)
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
void mqusdRecorderPlugin::OnMinimize(MQDocument doc, BOOL flag)
{
}

//---------------------------------------------------------------------------
//  OnReceiveUserMessage
//    プラグイン独自のメッセージの受け取り
//---------------------------------------------------------------------------
int mqusdRecorderPlugin::OnReceiveUserMessage(MQDocument doc, DWORD src_product, DWORD src_id, const char *description, void *message)
{
    return 0;
}

//---------------------------------------------------------------------------
//  OnSubCommand
//    A message for calling a sub comand
//    サブコマンドの呼び出し
//---------------------------------------------------------------------------
BOOL mqusdRecorderPlugin::OnSubCommand(MQDocument doc, int index)
{
    return FALSE;
}

//---------------------------------------------------------------------------
//  OnDraw
//    描画時の処理
//---------------------------------------------------------------------------
void mqusdRecorderPlugin::OnDraw(MQDocument doc, MQScene scene, int width, int height)
{
    CaptureFrame(doc);
}


//---------------------------------------------------------------------------
//  OnNewDocument
//    ドキュメント初期化時
//---------------------------------------------------------------------------
void mqusdRecorderPlugin::OnNewDocument(MQDocument doc, const char *filename, NEW_DOCUMENT_PARAM& param)
{
    m_mqo_path = filename ? filename : "";
    MarkSceneDirty();
}

//---------------------------------------------------------------------------
//  OnEndDocument
//    ドキュメント終了時
//---------------------------------------------------------------------------
void mqusdRecorderPlugin::OnEndDocument(MQDocument doc)
{
    m_mqo_path.clear();
    CloseABC();
}

//---------------------------------------------------------------------------
//  OnSaveDocument
//    ドキュメント保存時
//---------------------------------------------------------------------------
void mqusdRecorderPlugin::OnSaveDocument(MQDocument doc, const char *filename, SAVE_DOCUMENT_PARAM& param)
{
    m_mqo_path = filename ? filename : "";
}

//---------------------------------------------------------------------------
//  OnUndo
//    アンドゥ実行時
//---------------------------------------------------------------------------
BOOL mqusdRecorderPlugin::OnUndo(MQDocument doc, int undo_state)
{
    MarkSceneDirty();
    return TRUE;
}

//---------------------------------------------------------------------------
//  OnRedo
//    リドゥ実行時
//---------------------------------------------------------------------------
BOOL mqusdRecorderPlugin::OnRedo(MQDocument doc, int redo_state)
{
    MarkSceneDirty();
    return TRUE;
}

//---------------------------------------------------------------------------
//  OnUpdateUndo
//    アンドゥ状態更新時
//---------------------------------------------------------------------------
void mqusdRecorderPlugin::OnUpdateUndo(MQDocument doc, int undo_state, int undo_size)
{
    MarkSceneDirty();
}

//---------------------------------------------------------------------------
//  OnObjectModified
//    オブジェクトの編集時
//---------------------------------------------------------------------------
void mqusdRecorderPlugin::OnObjectModified(MQDocument doc)
{
}

//---------------------------------------------------------------------------
//  OnObjectSelected
//    オブジェクトの選択状態の変更時
//---------------------------------------------------------------------------
void mqusdRecorderPlugin::OnObjectSelected(MQDocument doc)
{
}

//---------------------------------------------------------------------------
//  OnUpdateObjectList
//    カレントオブジェクトの変更時
//---------------------------------------------------------------------------
void mqusdRecorderPlugin::OnUpdateObjectList(MQDocument doc)
{
}

//---------------------------------------------------------------------------
//  OnMaterialModified
//    マテリアルのパラメータ変更時
//---------------------------------------------------------------------------
void mqusdRecorderPlugin::OnMaterialModified(MQDocument doc)
{
}

//---------------------------------------------------------------------------
//  OnUpdateMaterialList
//    カレントマテリアルの変更時
//---------------------------------------------------------------------------
void mqusdRecorderPlugin::OnUpdateMaterialList(MQDocument doc)
{
}

//---------------------------------------------------------------------------
//  OnUpdateScene
//    シーン情報の変更時
//---------------------------------------------------------------------------
void mqusdRecorderPlugin::OnUpdateScene(MQDocument doc, MQScene scene)
{
}

//---------------------------------------------------------------------------
//  ExecuteCallback
//    コールバックに対する実装部
//---------------------------------------------------------------------------
bool mqusdRecorderPlugin::ExecuteCallback(MQDocument doc, void *option)
{
    CallbackInfo *info = (CallbackInfo*)option;
    return ((*this).*info->proc)(doc);
}

// コールバックの呼び出し
void mqusdRecorderPlugin::Execute(ExecuteCallbackProc proc)
{
    CallbackInfo info;
    info.proc = proc;
    BeginCallback(&info);
}

void mqusdRecorderPlugin::LogInfo(const char* fmt, ...)
{
    if (m_window) {
        char buf[1024 * 2];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

        m_window->LogInfo(buf);
    }
}

MQBasePlugin* GetPluginClass()
{
    return &g_plugin;
}

void msmqLogInfo(const char *message)
{
    g_plugin.LogInfo(message);
}

#ifdef mqusdDebug
void mqusdRecorderPlugin::DbgDoSomething()
{
    Execute(&mqusdRecorderPlugin::DbgDoSomethingImpl);
}

bool mqusdRecorderPlugin::DbgDoSomethingImpl(MQDocument doc)
{
    return false;
}
#endif // mqusdDebug


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
