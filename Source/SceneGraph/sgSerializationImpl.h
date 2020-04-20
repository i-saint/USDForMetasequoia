#pragma once
#include "MeshUtils/MeshUtils.h"
#include "sgSerialization.h"
#include "sgUtils.h"

#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>


namespace sg {

using creator_t = void* (*)();
void register_type(const char* name, creator_t c);

void* create_instance_(const char* name);
template<class T> T* create_instance(const char* name) { return static_cast<T*>(create_instance_(name)); }

template<class T>
struct type_registrar
{
    static void* create() { return new T(); }

    type_registrar()
    {
        register_type(typeid(T).name(), &create);
    }
};


// keep 4 byte align to directly map with SharedVector later.
static const int serialize_align = 4;

inline void write_align(serializer& s, size_t written_size)
{
    const int zero = 0;
    if (written_size % serialize_align != 0)
        s.write(&zero, serialize_align - (written_size % serialize_align));
}
inline void read_align(deserializer& d, size_t read_size)
{
    int dummy = 0;
    if (read_size % serialize_align != 0)
        d.read(&dummy, serialize_align - (read_size % serialize_align));
}

template<size_t n, sgEnableIf(n % serialize_align == 0)> inline void write_align(serializer&) {}
template<size_t n, sgEnableIf(n % serialize_align != 0)> inline void write_align(serializer& s)
{
    const int zero = 0;
    s.write(&zero, serialize_align - (n % serialize_align));
}

template<size_t n, sgEnableIf(n % serialize_align == 0)> inline void read_align(deserializer&) {}
template<size_t n, sgEnableIf(n % serialize_align != 0)> inline void read_align(deserializer& d)
{
    int dummy = 0;
    d.read(&dummy, serialize_align - (n % serialize_align));
}


// POD
template<class T, sgEnableIf(serializable_pod<T>::value)>
inline void write(serializer& s, const T& v)
{
    s.write(&v, sizeof(T));
    write_align<sizeof(T)>(s);
}
template<class T, sgEnableIf(serializable_pod<T>::value)>
inline void read(deserializer& d, T& v)
{
    d.read(&v, sizeof(T));
    read_align<sizeof(T)>(d);
}


// serializable object
template<class T, sgEnableIf(serializable<T>::value)>
inline void write(serializer& s, const T& v)
{
    serializable<T>::serialize(s, v);
}
template<class T, sgEnableIf(serializable<T>::value)>
inline void read(deserializer& d, T& v)
{
    serializable<T>::deserialize(d, v);
}


// serializable pointer
template<class T, sgEnableIf(serializable<T>::value)>
inline hptr write(serializer& s, T* const& v)
{
    hptr handle = s.getHandle(v);
    write(s, handle);
    if (handle.isFlesh()) {
        // write type name
        const char* type_name = typeid(*v).name();
        uint32_t name_len = (uint32_t)std::strlen(type_name);
        write(s, name_len);
        s.write(type_name, name_len);
        write_align(s, name_len);

        // write data
        write(s, *v);
    }
    return handle;
}
template<class T, sgEnableIf(serializable<T>::value)>
inline hptr read(deserializer& d, T*& v)
{
    hptr handle;
    read(d, handle);
    if (handle.isFlesh()) {
        // read type name and create instance
        uint32_t name_len;
        read(d, name_len);

        auto read_name = [&d, name_len](char* name) {
            d.read(name, name_len);
            read_align(d, name_len);
            name[name_len] = '\0';
        };

        if (name_len < 1024) {
            char name[1024];
            read_name(name);
            v = create_instance<T>(name);
        }
        else {
            char *name = new char[name_len + 1];
            read_name(name);
            v = create_instance<T>(name);
            delete[] name;
        }

        // register pointer
        d.setPointer(handle, v);

        // deserialize instance
        read(d, *v);
    }
    else {
        d.getPointer(handle, v);
    }
    return handle;
}



// POD fixed array
template<class T, uint32_t N, sgEnableIf(serializable_pod<T>::value)>
inline void write(serializer& s, const T(&v)[N])
{
    s.write(v, sizeof(T) * N);
    write_align(s, sizeof(T) * N);
}
template<class T, uint32_t N, sgEnableIf(serializable_pod<T>::value)>
inline void read(deserializer& d, T(&v)[N])
{
    d.read(v, sizeof(T) * N);
    read_align(d, sizeof(T) * N);
}

// object fixed array
template<class T, size_t N, sgEnableIf(serializable<T>::value)>
inline void write(serializer& s, const T(&v)[N])
{
    for (size_t i = 0; i < N; ++i)
        s.write(v[i]);
}
template<class T, size_t N, sgEnableIf(serializable<T>::value)>
inline void read(deserializer& d, T(&v)[N])
{
    for (size_t i = 0; i < N; ++i)
        d.read(v[i]);
}

// pointer fixed array
template<class T, size_t N, sgEnableIf(serializable<T>::value)>
static void write(serializer& s, const T* (&v)[N])
{
    for (size_t i = 0; i < N; ++i)
        s.write(v[i]);
}
template<class T, size_t N, sgEnableIf(serializable<T>::value)>
static void read(deserializer& d, T* (&v)[N])
{
    for (size_t i = 0; i < N; ++i)
        d.read(v[i]);
}

// POD array
template<class T, sgEnableIf(serializable_pod<T>::value)>
inline void write_array(serializer& s, const T* v, uint32_t n)
{
    s.write(v, sizeof(T) * n);
    write_align(s, sizeof(T) * n);
}
template<class T, sgEnableIf(serializable_pod<T>::value)>
inline void read_array(deserializer& d, T* v, uint32_t n)
{
    d.read(v, sizeof(T) * n);
    read_align(d, sizeof(T) * n);
}

// object array
template<class T, sgEnableIf(serializable<T>::value)>
static void write_array(serializer& s, const T* v, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i)
        write(s, v[i]);
}
template<class T, sgEnableIf(serializable<T>::value)>
static void read_array(deserializer& d, T* v, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i)
        read(d, v[i]);
}

