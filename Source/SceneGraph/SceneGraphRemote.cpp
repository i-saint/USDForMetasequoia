#include "pch.h"
#include "SceneGraphRemote.h"
#include "sgUtils.h"

namespace sg {

#define sgusdTBBDll muDLLPrefix "tbb" muDLLSuffix
#define sgusdUsdDll muDLLPrefix "usd_ms" muDLLSuffix
#define sgusdCoreDll muDLLPrefix "SceneGraphUSD" muDLLSuffix
#define sgusdCoreExe "SceneGraphUSD" muEXESuffix

static void* g_core_module;
static SceneInterface* (*g_sgusdCreateSceneInterface)(Scene* scene);

static std::string& USDModuleDir()
{
    static std::string s_dir;
    return s_dir;
}
void SetUSDModuleDir(const std::string& v)
{
    USDModuleDir() = v;
}
static std::string GetUSDModuleDir()
{
    return USDModuleDir().empty() ? mu::GetCurrentModuleDirectory() : USDModuleDir();
}

bool LoadUSDModule()
{
    static std::once_flag s_flag;
    std::call_once(s_flag, []() {
        g_core_module = mu::GetModule(sgusdCoreDll);
        if (!g_core_module) {
            std::string module_dir = GetUSDModuleDir();
            std::string tbb_dll = module_dir + muPathSep sgusdTBBDll;
            std::string usd_dll = module_dir + muPathSep sgusdUsdDll;
            std::string core_dll = module_dir + muPathSep sgusdCoreDll;
            mu::LoadModule(tbb_dll.c_str());
            mu::LoadModule(usd_dll.c_str());
            g_core_module = mu::LoadModule(core_dll.c_str());
        }
        if (g_core_module) {
            (void*&)g_sgusdCreateSceneInterface = mu::GetSymbol(g_core_module, "sgusdCreateSceneInterface");
        }
    });
    return g_core_module != nullptr;
}

ScenePtr CreateUSDScene()
{
#if defined(_WIN32) && defined(_M_IX86)
    return CreateUSDScenePipe();
#else
    LoadUSDModule();
    if (g_sgusdCreateSceneInterface) {
        auto ret = new Scene();
        ret->impl.reset(g_sgusdCreateSceneInterface(ret));
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

    bool isNodeTypeSupported(Node::Type type) override;
    Node* createNode(Node* parent, const char* name, Node::Type type) override;
    bool wrapNode(Node* node) override;
    double frameToTime(int frame) override;

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
    m_exe_path = GetUSDModuleDir();
    m_exe_path += muPathSep sgusdCoreExe;
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
        sg::deserializer des(stream);
        m_scene->deserialize(des);
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
    commnd += " -hide";
    commnd += " \"";
    commnd += m_usd_path;
    commnd += "\"";
    if (m_pipe->open(commnd.c_str(), std::ios::in | std::ios::binary)) {
        CopyStream(m_scene_buffer, *m_pipe);
        mu::MemoryStream stream(m_scene_buffer);
        sg::deserializer des(stream);
        m_scene->deserialize(des);
    }
    m_pipe.reset();
}

void USDScenePipe::write()
{
    sg::serializer ser(*m_pipe);
    m_scene->serialize(ser);
}

bool USDScenePipe::isNodeTypeSupported(Node::Type /*type*/)
{
    return true;
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

double USDScenePipe::frameToTime(int frame)
{
    return (1.0 / m_scene->frame_rate) * frame + m_scene->time_start;
}


ScenePtr CreateUSDScenePipe()
{
    auto ret = new Scene();
    ret->impl.reset(new USDScenePipe(ret));
    return ScenePtr(ret);
}

} // namespace sg
