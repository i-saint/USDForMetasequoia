#include "pch.h"
#include "muMisc.h"
#ifdef _WIN32
    #include <dbghelp.h>
    #include <psapi.h>
    #pragma comment(lib, "dbghelp.lib")
#else
    #include <unistd.h>
    #include <sys/mman.h>
    #include <dlfcn.h>
    #ifdef __APPLE__
        #include <mach-o/dyld.h>
    #else
        #include <link.h>
    #endif
#endif

namespace mu {

nanosec Now()
{
    using namespace std::chrono;
    return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

static std::function<void(const char*)> g_print_handler;
static std::function<void(const wchar_t*)> g_wprint_handler;
static const int g_print_buf_size = 1024 * 4;
static thread_local char* g_print_buf;
static thread_local wchar_t* g_wprint_buf;

static char* GetPrintBuffer()
{
    if (!g_print_buf)
        g_print_buf = new char[g_print_buf_size];
    return g_print_buf;
}

static wchar_t* GetWPrintBuffer()
{
    if (!g_wprint_buf)
        g_wprint_buf = new wchar_t[g_print_buf_size];
    return g_wprint_buf;
}

void SetPrintHandler(const PrintHandler& v)
{
    g_print_handler = v;
}

void Print(const char *fmt, ...)
{
    char* buf = GetPrintBuffer();

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, g_print_buf_size, fmt, args);
    va_end(args);

    if (g_print_handler) {
        g_print_handler(buf);
    }
    else {
#ifdef _WIN32
        ::OutputDebugStringA(buf);
#else
        printf(buf);
#endif
    }
}

void Print(const wchar_t *fmt, ...)
{
    wchar_t* buf = GetWPrintBuffer();

    va_list args;
    va_start(args, fmt);
    vswprintf(buf, g_print_buf_size, fmt, args);
    va_end(args);

    if (g_wprint_handler) {
        g_wprint_handler(buf);
    }
    else {
#ifdef _WIN32
        ::OutputDebugStringW(buf);
#else
        wprintf(buf);
#endif
    }
}

std::string Format(const char *fmt, ...)
{
    char* buf = GetPrintBuffer();

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, g_print_buf_size, fmt, args);
    va_end(args);

    return buf;
}

std::string ToUTF8(const char *src)
{
#ifdef _WIN32
    // to UTF-16
    int wsize = ::MultiByteToWideChar(CP_ACP, 0, (LPCSTR)src, -1, nullptr, 0);
    if (wsize > 0)
        --wsize; // remove last '\0'
    std::wstring ws;
    ws.resize(wsize);
    ::MultiByteToWideChar(CP_ACP, 0, (LPCSTR)src, -1, (LPWSTR)ws.data(), wsize);

    // to UTF-8
    int u8size = ::WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)ws.data(), -1, nullptr, 0, nullptr, nullptr);
    if (u8size > 0)
        --u8size; // remove last '\0'
    std::string u8s;
    u8s.resize(u8size);
    ::WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)ws.data(), -1, (LPSTR)u8s.data(), u8size, nullptr, nullptr);
    return u8s;
#else
    return src;
#endif
}
std::string ToUTF8(const wchar_t* src)
{
#ifdef _WIN32
    // to UTF-8
    int u8size = ::WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)src, -1, nullptr, 0, nullptr, nullptr);
    if (u8size > 0)
        --u8size; // remove last '\0'
    std::string u8s;
    u8s.resize(u8size);
    ::WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)src, -1, (LPSTR)u8s.data(), u8size, nullptr, nullptr);
    return u8s;
#else
    std::string ret;
    ret.resize(std::wcslen(src) * 8);
    ret.resize(std::wcstombs(&ret[0], src, ret.size()));
    return ret;
#endif
}
std::string ToUTF8(const std::string& src)
{
    return ToUTF8(src.c_str());
}
std::string ToUTF8(const std::wstring& src)
{
    return ToUTF8(src.c_str());
}

