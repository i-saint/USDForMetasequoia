#include "pch.h"
#include "muRawVector.h"

#ifdef muDebug
    #define muDbgVectorGuard
#endif

#ifdef muDbgVectorGuard
#include "muMisc.h"

#define mudbgDll muDLLPrefix "mudbg" muDLLSuffix

static void* g_mudbg_module;
void (*_muvgOnAllocate)(void* addr, size_t size_alloc, size_t size_required);
void (*_muvgOnFree)(void* addr);
void (*_muvgReportCorruption)();
void (*_muvgPrintRecords)();

static void muvgInitializeImpl()
{
    if (g_mudbg_module)
        return;

    g_mudbg_module = mu::GetModule(mudbgDll);
    if (!g_mudbg_module) {
        std::string path = mu::GetCurrentModuleDirectory();
        path += muPathSep;
        path += mudbgDll;
        g_mudbg_module = mu::LoadModule(path.c_str());
    }
    if (g_mudbg_module) {
#define GetSym(Name) (void*&)_##Name = mu::GetSymbol(g_mudbg_module, #Name)
        GetSym(muvgOnAllocate);
        GetSym(muvgOnFree);
        GetSym(muvgReportCorruption);
        GetSym(muvgPrintRecords);
#undef GetSym
    }
}
#endif // muDbgVectorGuard

void muvgInitialize()
{
#ifdef muDbgVectorGuard
    static std::once_flag s_once;
    std::call_once(s_once, []() { muvgInitializeImpl(); });
#endif
}



void* muMalloc(size_t size_required, size_t alignment)
{
#ifdef muDbgVectorGuard
    muvgInitialize();
    size_t pad = 16;
#else
    size_t pad = 0;
#endif

    size_t mask = alignment - 1;
    size_t size_alloc = (size_required + pad + mask) & (~mask);

#ifdef _WIN32
    void* ret = _mm_malloc(size_alloc, alignment);
#else
    void *ret = nullptr;
    posix_memalign(&ret, alignment, size_alloc);
#endif

#ifdef muDbgVectorGuard
    if (_muvgOnAllocate)
        _muvgOnAllocate(ret, size_alloc, size_required);
#endif
    return ret;
}

void muFree(void *addr)
{
#ifdef muDbgVectorGuard
    if (_muvgOnFree)
        _muvgOnFree(addr);
#endif

#ifdef _WIN32
    _mm_free(addr);
#else
    free(addr);
#endif
}

bool muvgEnabled()
{
#ifdef muDbgVectorGuard
    return true;
#else
    return false;
#endif
}

void muvgReportCorruption()
{
#ifdef muDbgVectorGuard
    if (_muvgReportCorruption)
        _muvgReportCorruption();
#endif
}

void muvgPrintRecords()
{
#ifdef muDbgVectorGuard
    if (_muvgPrintRecords)
        _muvgPrintRecords();
#endif
}
