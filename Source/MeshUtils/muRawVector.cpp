#include "pch.h"
#include "muRawVector.h"

//#if defined(muDebug) && defined(_WIN32)
//    #define muDbgVectorGuard
//#endif

#ifdef muDbgVectorGuard
#include "muMisc.h"

class muAllocTable
{
public:
    static const int sentinel_pattern = 0xaa;
    static const int sentinel_size = 4;

    struct muAllocRecord
    {
        size_t index;
        size_t size_alloc;
        size_t size_required;
        void* addr;
        void* callstack[32];
    };

    static muAllocTable& getInstance();
    void append(void* addr, size_t size_alloc, size_t size_required);
    void erase(void* addr);

    void print(muAllocRecord& rec);
    void checkAndReport(muAllocRecord& rec);
    void reportCorruption();
    void printRecords();

private:
    using lock_t = std::lock_guard<std::mutex>;

    muAllocTable();

    std::map<void*, muAllocRecord> m_records;
    std::mutex m_mutex;
    size_t m_index = 0;
};

muAllocTable& muAllocTable::getInstance()
{
    static muAllocTable s_inst;
    return s_inst;
}

muAllocTable::muAllocTable()
{
    mu::InitializeSymbols();
}

void muAllocTable::append(void* addr, size_t size_alloc, size_t size_required)
{
    muAllocRecord* rec = nullptr;
    {
        lock_t l(m_mutex);
        rec = &m_records[addr];
    }
    rec->addr = addr;
    rec->index = m_index++;
    rec->size_alloc = size_alloc;
    rec->size_required = size_required;

    byte* sentinel = (byte*)addr + size_required;
    memset(sentinel, sentinel_pattern, sentinel_size);

    mu::CaptureCallstack(rec->callstack, _countof(rec->callstack));
}

void muAllocTable::erase(void* addr)
{
    lock_t l(m_mutex);
    auto it = m_records.find(addr);
    if (it == m_records.end()) {
        mu::Print("muAllocTable::erase()\n");
        ::DebugBreak();
    }
    else {
        checkAndReport(it->second);
        m_records.erase(it);
    }
}

void muAllocTable::print(muAllocRecord& rec)
{
    mu::Print("Alloc Record %d\n", (uint32_t)rec.index);
    mu::Print("  Address 0x%p \n", rec.addr);
    mu::Print("  Size Allocated %d \n", (uint32_t)rec.size_alloc);
    mu::Print("  Size Requested %d \n", (uint32_t)rec.size_required);
    mu::Print("  Callstack:\n");

    char buf[2048];
    for (int i = 0; i < _countof(rec.callstack); ++i) {
        void* addr = rec.callstack[i];
        if (!addr)
            break;
        mu::AddressToSymbolName(buf, sizeof(buf), rec.callstack[i]);
        mu::Print("    %s\n", buf);
    }
}

void muAllocTable::checkAndReport(muAllocRecord& rec)
{
    byte* sentinel = (byte*)rec.addr + rec.size_required;
    for (int i = 0; i < sentinel_size; ++i) {
        if (sentinel[i] != sentinel_pattern) {
            mu::Print("muAllocTable::reportCorruption()\n");
            print(rec);
            ::DebugBreak();
            break;
        }
    }
}

void muAllocTable::reportCorruption()
{
    lock_t l(m_mutex);
    for (auto& kvp : m_records)
        checkAndReport(kvp.second);
}

void muAllocTable::printRecords()
{
    lock_t l(m_mutex);
    for (auto& kvp : m_records)
        print(kvp.second);
}
#endif

void* muMalloc(size_t size_required, size_t alignment)
{
    size_t mask = alignment - 1;
    size_t size_alloc = (size_required + mask) & (~mask);
#ifdef muDbgVectorGuard
    size_alloc += muAllocTable::sentinel_size;
#endif

#ifdef _WIN32
    void* ret = _mm_malloc(size_alloc, alignment);
#else
    void *ret = nullptr;
    posix_memalign(&ret, alignment, size_alloc);
#endif

#ifdef muDbgVectorGuard
    muAllocTable::getInstance().append(ret, size_alloc, size_required);
#endif
    return ret;
}

void muFree(void *addr)
{
#ifdef muDbgVectorGuard
    if (addr)
        muAllocTable::getInstance().erase(addr);
#endif

#ifdef _WIN32
    _mm_free(addr);
#else
    free(addr);
#endif
}

bool muVectorGuardEnabled()
{
#ifdef muDbgVectorGuard
    return true;
#else
    return false;
#endif
}

void muReportCorruption()
{
#ifdef muDbgVectorGuard
    muAllocTable::getInstance().reportCorruption();
#endif
}

void muReportRecords()
{
#ifdef muDbgVectorGuard
    muAllocTable::getInstance().printRecords();
#endif
}
