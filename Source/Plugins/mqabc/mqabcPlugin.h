#pragma once

namespace mqusd {

class mqabcWindow;

class mqabcPlugin : public MQStationPlugin
{
public:
    mqabcPlugin();
    virtual ~mqabcPlugin();

#if defined(__APPLE__) || defined(__linux__)
    MQBasePlugin *CreateNewPlugin() override;
#endif
    void GetPlugInID(DWORD *Product, DWORD *ID) override;
    const char *GetPlugInName(void) override;
#if MQPLUGIN_VERSION >= 0x0470
    const wchar_t *EnumString(void) override;
#else
    const char *EnumString(void) override;
#endif
    const char *EnumSubCommand(int index) override;
    const wchar_t *GetSubCommandString(int index) override;

    BOOL Initialize() override;
    void Exit() override;

    BOOL Activate(MQDocument doc, BOOL flag) override;
    BOOL IsActivated(MQDocument doc) override;
    void OnMinimize(MQDocument doc, BOOL flag) override;
    BOOL OnSubCommand(MQDocument doc, int index) override;
    void OnDraw(MQDocument doc, MQScene scene, int width, int height) override;

    void OnNewDocument(MQDocument doc, const char *filename, NEW_DOCUMENT_PARAM& param) override;
    void OnEndDocument(MQDocument doc) override;
    void OnSaveDocument(MQDocument doc, const char *filename, SAVE_DOCUMENT_PARAM& param) override;
    BOOL OnUndo(MQDocument doc, int undo_state) override;
    BOOL OnRedo(MQDocument doc, int redo_state) override;
    void OnUpdateUndo(MQDocument doc, int undo_state, int undo_size) override;

    bool ExecuteCallback(MQDocument doc, void* option) override;



    void LogInfo(const char* message);

    const std::string& GetMQOPath() const;

private:
    mqabcWindow* m_window = nullptr;
    std::string m_mqo_path;
};

} // namespace mqusd
