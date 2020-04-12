#include "pch.h"
#include "sgSerialization.h"
#include "sgSerializationImpl.h"

namespace sg {

struct type_record
{
    const char* name;
    type_creator creator;
};

static std::vector<type_record>& get_type_records()
{
    static std::vector<type_record> s_records;
    return s_records;
}

void register_type(const char* name, type_creator creator)
{
    auto& records = get_type_records();
    records.push_back({ name, creator });
}

void* create_instance_(const char* name)
{
    static std::once_flag s_once;

    auto& records = get_type_records();
    std::call_once(s_once, [&records]() {
        // sort records
        std::sort(records.begin(), records.end(), [](auto& a, auto& b) {
            return std::strcmp(a.name, b.name) < 0;
        });
    });

    // find record and call creator
    auto it = std::lower_bound(records.begin(), records.end(), name, [](auto& a, const char* name) {
        return std::strcmp(a.name, name) < 0;
    });
    if (it != records.end() && std::strcmp(it->name, name) == 0) {
        return it->creator();
    }
    else {
        // should not be here
        mu::DbgBreak();
        return nullptr;
    }
}


serializer::serializer(std::ostream& s)
    : m_stream(s)
{
}

serializer::~serializer()
{
}

void serializer::write(const void* v, size_t size)
{
    m_stream.write((char*)v, size);
}

std::ostream& serializer::getStream()
{
    return m_stream;
}

hptr serializer::getHandle(pointer_t v)
{
    if (!v)
        return { 0 };

    hptr ret;
    uint32_t& index = m_pointer_records[v];
    if (index == 0) {
        index = (uint32_t)m_pointer_records.size();
        ret = { index | hptr::kFleshFlag };
    }
    else {
        ret = { index };
    }
    return ret;
}

deserializer::deserializer(std::istream& s)
    : m_stream(s)
{
}

deserializer::~deserializer()
{
}

void deserializer::read(void* v, size_t size)
{
    m_stream.read((char*)v, size);
}

std::istream& deserializer::getStream()
{
    return m_stream;
}

void deserializer::setPointer(hptr h, pointer_t v)
{
    uint32_t index = h.getIndex();
    while (m_pointer_records.size() <= index)
        m_pointer_records.resize(std::max<size_t>(256, m_pointer_records.size() * 2));
    m_pointer_records[index].pointer = v;
}

deserializer::Record& deserializer::getRecord(hptr h)
{
    return m_pointer_records[h.getIndex()];
}

bool deserializer::getPointer_(hptr h, pointer_t& v)
{
    if (h.isNull()) {
        v = nullptr;
        return true;
    }

    uint32_t index = h.getIndex();
#ifdef mqusdDebug
    if (index >= m_pointer_records.size()) {
        // should not be here
        mu::DbgBreak();
        v = nullptr;
        return false;
    }
#endif
    v = m_pointer_records[index].pointer;
    return true;
}

} // namespace sg
