#include "pch.h"
#include "sgSerialization.h"
#include "sgSerializationImpl.h"

namespace sg {

// type_table

struct type_record
{
    const char* name;
    creator_t creator;
};

class type_table
{
public:
    static type_table& getInstance();
    void append(const char* name, creator_t c);
    type_record* find(const char* name);

private:
    type_table();
    void sort();

private:
    std::vector<type_record> m_records;
    bool m_dirty = false;
};


type_table& type_table::getInstance()
{
    static type_table s_inst;
    return s_inst;
}

type_table::type_table()
{
}

void type_table::append(const char* name, creator_t c)
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

type_record* type_table::find(const char* name)
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

void type_table::sort()
{
    if (!m_dirty)
        return;

    m_dirty = false;
    std::sort(m_records.begin(), m_records.end(), [](type_record& a, type_record& b) {
        return std::strcmp(a.name, b.name) < 0;
        });
}


void register_type(const char* name, creator_t c)
{
    type_table::getInstance().append(name, c);
}

void* create_instance_(const char* name)
{
    if (type_record* rec = type_table::getInstance().find(name))
        return rec->creator();

#ifdef sgDebug
    // type not found. should not be here.
    mu::Print("create_instance_(): type \"%s\" is not registered. maybe forgot sgRegisterType()?\n", name);
    mu::DbgBreak();
#endif
    return nullptr;
}


// serializer

struct serializer::impl
{
    std::ostream& stream;
    std::map<pointer_t, uint32_t> pointer_records;

    impl(std::ostream& s) : stream(s) {}
};

serializer::serializer(std::ostream& s)
    : m_impl(std::make_unique<impl>(s))
{
}

serializer::~serializer()
{
}

void serializer::write(const void* v, size_t size)
{
    m_impl->stream.write((char*)v, size);
}

std::ostream& serializer::getStream()
{
    return m_impl->stream;
}

hptr serializer::getHandle(pointer_t v)
{
    if (!v)
        return { 0 };

    hptr ret;
    uint32_t& index = m_impl->pointer_records[v];
    if (index == 0) {
        index = (uint32_t)m_impl->pointer_records.size();
        ret = { index | hptr::kFleshFlag };
    }
    else {
        ret = { index };
    }
    return ret;
}


// deserializer

struct deserializer::impl
{
    std::istream& stream;
    std::vector<Record> pointer_records;

    impl(std::istream& s) : stream(s) {}
};

deserializer::deserializer(std::istream& s)
    : m_impl(std::make_unique<impl>(s))
{
}

deserializer::~deserializer()
{
}

void deserializer::read(void* v, size_t size)
{
    m_impl->stream.read((char*)v, size);
}

std::istream& deserializer::getStream()
{
    return m_impl->stream;
}

void deserializer::setPointer(hptr h, pointer_t v)
{
    uint32_t index = h.getIndex();
    auto& records = m_impl->pointer_records;
    while (records.size() <= index)
        records.resize(std::max<size_t>(256, records.size() * 2));
    records[index].pointer = v;
}

deserializer::Record& deserializer::getRecord(hptr h)
{
    return m_impl->pointer_records[h.getIndex()];
}

bool deserializer::getPointer_(hptr h, pointer_t& v)
{
    if (h.isNull()) {
        v = nullptr;
        return true;
    }

    uint32_t index = h.getIndex();
    auto& records = m_impl->pointer_records;
#ifdef sgDebug
    if (index >= records.size()) {
        // should not be here
        mu::DbgBreak();
        v = nullptr;
        return false;
    }
#endif
    v = records[index].pointer;
    return true;
}

} // namespace sg