// pointer array
template<class T, sgEnableIf(serializable<T>::value)>
static void write_array(serializer& s, T* const* v, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i)
        write(s, v[i]);
}
template<class T, sgEnableIf(serializable<T>::value)>
static void read_array(deserializer& d, T** v, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i)
        read(d, v[i]);
}


// specializations

template<class T>
struct serializable<std::unique_ptr<T>> : serialize_nonintrusive<T>
{
    static void serialize(serializer& s, const std::unique_ptr<T>& v)
    {
        write(s, v.get());
    }

    static void deserialize(deserializer& d, std::unique_ptr<T>& v)
    {
        T* p;
        read(d, p);
        v.reset(p);
    }
};

template<class T>
struct serializable<std::shared_ptr<T>> : serialize_nonintrusive<T>
{
    static void serialize(serializer& s, const std::shared_ptr<T>& v)
    {
        write(s, v.get());
    }

    static void deserialize(deserializer& d, std::shared_ptr<T>& v)
    {
        T* p;
        hptr handle = read(d, p);
        if (p) {
            auto& rec = d.getRecord(handle);
            if (!rec.shared) {
                v.reset(p);
                rec.shared = v;
            }
            else {
                v = std::static_pointer_cast<T>(rec.shared);
            }
        }
#ifdef mqusdDebug
        else if(!handle.isNull()) {
            mu::DbgBreak();
        }
#endif
    }
};

template<class T>
struct serializable<std::basic_string<T>> : serialize_nonintrusive<std::basic_string<T>>
{
    static void serialize(serializer& s, const std::basic_string<T>& v)
    {
        uint32_t size = (uint32_t)v.size();
        write(s, size);
        write_array(s, v.data(), size);
    }

    static void deserialize(deserializer& d, std::basic_string<T>& v)
    {
        uint32_t size;
        read(d, size);
        v.resize(size);
        read_array(d, (T*)v.data(), size);
    }
};


