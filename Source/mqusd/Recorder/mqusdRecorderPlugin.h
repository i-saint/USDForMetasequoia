#pragma once
#include "mqusdSceneGraph.h"

class mqusdRecorderWindow;

struct mqusdRecorderSettings
{
    bool capture_uvs = true;
    bool capture_normals = true;
    bool capture_colors = false;
    bool capture_material_ids = true;
    bool capture_materials = true;
    bool freeze_mirror = true;
    bool freeze_lathe = true;
    bool freeze_subdiv = false;
    bool keep_time = false;
    float scale_factor = 0.05f;
    float frame_rate = 30.0f; // relevant only when keep_time is false
    float time_scale = 1.0f; // relevant only when keep_time is true
};

class mqusdRecorderPlugin : public MQStationPlugin
{
public:
    mqusdRecorderPlugin();
    virtual ~mqusdRecorderPlugin();

#if defined(__APPLE__) || defined(__linux__)
    // Create a new plugin class for another document.
    // 別のドキュメントのための新しいプラグインクラスを作成する。
    MQBasePlugin *CreateNewPlugin() override;
#endif
    // プラグインIDを返す。
    void GetPlugInID(DWORD *Product, DWORD *ID) override;
    // プラグイン名を返す。
    const char *GetPlugInName(void) override;
    // ポップアップメニューに表示される文字列を返す。
#if MQPLUGIN_VERSION >= 0x0470
    const wchar_t *EnumString(void) override;
#else
    const char *EnumString(void) override;
#endif
    // サブコマンド前を列挙
    const char *EnumSubCommand(int index) override;
    // サブコマンドの文字列を列挙
    const wchar_t *GetSubCommandString(int index) override;

    // アプリケーションの初期化
    BOOL Initialize() override;
    // アプリケーションの終了
    void Exit() override;

    // 表示・非表示切り替え要求
    BOOL Activate(MQDocument doc, BOOL flag) override;
    // 表示・非表示状態の返答
    BOOL IsActivated(MQDocument doc) override;
    // ウインドウの最小化への返答
    void OnMinimize(MQDocument doc, BOOL flag) override;
    // プラグイン独自のメッセージの受け取り
    int OnReceiveUserMessage(MQDocument doc, DWORD src_product, DWORD src_id, const char *description, void *message) override;
    // サブコマンドの呼び出し
    BOOL OnSubCommand(MQDocument doc, int index) override;
    // 描画時の処理
    void OnDraw(MQDocument doc, MQScene scene, int width, int height) override;

    // ドキュメント初期化時
    void OnNewDocument(MQDocument doc, const char *filename, NEW_DOCUMENT_PARAM& param) override;
    // ドキュメント終了時
    void OnEndDocument(MQDocument doc) override;
    // ドキュメント保存時
    void OnSaveDocument(MQDocument doc, const char *filename, SAVE_DOCUMENT_PARAM& param) override;
    // アンドゥ実行時
    BOOL OnUndo(MQDocument doc, int undo_state) override;
    // リドゥ実行時
    BOOL OnRedo(MQDocument doc, int redo_state) override;
    // アンドゥ状態更新時
    void OnUpdateUndo(MQDocument doc, int undo_state, int undo_size) override;
    // オブジェクトの編集時
    void OnObjectModified(MQDocument doc) override;
    // オブジェクトの選択状態の変更時
    void OnObjectSelected(MQDocument doc) override;
    // カレントオブジェクトの変更時
    void OnUpdateObjectList(MQDocument doc) override;
    // マテリアルのパラメータ変更時
    void OnMaterialModified(MQDocument doc) override;
    // カレントマテリアルの変更時
    void OnUpdateMaterialList(MQDocument doc) override;
    // シーン情報の変更時
    void OnUpdateScene(MQDocument doc, MQScene scene) override;


    typedef bool (mqusdRecorderPlugin::*ExecuteCallbackProc)(MQDocument doc);

    void Execute(ExecuteCallbackProc proc);

    struct CallbackInfo {
        ExecuteCallbackProc proc;
    };

    // コールバックに対する実装部
    bool ExecuteCallback(MQDocument doc, void *option) override;

    void LogInfo(const char* fmt, ...);


    bool OpenUSD(const std::string& v);
    bool CloseUSD();

    const std::string& GetMQOPath() const;
    const std::string& GetUSDPath() const;
    bool IsArchiveOpened() const;
    bool IsRecording() const;
    void SetRecording(bool v);
    void SetInterval(double v);
    double GetInterval() const;
    mqusdRecorderSettings& GetSettings();

    void MarkSceneDirty();
    void CaptureFrame(MQDocument doc);

#ifdef mqusdDebug
    void DbgDoSomething();
    bool DbgDoSomethingImpl(MQDocument doc);
#endif // mqusdDebug

private:
    struct ObjectRecord
    {
        MQDocument mqdocument;
        MQObject mqobject;
        bool need_release = false;

        mqusdMesh mesh;
    };

    struct MaterialRecord
    {
        MQDocument mqdocument;
        MQMaterial mqmaterial;

        mqusdMaterial material;
    };

    void ExtractMeshData(ObjectRecord& rec);
    void FlushUSD();
    void WaitFlush();
    void WriteMaterials();

private:
    mqusdRecorderWindow* m_window = nullptr;
    mqusdRecorderSettings m_settings;

    bool m_dirty = false;
    bool m_recording = false;

    std::string m_mqo_path;
    mu::nanosec m_start_time = 0;
    mu::nanosec m_last_flush = 0;
    mu::nanosec m_interval = 5000000000; // 5 sec
    int m_frame = 0;
    double m_time = 0.0;

    ScenePtr m_scene;
    RootNode *m_root_node = nullptr;
    MeshNode *m_mesh_node = nullptr;

    std::vector<ObjectRecord> m_obj_records;
    std::vector<MaterialRecord> m_material_records;
    std::future<void> m_task_write;
};
