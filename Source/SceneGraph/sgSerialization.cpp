#include "pch.h"
#include "sgSerialization.h"
#include "sgSerializationImpl.h"

namespace sg {

struct type_record
{
    const char* name;
    creator_t creator;
};

class type_table
{
public:
    static type_table& getInstance()
    {
        static type_table s_inst;
        return s_inst;
    }

    void append(const char* name, creator_t c)
    {
        // if the type is already registered, update the record.
        // this is a common occurrence when hot-reloading dlls.
        if (auto* rec = find(name)) {
            rec->creator = c;
        }
        else {
            m_records.push_back({ name, c });
            m_dirty = true;
        }
    }

    type_record* find(const char* name)
    {
        sort();

        auto it = std::lower_bound(m_records.begin(), m_records.end(), name, [](type_record& a, const char* name) {
            return std::strcmp(a.name, name) < 0;
        });

        if (it != m_records.end() && std::strcmp(it->name, name) == 0)
            return &*it;
        else
            return nullptr;
    }

private:
    type_table()
    {}

    void sort()
    {
        if (!m_dirty)
            return;

        m_dirty = false;
        std::sort(m_records.begin(), m_records.end(), [](type_record& a, type_record& b) {
            return std::strcmp(a.name, b.name) < 0;
        });
    }

private:
    std::vector<type_record> m_records;
    bool m_dirty = false;
};

using register_type_t = void(*)(const char*, creator_t);
using create_instance_t = void*(*)(const char* name);

void register_type(const char* name, creator_t c)
{
    type_table::getInstance().append(name, c);
}

void* create_instance_(const char* name)
{
    if (type_record* rec = type_table::getInstance().find(name))
        return rec->creator();

    // type not found. should not be here.
    mu::Print("type %s not found. maybe forgot sgRegisterType()?\n", name);
    mu::DbgBreak();
    return nullptr;
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
