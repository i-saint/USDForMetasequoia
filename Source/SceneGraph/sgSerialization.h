#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <type_traits>
#include "sgUtils.h"

namespace sg {

using serializer = std::ostream;
using deserializer = std::istream;

template<class T>
struct serializable_pod
{
    static constexpr bool value = std::is_pod<T>::value && !std::is_pointer<T>::value;
};

template<class T>
struct serializable_obj
{
    static constexpr bool value = false;
};

template<class T>
struct serializable
{
    static constexpr bool value = false;
};



// align
static const int serialize_align = 4;

inline void write_align(serializer& s, size_t written_size)
{
    const int zero = 0;
    if (written_size % serialize_align != 0)
        s.write((const char*)&zero, serialize_align - (written_size % serialize_align));
}
inline void read_align(deserializer& d, size_t read_size)
{
    int dummy = 0;
    if (read_size % serialize_align != 0)
        d.read((char*)&dummy, serialize_align - (read_size % serialize_align));
}

template<size_t n, sgEnableIf(n % serialize_align == 0)> inline void write_align(serializer&) {}
template<size_t n, sgEnableIf(n % serialize_align != 0)> inline void write_align(serializer& s)
{
    const int zero = 0;
    s.write((const char*)&zero, serialize_align - (n % serialize_align));
}

template<size_t n, sgEnableIf(n % serialize_align == 0)> inline void read_align(deserializer&) {}
template<size_t n, sgEnableIf(n % serialize_align != 0)> inline void read_align(deserializer& d)
{
    int dummy = 0;
    d.read((char*)&dummy, serialize_align - (n % serialize_align));
}


// POD
template<class T, sgEnableIf(serializable_pod<T>::value)>
inline void write(serializer& s, const T& v)
{
    s.write((const char*)&v, sizeof(T));
    write_align<sizeof(T)>(s);
}
template<class T, sgEnableIf(serializable_pod<T>::value)>
inline void read(deserializer& d, T& v)
{
    d.read((char*)&v, sizeof(T));
    read_align<sizeof(T)>(d);
}

// POD array
template<class T, sgEnableIf(serializable_pod<T>::value)>
inline void write(serializer& s, const T* v, uint32_t n)
{
    s.write((const char*)v, sizeof(T) * n);
    write_align(s, sizeof(T) * n);
}
template<class T, sgEnableIf(serializable_pod<T>::value)>
inline void read(deserializer& d, T* v, uint32_t n)
{
    d.read((char*)v, sizeof(T) * n);
    read_align(d, sizeof(T) * n);
}

// other generic types
template<class T, sgEnableIf(serializable_obj<T>::value)>
inline void write(serializer& s, const T& v)
{
    serializable_obj<T>::serialize(s, v);
}
template<class T, sgEnableIf(serializable_obj<T>::value)>
inline void read(deserializer& d, T& v)
{
    serializable_obj<T>::deserialize(d, v);
}

// serializable
template<class T, sgEnableIf(serializable<T>::value)>
inline void write(serializer& s, const T& v)
{
    const_cast<T&>(v).serialize(s);
}
template<class T, sgEnableIf(serializable<T>::value)>
inline void read(deserializer& d, T& v)
{
    v.deserialize(d);
}


// specializations

template<class T>
struct serializable_obj<std::shared_ptr<T>>
{
    static constexpr bool value = true;

    static void serialize(serializer& s, const std::shared_ptr<T>& v)
    {
        // flag to distinct from null
        write(s, v ? 1 : 0);
        if (v)
            write(s, *v);
    }

    static void deserialize(deserializer& d, std::shared_ptr<T>& v)
    {
        int is_null;
        read(d, is_null);
        if (is_null != 0)
            T::deserialize(d, v);
        else
            v = {};
    }
};

template<class T>
struct serializable_obj<std::basic_string<T>>
{
    static constexpr bool value = true;

    static void serialize(serializer& s, const std::basic_string<T>& v)
    {
        uint32_t size = (uint32_t)v.size();
        write(s, size);
        write(s, v.data(), size);
    }

    static void deserialize(deserializer& d, std::basic_string<T>& v)
    {
        uint32_t size;
        read(d, size);
        v.resize(size);
        read(d, v.data(), size);
    }
};

template<class T>
struct serializable_obj<std::vector<T>>
{
    static constexpr bool value = true;

    static void serialize(serializer& s, const std::vector<T>& v)
    {
        uint32_t size = (uint32_t)v.size();
        write(s, size);
        for (const auto& e : v)
            write(s, e);
    }

    static void deserialize(deserializer& d, std::vector<T>& v)
    {
        uint32_t size;
        read(d, size);
        v.resize(size);
        for (auto& e : v)
            read(d, e);
    }
};

template<class T>
struct serializable_obj<SharedVector<T>>
{
    static constexpr bool value = true;

    static void serialize(serializer& s, const SharedVector<T>& v)
    {
        uint32_t size = (uint32_t)v.size();
        write(s, size);
        write(s, v.cdata(), size);
        write_align(s, sizeof(T) * size); // keep 4 byte alignment
    }

    static void deserialize(deserializer& d, SharedVector<T>& v)
    {
        uint32_t size;
        read(d, size);
        if (typeid(d) == typeid(mu::MemoryStream)) {
            // just share buffer (no copy)
            auto& ms = static_cast<mu::MemoryStream&>(d);
            v.share((T*)ms.gskip(sizeof(T) * size), size);
        }
        else {
            v.resize_discard(size);
            read(d, v.data(), size);
        }
        read_align(d, sizeof(T) * size); // align
    }
};

} // namespace sg

#define sgDeclPtr(T) using T##Ptr = std::shared_ptr<T>
#define sgSerializable(T) template<> struct serializable<T> { static constexpr bool value = true; };

#define sgSerialize(V) sg::write(s, V);
#define sgDeserialize(V) sg::read(d, V);