std::string ToANSI(const char *src)
{
#ifdef _WIN32
    // to UTF-16
    int wsize = ::MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)src, -1, nullptr, 0);
    if (wsize > 0)
        --wsize; // remove last '\0'
    std::wstring ws;
    ws.resize(wsize);
    ::MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)src, -1, (LPWSTR)ws.data(), wsize);

    // to ANSI
    int u8size = ::WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)ws.data(), -1, nullptr, 0, nullptr, nullptr);
    if (u8size > 0)
        --u8size; // remove last '\0'
    std::string u8s;
    u8s.resize(u8size);
    ::WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)ws.data(), -1, (LPSTR)u8s.data(), u8size, nullptr, nullptr);
    return u8s;
#else
    return src;
#endif
}

std::string ToANSI(const wchar_t* src)
{
#ifdef _WIN32
    // to ANSI
    int u8size = ::WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)src, -1, nullptr, 0, nullptr, nullptr);
    if (u8size > 0)
        --u8size; // remove last '\0'
    std::string u8s;
    u8s.resize(u8size);
    ::WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)src, -1, (LPSTR)u8s.data(), u8size, nullptr, nullptr);
    return u8s;
#else
    std::string ret;
    ret.resize(std::wcslen(src) * 8);
    ret.resize(std::wcstombs(&ret[0], src, ret.size()));
    return ret;
#endif
}

std::string ToANSI(const std::string& src)
{
    return ToANSI(src.c_str());
}
std::string ToANSI(const std::wstring& src)
{
    return ToANSI(src.c_str());
}

std::string ToMBS(const wchar_t *src)
{
    return ToUTF8(src);
}
std::string ToMBS(const std::wstring& src)
{
    return ToUTF8(src);
}

std::wstring ToWCS(const char* src)
{
#ifdef _WIN32
    // to UTF-16
    int wsize = ::MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)src, -1, nullptr, 0);
    if (wsize > 0)
        --wsize; // remove last '\0'
    std::wstring ws;
    ws.resize(wsize);
    ::MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)src, -1, (LPWSTR)ws.data(), wsize);
    return ws;
#else
    std::wstring ret;
    ret.resize(std::strlen(src) * 8);
    ret.resize(std::mbstowcs(&ret[0], src, ret.size()));
    return ret;
#endif
}
std::wstring ToWCS(const std::string& src)
{
    return ToWCS(src.c_str());
}

std::string SanitizeFileName(const std::string& src)
{
    std::string ret = src;
    for (auto& c : ret) {
        if (c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
            c = '_';
    }
    return ret;
}

std::string GetDirectory(const char* src)
{
    int last_separator = 0;
    for (int i = 0; src[i] != L'\0'; ++i) {
        if (src[i] == L'\\' || src[i] == L'/')
            last_separator = i;
    }
    return std::string(src, last_separator);
}
std::string GetDirectory(const wchar_t* src)
{
    int last_separator = 0;
    for (int i = 0; src[i] != L'\0'; ++i) {
        if (src[i] == L'\\' || src[i] == L'/')
            last_separator = i;
    }
    return ToMBS(std::wstring(src, last_separator));
}

std::string GetFilename(const char *src)
{
    int last_separator = 0;
    for (int i = 0; src[i] != '\0'; ++i) {
        if (src[i] == '\\' || src[i] == '/')
            last_separator = i + 1;
    }
    return std::string(src + last_separator);
}

std::string GetFilename(const wchar_t *src)
{
    int last_separator = 0;
    for (int i = 0; src[i] != L'\0'; ++i) {
        if (src[i] == L'\\' || src[i] == L'/')
            last_separator = i + 1;
    }
    return ToMBS(src + last_separator);
}

std::string GetFilename_NoExtension(const char *src)
{
    int last_separator = 0;
    int last_comma = 0;
    for (int i = 0; src[i] != '\0'; ++i) {
        if (src[i] == '\\' || src[i] == '/')
            last_separator = i + 1;
        if (src[i] == '.')
            last_comma = i;
    }

    if (last_comma > last_separator)
        return std::string(src + last_separator, src + last_comma);
    else
        return std::string(src + last_separator);
}

std::string GetFilename_NoExtension(const wchar_t *src)
{
    int last_separator = 0;
    int last_comma = 0;
    for (int i = 0; src[i] != '\0'; ++i) {
        if (src[i] == '\\' || src[i] == '/')
            last_separator = i + 1;
        if (src[i] == '.')
            last_comma = i;
    }

    if (last_comma > last_separator)
        return ToMBS(std::wstring(src + last_separator, src + last_comma));
    else
        return ToMBS(src + last_separator);
}


std::string GetCurrentModuleDirectory()
{
    static std::string s_result;
    if (s_result.empty()) {
#ifdef _WIN32
        HMODULE mod = 0;
        char buf[1024];
        ::GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)&GetCurrentModuleDirectory, &mod);
        ::GetModuleFileNameA(mod, buf, sizeof(buf));
        s_result = GetDirectory(buf);
#else
        Dl_info info;
        dladdr((void*)&GetCurrentModuleDirectory, &info);
        s_result = GetDirectory(info.dli_fname);
#endif
    }
    return s_result;
}

