#pragma once

#define sgEnableIf(...) std::enable_if_t<__VA_ARGS__, bool> = true

#define sgDbgPrint(...)
#define sgDbgFatal(...)

namespace sg {


namespace detail
{
    template<class R, class... Args>
    struct lambda_traits_impl
    {
        using return_type = R;
        enum { arity = sizeof...(Args) };

        template<size_t i> struct arg
        {
            using type = typename std::tuple_element<i, std::tuple<Args...>>::type;
        };
    };
}

template<class L>
struct lambda_traits : lambda_traits<decltype(&L::operator())>
{};

template<class R, class T, class... Args>
struct lambda_traits<R(T::*)(Args...)> : detail::lambda_traits_impl<R, Args...>
{};

template<class R, class T, class... Args>
struct lambda_traits<R(T::*)(Args...) const> : detail::lambda_traits_impl<R, Args...>
{};


template<class Body, class A1, class A2, sgEnableIf(lambda_traits<Body>::arity == 1)>
inline auto invoke(const Body& body, A1& a1, A2&) { return body(a1); }

template<class Body, class A1, class A2, sgEnableIf(lambda_traits<Body>::arity == 2)>
inline auto invoke(const Body& body, A1& a1, A2& a2) { return body(a1, a2); }



template<class Body, class A1, sgEnableIf(std::is_same<typename lambda_traits<Body>::return_type, bool>::value)>
inline bool invoke_false(const Body& body, A1& a1)
{
    return body(a1) == false;
}
template<class Body, class A1, sgEnableIf(!std::is_same<typename lambda_traits<Body>::return_type, bool>::value)>
inline bool invoke_false(const Body& body, A1& a1)
{
    body(a1);
    return false;
}

template<class Body, class A1, class A2, sgEnableIf(std::is_same<typename lambda_traits<Body>::return_type, bool>::value)>
inline bool invoke_false(const Body& body, A1& a1, A2& a2)
{
    return invoke(body, a1, a2) == false;
}
template<class Body, class A1, class A2, sgEnableIf(!std::is_same<typename lambda_traits<Body>::return_type, bool>::value)>
inline bool invoke_false(const Body& body, A1& a1, A2& a2)
{
    invoke(body, a1, a2);
    return false;
}

template<class Container, class Body>
inline void each(Container& dst, const Body& body)
{
    for (auto& v : dst)
        body(v);
}

template<class Container, class Body>
inline void each_with_index(Container& dst, const Body& body)
{
    int i = 0;
    for (auto& v : dst)
        body(v, i++);
}


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

} // namespace sg
