#pragma once

namespace mqusd {

template<class T>
inline bool is_uniform(const T* data, size_t data_size, size_t element_size)
{
    auto* base = data;
    size_t n = data_size / element_size;
    for (size_t di = 1; di < n; ++di) {
        data += element_size;
        for (size_t ei = 0; ei < element_size; ++ei) {
            if (data[ei] != base[ei])
                return false;
        }
    }
    return true;
}
template<class Container, class Body>
inline bool is_uniform(const Container& container, const Body& body)
{
    for (auto& e : container) {
        if (!body(e))
            return false;
    }
    return true;
}
template<class T>
inline void expand_uniform(T* dst, size_t data_size, const T* src, size_t element_size)
{
    size_t n = data_size / element_size;
    for (size_t di = 0; di < n; ++di) {
        for (size_t ei = 0; ei < element_size; ++ei)
            dst[ei] = src[ei];
        dst += element_size;
    }
}

template<class DstContainer, class SrcContainer, class Convert>
inline void transform_container(DstContainer& dst, const SrcContainer& src, const Convert& c)
{
    size_t n = src.size();
    dst.resize(n);
    for (size_t i = 0; i < n; ++i)
        c(dst[i], src[i]);
}

template<class Container, class Body>
inline void each_with_index(Container& dst, const Body& body)
{
    int i = 0;
    for (auto& v : dst)
        body(v, i++);
}

template<class Container, class Condition>
inline void erase_if(Container& dst, const Condition& cond)
{
    dst.erase(
        std::remove_if(dst.begin(), dst.end(), cond),
        dst.end());
}

template<class Container>
inline auto append(Container& dst, const Container& src)
{
    size_t pos = dst.size();
    if (dst.empty())
        dst = src; // share if Container is SharedVector
    else
        dst.insert(dst.end(), src.begin(), src.end());
    return dst.begin() + pos;
}

template<class T>
inline void fill(T* dst, size_t size, T v)
{
    for (size_t i = 0; i < size; ++i)
        *(dst++) = v;
}
template<class Container, class T>
inline void fill(Container& dst, T v)
{
    fill(dst.data(), dst.size(), v);
}

template<class T>
inline void add(T* dst, size_t size, T v)
{
    for (size_t i = 0; i < size; ++i)
        *(dst++) += v;
}

template<class Container, class Getter>
inline auto get_max(const Container& src, const Getter& getter)
{
    size_t n = src.size();
    auto r = getter(src.front());
    for (size_t i = 1; i < n; ++i)
        r = std::max(r, getter(src[i]));
    return r;
}

std::string SanitizeNodeName(const std::string& name);
std::string SanitizeNodePath(const std::string& path);
std::string GetParentPath(const std::string& path);
const char* GetLeafName(const std::string& path);

} // namespace mqusd

#define mqusdDbgPrint(...)
#define mqusdDbgFatal(...)