void SetEnv(const char* name, const char* value)
{
#ifdef _WIN32
    // get/setenv() and Set/GetEnvironmentVariable() is *not* compatible.
    // set both to make sure.
    ::_putenv_s(name, value);
    ::SetEnvironmentVariableA(name, value);
#else
    ::setenv(name, value, 1);
#endif
}


void* LoadModule(const char *path)
{
#ifdef _WIN32
    return ::LoadLibraryA(path);
#else  // _WIN32
    return ::dlopen(path, RTLD_NOW);
#endif //_WIN32
}

void* GetModule(const char *module_name)
{
#if defined(_WIN32)

    return ::GetModuleHandleA(module_name);

#elif defined(__APPLE__)

    uint32_t n = _dyld_image_count();
    for (uint32_t i = 0; i < n; ++i) {
        auto* path = _dyld_get_image_name(i);
        if (std::strstr(path, module_name))
            return dlopen(path, RTLD_LAZY);
    }
    return nullptr;

#else

    auto* mod = dlopen(nullptr, RTLD_LAZY);
    link_map* it = nullptr;
    dlinfo(mod, RTLD_DI_LINKMAP, &it);
    while (it) {
        if (std::strstr(it->l_name, module_name))
            return dlopen(it->l_name, RTLD_LAZY);
        it = it->l_next;
    }
    return nullptr;

#endif //_WIN32
}

void* GetSymbol(void* module, const char* name)
{
#ifdef _WIN32
    return ::GetProcAddress((HMODULE)module, name);
#else  // _WIN32
    return ::dlsym(module, name);
#endif //_WIN32
}


void InitializeSymbols(const char *path_)
{
#ifdef _WIN32
    auto path = path_ ? path_ : GetCurrentModuleDirectory();

    DWORD opt = ::SymGetOptions();
    opt |= SYMOPT_DEFERRED_LOADS;
    opt &= ~SYMOPT_UNDNAME;
    ::SymSetOptions(opt);
    ::SymInitialize(::GetCurrentProcess(), path.c_str(), TRUE);
#else  // _WIN32
#endif //_WIN32
}

