#pragma once

namespace mqusd {

using serializer = std::ostream;
using deserializer = std::istream;

template<class T> struct serializable { static const bool value = false; };

template<class T, bool hs = serializable<T>::value> struct write_impl2;
template<class T> struct write_impl2<T, true>
{
    void operator()(serializer& os, T& v) const { v.serialize(os); }
};
template<class T> struct write_impl2<T, false>
{
    void operator()(serializer& os, T& v) const
    {
        static_assert(sizeof(T) % 4 == 0, "");
        os.write((const char*)&v, sizeof(T));
    }
};

template<class T, bool hs = serializable<T>::value> struct read_impl2;
template<class T> struct read_impl2<T, true> {
    void operator()(deserializer& is, T& v) const { v.deserialize(is); }
};
template<class T> struct read_impl2<T, false> {
    void operator()(deserializer& is, T& v) const { is.read((char*)&v, sizeof(T)); }
};


template<class T>
struct write_impl
{
    void operator()(serializer& os, T& v) const
    {
        write_impl2<T>()(os, v);
    }
};
template<class T>
struct read_impl
{
    void operator()(deserializer& is, T& v) const
    {
        read_impl2<T>()(is, v);
    }
};


inline void write_align(serializer& os, size_t written_size)
{
    const int zero = 0;
    if (written_size % 4 != 0)
        os.write((const char*)&zero, 4 - (written_size % 4));
}

template<>
struct write_impl<bool>
{
    void operator()(serializer& os, const bool& v) const
    {
        os.write((const char*)&v, sizeof(v));
        write_align(os, sizeof(v)); // keep 4 byte alignment
    }
};
template<class T>
struct write_impl<std::shared_ptr<T>>
{
    void operator()(serializer& os, const std::shared_ptr<T>& v) const
    {
        v->serialize(os);
    }
};
template<class T>
struct write_impl<SharedVector<T>>
{
    void operator()(serializer& os, const SharedVector<T>& v) const
    {
        auto size = (uint32_t)v.size();
        os.write((const char*)&size, sizeof(size));
        os.write((const char*)v.cdata(), sizeof(T) * size);
        write_align(os, sizeof(T) * size); // keep 4 byte alignment
    }
};
template<>
struct write_impl<std::string>
{
    void operator()(serializer& os, const std::string& v) const
    {
        auto size = (uint32_t)v.size();
        os.write((const char*)&size, 4);
        os.write((const char*)v.data(), size);
        write_align(os, size); // keep 4 byte alignment
    }
};
template<class T>
struct write_impl<std::vector<T>>
{
    void operator()(serializer& os, const std::vector<T>& v) const
    {
        auto size = (uint32_t)v.size();
        os.write((const char*)&size, 4);
        for (const auto& e : v)
            write_impl<T>()(os, e);
    }
};



inline void read_align(deserializer& is, size_t read_size)
{
    int dummy = 0;
    if (read_size % 4 != 0)
        is.read((char*)&dummy, 4 - (read_size % 4));
}

template<>
struct read_impl<bool>
{
    void operator()(deserializer& is, bool& v) const
    {
        is.read((char*)&v, sizeof(v));
        read_align(is, sizeof(v)); // align
    }
};
template<class T>
struct read_impl<std::shared_ptr<T>>
{
    void operator()(deserializer& is, std::shared_ptr<T>& v) const
    {
        T::deserialize(is, v);
    }
};
template<class T>
struct read_impl<SharedVector<T>>
{
    void operator()(deserializer& is, SharedVector<T>& v) const
    {
        uint32_t size = 0;
        is.read((char*)&size, sizeof(size));
        if (typeid(is) == typeid(mu::MemoryStream)) {
            // just share buffer (no copy)
            auto& ms = static_cast<mu::MemoryStream&>(is);
            v.share((T*)ms.gskip(sizeof(T) * size), size);
        }
        else {
            v.resize_discard(size);
            is.read((char*)v.data(), sizeof(T) * size);
        }
        read_align(is, sizeof(T) * size); // align
    }
};
template<>
struct read_impl<std::string>
{
    void operator()(deserializer& is, std::string& v) const
    {
        uint32_t size = 0;
        is.read((char*)&size, 4);
        v.resize(size);
        is.read((char*)v.data(), size);
        read_align(is, size); // align
    }
};
template<class T>
struct read_impl<std::vector<T>>
{
    void operator()(deserializer& is, std::vector<T>& v) const
    {
        uint32_t size = 0;
        is.read((char*)&size, 4);
        v.resize(size);
        for (auto& e : v)
            read_impl<T>()(is, e);
    }
};

template<class T> inline void write(serializer& os, T& v) { return write_impl<T>()(os, v); }
template<class T> inline void read(deserializer& is, T& v) { return read_impl<T>()(is, v); }

} // namespace mqusd

#define mqusdDeclPtr(T) using T##Ptr = std::shared_ptr<T>
#define mqusdSerializable(T) template<> struct serializable<T> { static const bool value = true; };
