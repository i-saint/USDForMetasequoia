#include "pch.h"
#include "sgSerialization.h"
#include "sgSerializationImpl.h"

namespace sg {

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

hptr serializer::getPointerRecord(void* v)
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

void deserializer::setPointerRecord(hptr h, void* v)
{
    uint32_t index = h.getIndex();
    while (m_pointer_records.size() < index)
        m_pointer_records.resize(std::max<size_t>(256, m_pointer_records.size() * 2));
    m_pointer_records[index] = v;
}

bool deserializer::getPointer_(hptr h, void*& v)
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
    v = m_pointer_records[index];
    return true;
}

} // namespace sg
