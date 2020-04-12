#pragma once
#include "MeshUtils/MeshUtils.h"
#include "sgSerialization.h"
#include "sgUtils.h"

namespace sg {

using type_creator = void*(*)();
void register_type(const char* name, type_creator creator);
void* create_instance_(const char* name);

template<class T>
T* create_instance(const char* name) { return static_cast<T*>(create_instance_(name)); }


struct type_registrar
{
    type_registrar(const char* name, type_creator creator)
    {
        register_type(name, creator);
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


template<class T>
struct serializable_pod
{
    static constexpr bool value = std::is_pod<T>::value && !std::is_pointer<T>::value;
};

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

// POD array
template<class T, sgEnableIf(serializable_pod<T>::value)>
inline void write(serializer& s, const T* v, uint32_t n)
{
    s.write(v, sizeof(T) * n);
    write_align(s, sizeof(T) * n);
}
template<class T, sgEnableIf(serializable_pod<T>::value)>
inline void read(deserializer& d, T* v, uint32_t n)
{
    d.read(v, sizeof(T) * n);
    read_align(d, sizeof(T) * n);
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

inline uint32_t write_string(serializer& s, const char *v)
{
    uint32_t len = (uint32_t)std::strlen(v);
    write(s, len);
    write(s, v, len);
    write_align(s, len);
    return len;
}
inline uint32_t read_string(deserializer& d, char*& v, uint32_t n)
{
    uint32_t len;
    read(d, len);
    if (len >= n) {
        // exceeds buffer size. create new string.
        v = new char[len + 1];
    }

    read(d, v, len);
    read_align(d, len);
    v[len] = '\0';
    return len;
}

// serializable pointer
template<class T, sgEnableIf(serializable<T>::value)>
inline hptr write(serializer& s, T* const& v)
{
    hptr handle = s.getHandle(v);
    write(s, handle);
    if (handle.isFlesh()) {
        // write type name
        write_string(s, typeid(*v).name());
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
        // read type name
        char name_buf[1024];
        char* name = name_buf;
        read_string(d, name, sizeof(name_buf));
        v = create_instance<T>(name);

        if (name != name_buf) {
#ifdef mqusdDebug
            mu::DbgBreak();
#endif
            delete[] name;
        }

        d.setPointer(handle, v);
        read(d, *v);
    }
    else {
        d.getPointer(handle, v);
    }
    return handle;
}



// specializations

template<class T>
struct serialize_nonintrusive
{
    static constexpr bool value = true;
};

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
            // shared_from_this() can be used only when already owned. so, track ref count.
            if (d.getRecord(handle).ref++ == 0)
                v.reset(p);
            else
                v = p->shared_from_this();
        }
#ifdef mqusdDebug
        else if(!handle.isNull()) {
            mu::DbgBreak();
        }
#endif
    }
};

template<class T>
struct serializable<std::basic_string<T>> : serialize_nonintrusive<T>
{
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
        read(d, (T*)v.data(), size);
    }
};

template<class T>
struct serializable<std::vector<T>> : serialize_nonintrusive<T>
{
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
struct serializable<SharedVector<T>> : serialize_nonintrusive<T>
{
    static void serialize(serializer& s, const SharedVector<T>& v)
    {
        uint32_t size = (uint32_t)v.size();
        write(s, size);
        write(s, v.cdata(), size);
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
            read(d, v.data(), size);
        }
        read_align(d, sizeof(T) * size); // align
    }
};

} // namespace sg


#define sgRegisterClass(T)\
    static void* create_##T() { return new T(); }\
    static sg::type_registrar s_register_##T(typeid(T).name(), create_##T);

#define sgWrite(V) sg::write(s, V);
#define sgRead(V) sg::read(d, V);
