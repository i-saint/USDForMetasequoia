#pragma once
#include "mqusd.h"

namespace mqusd {

#ifdef  MQPluginH

inline float3 to_float3(const MQColor& v) { return (float3&)v; }
inline float3 to_float3(const MQPoint& v) { return (float3&)v; }
inline float4x4 to_float4x4(const MQMatrix& v) { return (float4x4&)v; }
inline MQColor to_color(const float3& v) { return (MQColor&)v; }
inline MQPoint to_point(const float3& v) { return (MQPoint&)v; }

inline float3 to_eular(const MQAngle& ang, bool flip_head = false)
{
    auto ret = flip_head ?
        float3{ ang.pitch, -ang.head + 180.0f,  ang.bank } :
        float3{ ang.pitch, ang.head, ang.bank };
    return ret * mu::DegToRad;
}
inline quatf to_quat(const MQAngle& ang)
{
    return mu::rotate_zxy(to_eular(ang));
}
inline MQAngle to_angle(const quatf& q)
{
    auto r = mu::to_euler_zxy(q);
    r = float3{ r.y, r.x, r.z } * mu::RadToDeg;
    return (MQAngle&)r;
}

#endif // MQPluginH


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

template<class DstContainer, class SrcContainer, class Convert>
inline void transform_container(DstContainer& dst, const SrcContainer& src, const Convert& c)
{
    size_t n = src.size();
    dst.resize(n);
    for (size_t i = 0; i < n; ++i)
        c(dst[i], src[i]);
}

std::string SanitizeNodeName(const std::string& name);
std::string SanitizeNodePath(const std::string& path);
std::string GetParentPath(const std::string& path);
const char* GetLeafName(const std::string& path);


} // namespace mqusd
