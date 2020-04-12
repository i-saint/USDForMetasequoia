#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <type_traits>

namespace sg {

struct hptr
{
    enum {
        kIndexMask = 0x7fffffff,
        kFleshFlag = 0x80000000,
    };

    bool isNull() const { return handle == 0; }
    bool isFlesh() const { return (handle & kFleshFlag) != 0; }
    uint32_t getIndex() const { return handle & kIndexMask;  }

    uint32_t handle;
};

class serializer
{
public:
    using pointer_t = const void*;

    serializer(std::ostream& s);
    ~serializer();
    void write(const void* v, size_t size);

    std::ostream& getStream();
    hptr getHandle(pointer_t v);

private:
    std::ostream& m_stream;
    std::map<pointer_t, uint32_t> m_pointer_records;
};

class deserializer
{
public:
    using pointer_t = void*;
    struct Record
    {
        pointer_t pointer = nullptr;
        int ref = 0;
    };

    deserializer(std::istream& s);
    ~deserializer();
    void read(void* v, size_t size);

    std::istream& getStream();
    void setPointer(hptr h, pointer_t v);
    Record& getRecord(hptr h);
    bool getPointer_(hptr h, pointer_t& v);

    template<class T>
    bool getPointer(hptr h, T*& v)
    {
        return getPointer_(h, (pointer_t&)v);
    }

private:
    std::istream& m_stream;
    std::vector<Record> m_pointer_records;
};


template<class T>
struct serializable
{
    static constexpr bool value = false;
};

template<class T>
struct serialize_intrusive
{
    static constexpr bool value = true;
    static void serialize(serializer& s, const T& v) { v.serialize(s); }
    static void deserialize(deserializer& d, T& v) { v.deserialize(d); }
};

} // namespace sg


#define sgSerializable(T) template<> struct serializable<T> : serialize_intrusive<T> {};
#define sgDeclPtr(T) using T##Ptr = std::shared_ptr<T>
