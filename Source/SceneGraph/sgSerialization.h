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
    static hptr null() { return hptr{0}; }

    bool isNull() const { return handle == 0; }
    bool isFlesh() const { return (handle & kFleshFlag) != 0; }
    uint32_t getIndex() const { return handle & kIndexMask;  }

    uint32_t handle;
};

class serializer
{
public:
    serializer(std::ostream& s);
    ~serializer();
    void write(const void* v, size_t size);

    std::ostream& getStream();
    hptr getPointerRecord(void* v);

private:
    std::ostream& m_stream;
    std::map<void*, uint32_t> m_pointer_records;
};

class deserializer
{
public:
    deserializer(std::istream& s);
    ~deserializer();
    void read(void* v, size_t size);

    std::istream& getStream();
    void setPointerRecord(hptr h, void* v);
    bool getPointer_(hptr h, void*& v);

    template<class T>
    bool getPointer(hptr h, T*& v)
    {
        return getPointer_(h, (void*&)v);
    }

private:
    std::istream& m_stream;
    std::vector<void*> m_pointer_records;
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
    static void serialize(serializer& s, const T& v) { const_cast<T&>(v).serialize(s); }
    static void deserialize(deserializer& d, T& v) { v.deserialize(d); }
};

} // namespace sg


#define sgSerializable(T) template<> struct serializable<T> : serialize_intrusive<T> {};
#define sgDeclPtr(T) using T##Ptr = std::shared_ptr<T>
