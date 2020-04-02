#pragma once
#include "muMath.h"

namespace mu {

template<class T> struct limits_impl;

// note: inf() & nan() are functions because making it constants cause link error on gcc.
template<> struct limits_impl<float>
{
    static constexpr float inf() { return std::numeric_limits<float>::infinity(); }
    static constexpr float nan() { return std::numeric_limits<float>::quiet_NaN(); }
    static constexpr float min() { return std::numeric_limits<float>::min(); }
    static constexpr float max() { return std::numeric_limits<float>::max(); }
    static constexpr float epsilon() { return std::numeric_limits<float>::epsilon(); }
    static bool isinf(const float& v) { return std::isinf(v); }
    static bool isnan(const float& v) { return std::isnan(v); }
};

template<> struct limits_impl<double>
{
    static constexpr double inf() { return std::numeric_limits<double>::infinity(); }
    static constexpr double nan() { return std::numeric_limits<double>::quiet_NaN(); }
    static constexpr double min() { return std::numeric_limits<double>::min(); }
    static constexpr double max() { return std::numeric_limits<double>::max(); }
    static constexpr double epsilon() { return std::numeric_limits<double>::epsilon(); }
    static bool isinf(const double& v) { return std::isinf(v); }
    static bool isnan(const double& v) { return std::isnan(v); }
};

template<class T> struct limits_impl<tvec2<T>>
{
    using impl = limits_impl<T>;
    static constexpr tvec2<T> inf() { return { impl::inf(), impl::inf() }; }
    static constexpr tvec2<T> nan() { return { impl::nan(), impl::nan() }; }
    static constexpr tvec2<T> min() { return { impl::min(), impl::min() }; }
    static constexpr tvec2<T> max() { return { impl::max(), impl::max() }; }
    static constexpr tvec2<T> epsilon() { return { impl::epsilon(), impl::epsilon() }; }
    static bool isinf(const tvec2<T>& v) { return impl::isinf(v.x) || impl::isinf(v.y); }
    static bool isnan(const tvec2<T>& v) { return impl::isnan(v.x) || impl::isnan(v.y); }
};

template<class T> struct limits_impl<tvec3<T>>
{
    using impl = limits_impl<T>;
    static constexpr tvec3<T> inf() { return { impl::inf(), impl::inf(), impl::inf() }; }
    static constexpr tvec3<T> nan() { return { impl::nan(), impl::nan(), impl::nan() }; }
    static constexpr tvec3<T> min() { return { impl::min(), impl::min(), impl::min() }; }
    static constexpr tvec3<T> max() { return { impl::max(), impl::max(), impl::max() }; }
    static constexpr tvec3<T> epsilon() { return { impl::epsilon(), impl::epsilon(), impl::epsilon() }; };
    static bool isinf(const tvec3<T>& v) { return impl::isinf(v.x) || impl::isinf(v.y) || impl::isinf(v.z); }
    static bool isnan(const tvec3<T>& v) { return impl::isnan(v.x) || impl::isnan(v.y) || impl::isnan(v.z); }
};

template<class T> struct limits_impl<tvec4<T>>
{
    using impl = limits_impl<T>;
    static constexpr tvec4<T> inf() { return { impl::inf(), impl::inf(), impl::inf(), impl::inf() }; }
    static constexpr tvec4<T> nan() { return { impl::nan(), impl::nan(), impl::nan(), impl::nan() }; }
    static constexpr tvec4<T> min() { return { impl::min(), impl::min(), impl::min(), impl::min() }; }
    static constexpr tvec4<T> max() { return { impl::max(), impl::max(), impl::max(), impl::max() }; }
    static constexpr tvec4<T> epsilon() { return { impl::epsilon(), impl::epsilon(), impl::epsilon(), impl::epsilon() }; };
    static bool isinf(const tvec4<T>& v) { return impl::isinf(v.x) || impl::isinf(v.y) || impl::isinf(v.z) || impl::isinf(v.w); }
    static bool isnan(const tvec4<T>& v) { return impl::isnan(v.x) || impl::isnan(v.y) || impl::isnan(v.z) || impl::isnan(v.w); }
};

template<class T> struct limits_impl<tquat<T>>
{
    using impl = limits_impl<T>;
    static constexpr tquat<T> inf() { return { impl::inf(), impl::inf(), impl::inf(), impl::inf() }; }
    static constexpr tquat<T> nan() { return { impl::nan(), impl::nan(), impl::nan(), impl::nan() }; }
    static constexpr tquat<T> min() { return { impl::min(), impl::min(), impl::min(), impl::min() }; }
    static constexpr tquat<T> max() { return { impl::max(), impl::max(), impl::max(), impl::max() }; }
    static constexpr tquat<T> epsilon() { return { impl::epsilon(), impl::epsilon(), impl::epsilon(), impl::epsilon() }; }
    static bool isinf(const tquat<T>& v) { return impl::isinf(v.x) || impl::isinf(v.y) || impl::isinf(v.z) || impl::isinf(v.w); }
    static bool isnan(const tquat<T>& v) { return impl::isnan(v.x) || impl::isnan(v.y) || impl::isnan(v.z) || impl::isnan(v.w); }
};

template<class T> struct limits_impl<tmat2x2<T>>
{
    using impl = limits_impl<tvec2<T>>;
    static constexpr tmat2x2<T> inf() { return { { impl::inf(), impl::inf() } }; }
    static constexpr tmat2x2<T> nan() { return { { impl::nan(), impl::nan() } }; }
    static constexpr tmat2x2<T> min() { return { { impl::min(), impl::min() } }; }
    static constexpr tmat2x2<T> max() { return { { impl::max(), impl::max() } }; }
    static constexpr tmat2x2<T> epsilon() { return { { impl::epsilon(), impl::epsilon() } }; }
    static bool isinf(const tmat2x2<T>& v) { return impl::isinf(v[0]) || impl::isinf(v[1]); }
    static bool isnan(const tmat2x2<T>& v) { return impl::isnan(v[0]) || impl::isnan(v[1]); }
};

template<class T> struct limits_impl<tmat3x3<T>>
{
    using impl = limits_impl<tvec3<T>>;
    static constexpr tmat3x3<T> inf() { return { { impl::inf(), impl::inf(), impl::inf() } }; }
    static constexpr tmat3x3<T> nan() { return { { impl::nan(), impl::nan(), impl::nan() } }; }
    static constexpr tmat3x3<T> min() { return { { impl::min(), impl::min(), impl::min() } }; }
    static constexpr tmat3x3<T> max() { return { { impl::max(), impl::max(), impl::max() } }; }
    static constexpr tmat3x3<T> epsilon() { return { { impl::epsilon(), impl::epsilon(), impl::epsilon() } }; }
    static bool isinf(const tmat3x3<T>& v) { return impl::isinf(v[0]) || impl::isinf(v[1]) || impl::isinf(v[2]); }
    static bool isnan(const tmat3x3<T>& v) { return impl::isnan(v[0]) || impl::isnan(v[1]) || impl::isnan(v[2]); }
};

template<class T> struct limits_impl<tmat4x4<T>>
{
    using impl = limits_impl<tvec4<T>>;
    static constexpr tmat4x4<T> inf() { return { { impl::inf(), impl::inf(), impl::inf(), impl::inf() } }; }
    static constexpr tmat4x4<T> nan() { return { { impl::nan(), impl::nan(), impl::nan(), impl::nan() } }; }
    static constexpr tmat4x4<T> min() { return { { impl::min(), impl::min(), impl::min(), impl::min() } }; }
    static constexpr tmat4x4<T> max() { return { { impl::max(), impl::max(), impl::max(), impl::max() } }; }
    static constexpr tmat4x4<T> epsilon() { return { { impl::epsilon(), impl::epsilon(), impl::epsilon(), impl::epsilon() } }; }
    static bool isinf(const tmat4x4<T>& v) { return impl::isinf(v[0]) || impl::isinf(v[1]) || impl::isinf(v[2]) || impl::isinf(v[3]); }
    static bool isnan(const tmat4x4<T>& v) { return impl::isnan(v[0]) || impl::isnan(v[1]) || impl::isnan(v[2]) || impl::isnan(v[3]); }
};


template<class T> inline T inf() { return limits_impl<T>::inf(); }
template<class T> inline T nan() { return limits_impl<T>::nan(); }
template<class T> inline T min() { return limits_impl<T>::min(); }
template<class T> inline T max() { return limits_impl<T>::max(); }
template<class T> inline T epsilon() { return limits_impl<T>::epsilon(); }
template<class T> inline bool isinf(const T& v) { return limits_impl<T>::isinf(v); }
template<class T> inline bool isnan(const T& v) { return limits_impl<T>::isnan(v); }

} // namespace mu
