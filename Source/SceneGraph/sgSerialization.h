#pragma once

namespace sg {

using serializer = std::ostream;
using deserializer = std::istream;

template<class T> struct serializable { static const bool value = false; };

template<class T, bool hs = serializable<T>::value> struct write_impl2;
template<class T> struct write_impl2<T, true>
{
    void operator()(serializer& s, T& v) const { v.serialize(s); }
};
template<class T> struct write_impl2<T, false>
{
    void operator()(serializer& s, T& v) const
    {
        static_assert(sizeof(T) % 4 == 0, "");
        s.write((const char*)&v, sizeof(T));
    }
};

template<class T, bool hs = serializable<T>::value> struct read_impl2;
template<class T> struct read_impl2<T, true> {
    void operator()(deserializer& d, T& v) const { v.deserialize(d); }
};
template<class T> struct read_impl2<T, false> {
    void operator()(deserializer& d, T& v) const { d.read((char*)&v, sizeof(T)); }
};


template<class T>
struct write_impl
{
    void operator()(serializer& s, T& v) const
    {
        write_impl2<T>()(s, v);
    }
};
template<class T>
struct read_impl
{
    void operator()(deserializer& d, T& v) const
    {
        read_impl2<T>()(d, v);
    }
};


inline void write_align(serializer& s, size_t written_size)
{
    const int zero = 0;
    if (written_size % 4 != 0)
        s.write((const char*)&zero, 4 - (written_size % 4));
}

template<>
struct write_impl<bool>
{
    void operator()(serializer& s, const bool& v) const
    {
        s.write((const char*)&v, sizeof(v));
        write_align(s, sizeof(v)); // keep 4 byte alignment
    }
};
template<class T>
struct write_impl<std::shared_ptr<T>>
{
    void operator()(serializer& s, const std::shared_ptr<T>& v) const
    {
        v->serialize(s);
    }
};
template<class T>
struct write_impl<SharedVector<T>>
{
    void operator()(serializer& s, const SharedVector<T>& v) const
    {
        auto size = (uint32_t)v.size();
        s.write((const char*)&size, sizeof(size));
        s.write((const char*)v.cdata(), sizeof(T) * size);
        write_align(s, sizeof(T) * size); // keep 4 byte alignment
    }
};
template<>
struct write_impl<std::string>
{
    void operator()(serializer& s, const std::string& v) const
    {
        auto size = (uint32_t)v.size();
        s.write((const char*)&size, 4);
        s.write((const char*)v.data(), size);
        write_align(s, size); // keep 4 byte alignment
    }
};
template<class T>
struct write_impl<std::vector<T>>
{
    void operator()(serializer& s, const std::vector<T>& v) const
    {
        auto size = (uint32_t)v.size();
        s.write((const char*)&size, 4);
        for (const auto& e : v)
            write_impl<T>()(s, e);
    }
};



inline void read_align(deserializer& d, size_t read_size)
{
    int dummy = 0;
    if (read_size % 4 != 0)
        d.read((char*)&dummy, 4 - (read_size % 4));
}

template<>
struct read_impl<bool>
{
    void operator()(deserializer& d, bool& v) const
    {
        d.read((char*)&v, sizeof(v));
        read_align(d, sizeof(v)); // align
    }
};
template<class T>
struct read_impl<std::shared_ptr<T>>
{
    void operator()(deserializer& d, std::shared_ptr<T>& v) const
    {
        T::deserialize(d, v);
    }
};
template<class T>
struct read_impl<SharedVector<T>>
{
    void operator()(deserializer& d, SharedVector<T>& v) const
    {
        uint32_t size = 0;
        d.read((char*)&size, sizeof(size));
        if (typeid(d) == typeid(mu::MemoryStream)) {
            // just share buffer (no copy)
            auto& ms = static_cast<mu::MemoryStream&>(d);
            v.share((T*)ms.gskip(sizeof(T) * size), size);
        }
        else {
            v.resize_discard(size);
            d.read((char*)v.data(), sizeof(T) * size);
        }
        read_align(d, sizeof(T) * size); // align
    }
};
template<>
struct read_impl<std::string>
{
    void operator()(deserializer& d, std::string& v) const
    {
        uint32_t size = 0;
        d.read((char*)&size, 4);
        v.resize(size);
        d.read((char*)v.data(), size);
        read_align(d, size); // align
    }
};
template<class T>
struct read_impl<std::vector<T>>
{
    void operator()(deserializer& d, std::vector<T>& v) const
    {
        uint32_t size = 0;
        d.read((char*)&size, 4);
        v.resize(size);
        for (auto& e : v)
            read_impl<T>()(d, e);
    }
};

template<class T> inline void write(serializer& s, T& v) { return write_impl<T>()(s, v); }
template<class T> inline void read(deserializer& d, T& v) { return read_impl<T>()(d, v); }

} // namespace sg

#define sgDeclPtr(T) using T##Ptr = std::shared_ptr<T>
#define sgSerializable(T) template<> struct serializable<T> { static const bool value = true; };

#define sgSerialize(V) sg::write(s, V);
#define sgDeserialize(V) sg::read(d, V);
