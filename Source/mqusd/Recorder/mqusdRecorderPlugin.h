﻿#pragma once
#include "Foundation/mqusdDocumentExporter.h"

namespace mqusd {

class mqusdRecorderWindow;

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

    void LogInfo(const char* message);


public:
    bool OpenUSD(const std::string& v);
    bool CloseUSD();
    void CaptureFrame(MQDocument doc);

    const std::string& GetMQOPath() const;
    const std::string& GetUSDPath() const;
    bool IsArchiveOpened() const;
    bool IsRecording() const;
    void SetRecording(bool v);
    ExportOptions& GetOptions();

private:
    void MarkSceneDirty();

private:
    mqusdRecorderWindow* m_window = nullptr;
    ScenePtr m_scene;
    ExportOptions m_options;
    DocumentExporterPtr m_exporter;

    bool m_dirty = false;
    bool m_recording = false;
    std::string m_mqo_path;
};

} // namespace mqusd