// std::vector
template<class T>
struct serializable<std::vector<T>> : serialize_nonintrusive<std::vector<T>>
{
    static void serialize(serializer& s, const std::vector<T>& v)
    {
        uint32_t size = (uint32_t)v.size();
        write(s, size);
        write_array(s, v.data(), size);
    }

    static void deserialize(deserializer& d, std::vector<T>& v)
    {
        uint32_t size;
        read(d, size);
        v.resize(size);
        read_array(d, v.data(), size);
    }
};

// std::list
template<class T>
struct serializable<std::list<T>> : serialize_nonintrusive<std::list<T>>
{
    static void serialize(serializer& s, const std::list<T>& v)
    {
        uint32_t size = (uint32_t)v.size();
        write(s, size);
        for (const auto& e : v)
            write(s, e);
    }

    static void deserialize(deserializer& d, std::list<T>& v)
    {
        uint32_t size;
        read(d, size);
        v.resize(size);
        for (auto& e : v)
            read(d, e);
    }
};

// std::set
template<class T>
struct serializable<std::set<T>> : serialize_nonintrusive<std::set<T>>
{
    static void serialize(serializer& s, const std::set<T>& v)
    {
        uint32_t size = (uint32_t)v.size();
        write(s, size);
        for (const auto& e : v)
            write(s, e);
    }

    static void deserialize(deserializer& d, std::set<T>& v)
    {
        uint32_t size;
        read(d, size);
        for (uint32_t i = 0; i < size; ++i) {
            T value;
            read(d, value);
            v.insert(std::move(value));
        }
    }
};

// std::map
template<class K, class V>
struct serializable<std::map<K, V>> : serialize_nonintrusive<std::map<K, V>>
{
    static void serialize(serializer& s, const std::map<K, V>& v)
    {
        uint32_t size = (uint32_t)v.size();
        write(s, size);
        for (const auto& kvp : v) {
            write(s, kvp.first);
            write(s, kvp.second);
        }
    }

    static void deserialize(deserializer& d, std::map<K, V>& v)
    {
        uint32_t size;
        read(d, size);
        for (uint32_t i = 0; i < size; ++i) {
            K key;
            V value;
            read(d, key);
            read(d, value);
            v.insert(std::make_pair(std::move(key), std::move(value)));
        }
    }
};


// RawVector
template<class T>
struct serializable<RawVector<T>> : serialize_nonintrusive<RawVector<T>>
{
    static void serialize(serializer& s, const RawVector<T>& v)
    {
        uint32_t size = (uint32_t)v.size();
        write(s, size);
        write_array(s, v.cdata(), size);
        write_align(s, sizeof(T) * size); // align
    }

    static void deserialize(deserializer& d, RawVector<T>& v)
    {
        uint32_t size;
        read(d, size);
        v.resize_discard(size);
        read_array(d, v.data(), size);
        read_align(d, sizeof(T) * size); // align
    }
};

// SharedVector
template<class T>
struct serializable<SharedVector<T>> : serialize_nonintrusive<SharedVector<T>>
{
    static void serialize(serializer& s, const SharedVector<T>& v)
    {
        uint32_t size = (uint32_t)v.size();
        write(s, size);
        write_array(s, v.cdata(), size);
        write_align(s, sizeof(T) * size); // align
    }

    static void deserialize(deserializer& d, SharedVector<T>& v)
    {
        uint32_t size;
        read(d, size);

        auto& stream = d.getStream();
        if (typeid(stream) == typeid(mu::MemoryStream)) {
            // just share buffer (no copy)
            auto& ms = static_cast<mu::MemoryStream&>(stream);
            v.share((T*)ms.gskip(sizeof(T) * size), size);
        }
        else {
            v.resize_discard(size);
            read_array(d, v.data(), size);
        }
        read_align(d, sizeof(T) * size); // align
    }
};

} // namespace sg


#define sgConcat2(x,y) x##y
#define sgConcat(x, y) sgConcat2(x, y)
#define sgRegisterType(T) static ::sg::type_registrar<T> sgConcat(g_sg_registrar, __COUNTER__);

#define sgWrite(V) ::sg::write(s, V);
#define sgRead(V) ::sg::read(d, V);
