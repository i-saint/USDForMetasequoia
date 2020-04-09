#pragma once
#include "../mqusd.h"

namespace mqusd {

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



template<class Body>
inline void MQEachObject(MQDocument doc, const Body& body)
{
    int n = doc->GetObjectCount();
    for (int i = 0; i < n; ++i) {
        if (auto obj = doc->GetObject(i))
            invoke(body, obj, i);
    }
}

template<class Body>
inline void MQEachMaterial(MQDocument doc, const Body& body)
{
    int n = doc->GetMaterialCount();
    for (int i = 0; i < n; ++i) {
        if (auto obj = doc->GetMaterial(i))
            invoke(body, obj, i);
    }
}

std::string MQGetName(MQObject obj);
std::string MQGetPath(MQDocument doc, MQObject obj);
std::string MQGetName(MQMaterial obj);

void MQSetName(MQObject obj, const std::string& name);
void MQSetName(MQMaterial obj, const std::string& name);

} // namespace mqusd
