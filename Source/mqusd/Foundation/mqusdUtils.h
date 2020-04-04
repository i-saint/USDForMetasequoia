#pragma once
#include "mqusd.h"

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


namespace detail {

template<class Body, class R>
struct each_object
{
    void operator()(MQDocument doc, const Body& body)
    {
        int n = doc->GetObjectCount();
        for (int i = 0; i < n; ++i) {
            if (auto obj = doc->GetObject(i))
                body(obj);
        }
    }
};

template<class Body>
struct each_object<Body, bool>
{
    void operator()(MQDocument doc, const Body& body)
    {
        int n = doc->GetObjectCount();
        for (int i = 0; i < n; ++i) {
            if (auto obj = doc->GetObject(i)) {
                if (!body(obj))
                    break;
            }
        }
    }
};

template<class Body, class R>
struct each_material
{
    void operator()(MQDocument doc, const Body& body)
    {
        int n = doc->GetMaterialCount();
        for (int i = 0; i < n; ++i) {
            if (auto obj = doc->GetMaterial(i))
                body(obj);
        }
    }
};

template<class Body>
struct each_material<Body, bool>
{
    void operator()(MQDocument doc, const Body& body)
    {
        int n = doc->GetMaterialCount();
        for (int i = 0; i < n; ++i) {
            if (auto obj = doc->GetMaterial(i)) {
                if (!body(obj))
                    break;
            }
        }
    }
};

} // namespace detail

template<class Body>
inline void each_object(MQDocument doc, const Body& body)
{
    detail::each_object<Body, decltype(body(nullptr))>()(doc, body);
}

template<class Body>
inline void each_material(MQDocument doc, const Body& body)
{
    detail::each_material<Body, decltype(body(nullptr))>()(doc, body);
}

inline std::string GetName(MQObject obj)
{
    char buf[256] = "";
    obj->GetName(buf, sizeof(buf));
    return buf;
}

inline std::string GetPath(MQDocument doc, MQObject obj)
{
    std::string ret;
    if (auto parent = doc->GetParentObject(obj))
        ret += GetPath(doc, parent);
    ret += '/';

    char buf[256] = "";
    obj->GetName(buf, sizeof(buf));
    ret += buf;
    return ret;
}

inline std::string GetName(MQMaterial obj)
{
    char buf[256] = "";
    obj->GetName(buf, sizeof(buf));
    return buf;
}

} // namespace mqusd
