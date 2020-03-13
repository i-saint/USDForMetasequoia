#pragma once
#include "mqusdMesh.h"

class mqusdPlayerWindow;

struct mqusdPlayerSettings
{
    float scale_factor = 20.0f;
    bool flip_x = false;
    bool flip_yz = false;
    bool flip_faces = false;
    bool import_materials = true;
};

class mqusdPlayerPlugin : public MQStationPlugin
{
public:
    mqusdPlayerPlugin();
    virtual ~mqusdPlayerPlugin();

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


    typedef bool (mqusdPlayerPlugin::*ExecuteCallbackProc)(MQDocument doc);

    void Execute(ExecuteCallbackProc proc);

    struct CallbackInfo {
        ExecuteCallbackProc proc;
    };

    // コールバックに対する実装部
    bool ExecuteCallback(MQDocument doc, void *option) override;

    void LogInfo(const char* fmt, ...);


    bool OpenUSD(const std::string& v);
    bool CloseUSD();
    void ImportMaterials(MQDocument doc);
    void Seek(MQDocument doc, int64_t i);
    void Refresh(MQDocument doc);

    mqusdPlayerSettings& GetSettings();
    bool IsArchiveOpened() const;
    int64_t GetSampleCount() const;

private:
    class Node
    {
    public:
        enum class Type
        {
            Unknown,
            Top,
            Xform,
            PolyMesh,
            Material,
        };

        Node(Node *parent, UsdPrim abc);
        virtual ~Node();
        virtual Type getType() const;
        virtual void update(int64_t si);

        template<class NodeT> NodeT* findParent();

        Node* parent = nullptr;
        std::vector<Node*> children;
        UsdPrim abcobj;
        std::string name;
    };
    using NodePtr = std::shared_ptr<Node>;


    class TopNode : public Node
    {
    using super = Node;
    public:
        static const Type node_type = Type::Top;

        TopNode(UsdPrim abc);
        Type getType() const override;
    };


    class XformNode : public Node
    {
    using super = Node;
    public:
        static const Type node_type = Type::Xform;

        XformNode(Node* parent, UsdPrim abc);
        Type getType() const override;
        void update(int64_t si) override;

        UsdGeomXformable schema;
        XformNode* parent_xform = nullptr;
        float4x4 local_matrix = float4x4::identity();
        float4x4 global_matrix = float4x4::identity();
    };


    class MeshNode : public Node
    {
    using super = Node;
    public:
        static const Type node_type = Type::PolyMesh;

        MeshNode(Node* parent, UsdPrim abc);
        Type getType() const override;
        void update(int64_t si) override;
        void convert(const mqusdPlayerSettings& settings);

        UsdGeomMesh schema;
        mqusdMesh mesh;
        XformNode* parent_xform = nullptr;
        size_t sample_count = 0;

    private:
        void updateMeshData(int64_t si);
    };


    class MaterialNode : public Node
    {
    using super = Node;
    public:
        static const Type node_type = Type::Material;

        MaterialNode(Node* parent, UsdPrim abc);
        Type getType() const override;
        void update(int64_t si) override;
        bool valid() const;

        mqusdMaterial material;

    private:
        void updateMeshData(int64_t si);
    };

    void ConstructTree(Node *n);
    bool DoSeek(MQDocument doc);

private:
    mqusdPlayerWindow* m_window = nullptr;
    mqusdPlayerSettings m_settings;

    std::string m_abc_path;
    std::shared_ptr<std::fstream> m_stream;
    UsdStageRefPtr m_stage;
    int64_t m_sample_count = 0;
    int64_t m_sample_index = 0;

    std::vector<NodePtr> m_nodes;
    TopNode* m_top_node = nullptr;
    std::vector<MeshNode*> m_mesh_nodes;
    std::vector<MaterialNode*> m_material_nodes;

    mqusdMesh m_mesh_merged;
    int m_mqobj_id = 0;
};
