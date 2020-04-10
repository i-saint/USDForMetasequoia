#include "pch.h"
#include "mudbg.h"
#include "../muMisc.h"

#define muvgMaxCallstack 44
#define muvgFillPattern 0xaa

namespace mu {

class AllocTable
{
public:
    struct muAllocRecord
    {
        size_t index;
        size_t size_alloc;
        size_t size_required;
        void* addr;
        void* callstack[muvgMaxCallstack];
    };
    using lock_t = std::lock_guard<std::mutex>;

    static AllocTable* getInstance();
    void append(void* addr, size_t size_alloc, size_t size_required);
    void erase(void* addr);

    void reportError();
    void printRecords();

private:
    AllocTable();
    void print(muAllocRecord& rec);
    void checkAndReport(muAllocRecord& rec);

    std::map<void*, muAllocRecord> m_records;
    std::mutex m_mutex;
    size_t m_index = 0;
};


AllocTable* AllocTable::getInstance()
{
    static AllocTable* s_inst = new AllocTable(); // never destroyed
    return s_inst;
}

AllocTable::AllocTable()
{
    mu::InitializeSymbols();
}

void AllocTable::append(void* addr, size_t size_alloc, size_t size_required)
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

    size_t sentinel_size = size_alloc - size_required;
    uint8_t* sentinel = (uint8_t*)addr + size_required;
    memset(sentinel, muvgFillPattern, sentinel_size);

    mu::CaptureCallstack(rec->callstack, muvgMaxCallstack);
}

void AllocTable::erase(void* addr)
{
    if (!addr)
        return;

    lock_t l(m_mutex);
    auto it = m_records.find(addr);
    if (it == m_records.end()) {
        mu::Print("muAllocTable::erase()\n");
        mu::DbgBreak();
    }
    else {
        checkAndReport(it->second);
        m_records.erase(it);
    }
}

void AllocTable::print(muAllocRecord& rec)
{
    mu::Print("Alloc Record %d\n", (uint32_t)rec.index);
    mu::Print("  Address 0x%p \n", rec.addr);
    mu::Print("  Size Allocated %d \n", (uint32_t)rec.size_alloc);
    mu::Print("  Size Requested %d \n", (uint32_t)rec.size_required);
    mu::Print("  Callstack:\n");

    char buf[2048];
    for (int i = 0; i < muvgMaxCallstack; ++i) {
        void* addr = rec.callstack[i];
        if (!addr)
            break;
        mu::AddressToSymbolName(buf, sizeof(buf), rec.callstack[i]);
        mu::Print("    %s\n", buf);
    }
}

void AllocTable::checkAndReport(muAllocRecord& rec)
{
    size_t sentinel_size = rec.size_alloc - rec.size_required;
    uint8_t* sentinel = (uint8_t*)rec.addr + rec.size_required;
    for (int i = 0; i < sentinel_size; ++i) {
        if (sentinel[i] != muvgFillPattern) {
            mu::Print("muAllocTable::reportCorruption()\n");
            print(rec);
            mu::DbgBreak();
            break;
        }
    }
}

void AllocTable::reportError()
{
    lock_t l(m_mutex);
    for (auto& kvp : m_records)
        checkAndReport(kvp.second);
}

void AllocTable::printRecords()
{
    lock_t l(m_mutex);
    for (auto& kvp : m_records)
        print(kvp.second);
}

} // namespace mu


mudbgAPI void muvgOnAllocate(void* addr, size_t size_alloc, size_t size_required)
{
    mu::AllocTable::getInstance()->append(addr, size_alloc, size_required);
}

mudbgAPI void muvgOnFree(void* addr)
{
    mu::AllocTable::getInstance()->erase(addr);
}

mudbgAPI void muvgReportError()
{
    mu::AllocTable::getInstance()->reportError();
}

mudbgAPI void muvgPrintRecords()
{
    mu::AllocTable::getInstance()->printRecords();
}
