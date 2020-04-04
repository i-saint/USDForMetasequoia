#include "pch.h"
#include "SceneGraphRemote.h"
#include "sgUtils.h"

namespace sg {

#define mqusdTBBDll muDLLPrefix "tbb" muDLLSuffix
#define mqusdUSDDll muDLLPrefix "usd_ms" muDLLSuffix
#define mqusdCoreDll muDLLPrefix "SceneGraphUSD" muDLLSuffix
#define mqusdCoreExe "SceneGraphUSD" muEXESuffix

static std::string g_usd_module_dir;
static std::string g_usd_plugin_path;
static void* g_core_module;
static SceneInterface* (*g_mqusdCreateUSDSceneInterface)(Scene* scene);

static std::string GetDefaultModulePath()
{
    std::string dir = mu::GetCurrentModuleDirectory();
#ifdef _WIN32
    return dir + "/mqusdCore";
#else
    return dir;
#endif
}

void SetUSDModuleDir(const std::string& v) { g_usd_module_dir = v; }
void SetUSDPluginPath(const std::string& v) { g_usd_plugin_path = v; }

static void LoadCoreModule()
{
    static std::once_flag s_flag;
    std::call_once(s_flag, []() {
        g_core_module = mu::GetModule(mqusdCoreDll);
        if (!g_core_module) {
            std::string core_dir = g_usd_module_dir.empty() ? GetDefaultModulePath() : g_usd_module_dir;
            std::string plugin_path = g_usd_plugin_path.empty() ? core_dir + "/usd/plugInfo.json" : g_usd_plugin_path;
            mu::SetEnv("PXR_PLUGINPATH_NAME", plugin_path.c_str());

            std::string tbb_dll = core_dir + "/" mqusdTBBDll;
            std::string usd_dll = core_dir + "/" mqusdUSDDll;
            std::string core_dll = core_dir + "/" mqusdCoreDll;
            mu::LoadModule(tbb_dll.c_str());
            mu::LoadModule(usd_dll.c_str());
            g_core_module = mu::LoadModule(core_dll.c_str());
        }
        if (g_core_module) {
            (void*&)g_mqusdCreateUSDSceneInterface = mu::GetSymbol(g_core_module, "mqusdCreateUSDSceneInterface");
        }
    });
}

ScenePtr CreateUSDScene()
{
#if defined(_WIN32) && defined(_M_IX86)
    return CreateUSDScenePipe();
#else
    LoadCoreModule();
    if (g_mqusdCreateUSDSceneInterface) {
        auto ret = new Scene();
        ret->impl.reset(g_mqusdCreateUSDSceneInterface(ret));
        return ScenePtr(ret);
    }
    else
        return nullptr;
#endif
}



class USDScenePipe : public SceneInterface
{
public:
    USDScenePipe(Scene* scene);
    ~USDScenePipe() override;
    bool open(const char* path) override;
    bool create(const char* path) override;
    bool save() override;
    void close() override;
    void read() override;
    void write() override;

    Node* createNode(Node* parent, const char* name, Node::Type type) override;
    bool wrapNode(Node* node) override;

private:
    Scene* m_scene = nullptr;
    std::string m_exe_path;
    std::string m_usd_path;
    std::shared_ptr<mu::PipeStream> m_pipe;
    RawVector<char> m_scene_buffer;
};


USDScenePipe::USDScenePipe(Scene* scene)
    : m_scene(scene)
{
    m_exe_path = g_usd_module_dir.empty() ? GetDefaultModulePath() : g_usd_module_dir;
    m_exe_path += '/';
    m_exe_path += mqusdCoreExe;
}

USDScenePipe::~USDScenePipe()
{
}

static void CopyStream(RawVector<char>& dst, std::iostream& src)
{
    const int len = 1024 * 1024;
    if (dst.size() < len)
        dst.resize_discard(len);

    size_t pos = 0;
    size_t space = dst.size();
    for (;;) {
        src.read(dst.data() + pos, space);
        if ((size_t)src.gcount() != space) {
            dst.resize(pos + (size_t)src.gcount());
            break;
        }
        else {
            pos = dst.size();
            dst.resize(dst.size() * 2);
            space = dst.size() - pos;
        }
    }
}

bool USDScenePipe::open(const char* path)
{
    close();

    m_usd_path = path;
    m_pipe.reset(new mu::PipeStream());
    std::string commnd = m_exe_path;
    commnd += " -hide -tree \"";
    commnd += m_usd_path;
    commnd += "\"";
    if (m_pipe->open(commnd.c_str(), std::ios::in | std::ios::binary)) {
        CopyStream(m_scene_buffer, *m_pipe);
        m_pipe.reset();

        mu::MemoryStream stream(m_scene_buffer);
        m_scene->deserialize(stream);
        return !m_scene->nodes.empty();
    }
    else {
        m_pipe.reset();
        return false;
    }
}

bool USDScenePipe::create(const char* path)
{
    close();

    m_usd_path = path;
    m_pipe.reset(new mu::PipeStream());
    std::string commnd = m_exe_path;
    commnd += " -hide -export \"";
    commnd += path;
    commnd += "\"";
    if (!m_pipe->open(commnd.c_str(), std::ios::out | std::ios::binary)) {
        return false;
    }
    return true;
}

bool USDScenePipe::save()
{
    // nothing to do here
    return true;
}

void USDScenePipe::close()
{
    m_pipe.reset();
}

void USDScenePipe::read()
{
    m_pipe.reset(new mu::PipeStream());
    std::string commnd = m_exe_path;

    if (!std::isnan(m_scene->time_current)) {
        char buf[128];
        sprintf(buf, " -time %lf", m_scene->time_current);
        commnd += buf;
    }
    commnd += " -hide \"";
    commnd += m_usd_path;
    commnd += "\"";
    if (m_pipe->open(commnd.c_str(), std::ios::in | std::ios::binary)) {
        CopyStream(m_scene_buffer, *m_pipe);
        mu::MemoryStream stream(m_scene_buffer);
        m_scene->deserialize(stream);
    }
    m_pipe.reset();
}

void USDScenePipe::write()
{
    m_scene->serialize(*m_pipe);
}

Node* USDScenePipe::createNode(Node* parent, const char* name, Node::Type type)
{
    return m_scene->createNodeImpl(parent, name, type);
}

bool USDScenePipe::wrapNode(Node* /*node*/)
{
    // nothing to do here
    return true;
}


ScenePtr CreateUSDScenePipe()
{
    auto ret = new Scene();
    ret->impl.reset(new USDScenePipe(ret));
    return ScenePtr(ret);
}

} // namespace sg
