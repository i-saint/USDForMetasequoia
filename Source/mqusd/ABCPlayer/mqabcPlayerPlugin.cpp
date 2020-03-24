#include "pch.h"
#include "mqusd.h"
#include "mqabcPlayerPlugin.h"
#include "mqabcPlayerWindow.h"
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
    *ID = mqabcPlayerPluginID;
}

//---------------------------------------------------------------------------
//  GetPlugInName
//    プラグイン名を返す。
//    この関数は[プラグインについて]表示時に呼び出される。
//---------------------------------------------------------------------------
const char *mqabcPlayerPlugin::GetPlugInName(void)
{
    return "Alembic Player (version " mqusdVersionString ") " mqusdCopyRight;
}

//---------------------------------------------------------------------------
//  EnumString
//    ポップアップメニューに表示される文字列を返す。
//    この関数は起動時に呼び出される。
//---------------------------------------------------------------------------
#if MQPLUGIN_VERSION >= 0x0470
const wchar_t *mqabcPlayerPlugin::EnumString(void)
{
    return L"Alembic Player";
}
#else
const char *mqabcPlayerPlugin::EnumString(void)
{
    return "Alembic Player";
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
    if (!m_window) {
        auto parent = MQWindow::GetMainWindow();
        m_window = new mqabcPlayerWindow(this, parent);
    }
    return TRUE;
}

//---------------------------------------------------------------------------
//  Exit
//    アプリケーションの終了
//---------------------------------------------------------------------------
void mqabcPlayerPlugin::Exit()
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
BOOL mqabcPlayerPlugin::Activate(MQDocument doc, BOOL flag)
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
BOOL mqabcPlayerPlugin::IsActivated(MQDocument doc)
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
}


//---------------------------------------------------------------------------
//  OnNewDocument
//    ドキュメント初期化時
//---------------------------------------------------------------------------
void mqabcPlayerPlugin::OnNewDocument(MQDocument doc, const char *filename, NEW_DOCUMENT_PARAM& param)
{
}

//---------------------------------------------------------------------------
//  OnEndDocument
//    ドキュメント終了時
//---------------------------------------------------------------------------
void mqabcPlayerPlugin::OnEndDocument(MQDocument doc)
{
    CloseABC();
}

//---------------------------------------------------------------------------
//  OnSaveDocument
//    ドキュメント保存時
//---------------------------------------------------------------------------
void mqabcPlayerPlugin::OnSaveDocument(MQDocument doc, const char *filename, SAVE_DOCUMENT_PARAM& param)
{
}

//---------------------------------------------------------------------------
//  OnUndo
//    アンドゥ実行時
//---------------------------------------------------------------------------
BOOL mqabcPlayerPlugin::OnUndo(MQDocument doc, int undo_state)
{
    return TRUE;
}

//---------------------------------------------------------------------------
//  OnRedo
//    リドゥ実行時
//---------------------------------------------------------------------------
BOOL mqabcPlayerPlugin::OnRedo(MQDocument doc, int redo_state)
{
    return TRUE;
}

//---------------------------------------------------------------------------
//  OnUpdateUndo
//    アンドゥ状態更新時
//---------------------------------------------------------------------------
void mqabcPlayerPlugin::OnUpdateUndo(MQDocument doc, int undo_state, int undo_size)
{
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

bool mqabcPlayerPlugin::OpenABC(MQDocument doc, const std::string& path)
{
    CloseABC();

    m_scene = CreateABCIScene();
    if (!m_scene)
        return false;

    if (!m_scene->open(path.c_str())) {
        mqusdLog(
            "failed to open %s\n"
            "it may not an usd file"
            , path.c_str());
        m_scene = {};
        return false;
    }

    m_importer.reset(new DocumentImporter(this, m_scene.get(), &m_options));
    m_importer->initialize(doc);

    return true;
}

bool mqabcPlayerPlugin::CloseABC()
{
    m_importer = {};
    m_scene = {};

    m_seek_time = 0;
    m_mqobj_id = 0;

    return true;
}

void mqabcPlayerPlugin::Seek(MQDocument doc, double t)
{
    if (!m_importer)
        return;

    m_seek_time = t;
    m_importer->read(doc, t);

    // repaint
    MQ_RefreshView(nullptr);
}

void mqabcPlayerPlugin::Refresh(MQDocument doc)
{
    Seek(doc, m_seek_time);
}

ImportOptions& mqabcPlayerPlugin::GetSettings()
{
    return m_options;
}

bool mqabcPlayerPlugin::IsArchiveOpened() const
{
    return m_scene != nullptr;
}

double mqabcPlayerPlugin::GetTimeStart() const
{
    return m_scene->time_start;
}
double mqabcPlayerPlugin::GetTimeEnd() const
{
    return m_scene->time_end;
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