void* FindSymbolByName(const char *name)
{
#ifdef _WIN32
    char buf[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
    PSYMBOL_INFO sinfo = (PSYMBOL_INFO)buf;
    sinfo->SizeOfStruct = sizeof(SYMBOL_INFO);
    sinfo->MaxNameLen = MAX_SYM_NAME;
    if (::SymFromName(::GetCurrentProcess(), name, sinfo) == FALSE) {
        return nullptr;
    }
    return (void*)sinfo->Address;
#else  // _WIN32
    return nullptr;
#endif //_WIN32
}


#ifdef _WIN32
struct cbEnumSymbolsCtx
{
    const char *name;
    void *ret;
};
BOOL CALLBACK cbEnumSymbols(PCSTR SymbolName, DWORD64 SymbolAddress, ULONG /*SymbolSize*/, PVOID UserContext)
{
    auto ctx = (cbEnumSymbolsCtx*)UserContext;
    if (std::strcmp(SymbolName, ctx->name) == 0) {
        ctx->ret = (void*)SymbolAddress;
        return FALSE;
    }
    return TRUE;

}
#endif //_WIN32

void* FindSymbolByName(const char *name, const char *module_name)
{
#ifdef _WIN32
    cbEnumSymbolsCtx ctx{ name, nullptr };
    SymEnumerateSymbols64(::GetCurrentProcess(), (ULONG64)GetModuleHandleA(module_name), cbEnumSymbols, &ctx);
    return ctx.ret;
#else  // _WIN32
    return nullptr;
#endif //_WIN32
}

int CaptureCallstack(void** dst, size_t dst_len)
{
#ifdef _WIN32
    std::fill_n(dst, dst_len, nullptr);
    return CaptureStackBackTrace(2, (DWORD)dst_len, dst, nullptr);
#else
    return 0;
#endif //_WIN32
}

void AddressToSymbolName(char *dst, size_t dst_len, void* address)
{
#ifdef _WIN32
#ifdef _WIN64
    using DWORDX = DWORD64;
    using PDWORDX = PDWORD64;
#else
    using DWORDX = DWORD;
    using PDWORDX = PDWORD;
#endif

    HANDLE proc = ::GetCurrentProcess();
    IMAGEHLP_MODULE mod = { sizeof(IMAGEHLP_MODULE) };
    IMAGEHLP_LINE line = { sizeof(IMAGEHLP_LINE) };
    DWORDX disp_sym = 0;
    DWORD disp_line = 0;

    char symbolBuffer[sizeof(IMAGEHLP_SYMBOL) + MAX_PATH] = { 0 };
    IMAGEHLP_SYMBOL* imageSymbol = (IMAGEHLP_SYMBOL*)symbolBuffer;
    imageSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
    imageSymbol->MaxNameLength = MAX_PATH;

    if (!::SymGetModuleInfo(proc, (DWORDX)address, &mod)) {
        snprintf(dst, dst_len, "[0x%p]", address);
    }
    else if (!::SymGetSymFromAddr(proc, (DWORDX)address, &disp_sym, imageSymbol)) {
        uint32_t distance = (uint32_t)((size_t)address - (size_t)mod.BaseOfImage);
        snprintf(dst, dst_len, "%s + 0x%x [0x%p]", mod.ModuleName, distance, address);
    }
    else if (!::SymGetLineFromAddr(proc, (DWORDX)address, &disp_line, &line)) {
        uint32_t distance = (uint32_t)((size_t)address - (size_t)imageSymbol->Address);
        snprintf(dst, dst_len, "%s!%s + 0x%x [0x%p]", mod.ModuleName, imageSymbol->Name, distance, address);
    }
    else {
        uint32_t distance = (uint32_t)((size_t)address - (size_t)imageSymbol->Address);
        snprintf(dst, dst_len, "%s(%d): %s!%s + 0x%x [0x%p]",
            line.FileName, line.LineNumber, mod.ModuleName, imageSymbol->Name, distance, address);
    }
#endif //_WIN32
}

void SetMemoryProtection(void *addr, size_t size, MemoryFlags flags)
{
#ifdef _WIN32
    DWORD flag = 0;
    switch (flags) {
    case MemoryFlags::ReadWrite: flag = PAGE_READWRITE; break;
    case MemoryFlags::ExecuteRead: flag = PAGE_EXECUTE_READ; break;
    case MemoryFlags::ExecuteReadWrite: flag = PAGE_EXECUTE_READWRITE; break;
    }
    DWORD old_flag;
    VirtualProtect(addr, size, flag, &old_flag);
#else
    int flag = 0;
    switch (flags) {
    case MemoryFlags::ReadWrite: flag = PROT_READ | PROT_WRITE; break;
    case MemoryFlags::ExecuteRead: flag = PROT_EXEC | PROT_READ; break;
    case MemoryFlags::ExecuteReadWrite: flag = PROT_EXEC | PROT_READ | PROT_WRITE; break;
    }
    void *page = (void*)((size_t)addr - ((size_t)addr % getpagesize()));
    mprotect(page, size, flag);
#endif
}


} // namespace mu
