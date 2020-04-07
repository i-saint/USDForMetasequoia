#pragma once
#include "sgusdInternal.h"

namespace sg {

enum PrintFlags
{
    PF_Path = 0x01,
    PF_Attr = 0x02,
    PF_Meta = 0x04,
    PF_Rel  = 0x08,

    PF_NonPath = 0x0e,
    PF_Full = 0x0f,
};

void PrintPrim(UsdPrim prim, PrintFlags flags = PF_Path);

template<class Body>
inline void EachChild(UsdPrim prim, const Body& body)
{
    for (auto child : prim.GetAllChildren())
        body(child);
}

template<class NodeT>
inline NodeT* CreateNode(USDNode* parent, UsdPrim prim)
{
    return new NodeT(
        parent ? parent->m_node : nullptr,
        prim.GetName().GetText());
}

template<class T> inline void GetValue(const T& src, float& v, UsdTimeCode t = { default_time })
{
    if (src)
        src.Get(&v, t);
}
template<class T> inline void GetValue(const T& src, float2& v, UsdTimeCode t = { default_time })
{
    if (src)
        src.Get((GfVec2f*)&v, t);
}
template<class T> inline void GetValue(const T& src, float3& v, UsdTimeCode t = { default_time })
{
    if (src)
        src.Get((GfVec3f*)&v, t);
}
template<class T> inline void GetValue(const T& src, float4& v, UsdTimeCode t = { default_time })
{
    if (src)
        src.Get((GfVec4f*)&v, t);
}
template<class T> inline void GetValue(const T& src, bool& v, UsdTimeCode t = { default_time })
{
    if (src) {
        int tmp = 0;
        if (src.Get(&tmp, t))
            v = tmp != 0;
    }
}
template<class T> inline void GetValue(const T& src, std::string& v, UsdTimeCode t = { default_time })
{
    if (src) {
        if (src.GetTypeName() == SdfValueTypeNames->Token) {
            TfToken tmp;
            src.Get(&tmp, t);
            v = tmp.GetString();
        }
        else if (src.GetTypeName() == SdfValueTypeNames->Asset) {
            SdfAssetPath tmp;
            src.Get(&tmp, t);
            v = tmp.GetAssetPath();
        }
    }
}

inline ShaderType ToShaderType(const std::string& v)
{
    if (v == sgusdShaderMQClassic)
        return ShaderType::MQClassic;
    else if (v == sgusdShaderMQConstant)
        return ShaderType::MQConstant;
    else if (v == sgusdShaderMQLambert)
        return ShaderType::MQLambert;
    else if (v == sgusdShaderMQPhong)
        return ShaderType::MQPhong;
    else if (v == sgusdShaderMQBlinn)
        return ShaderType::MQBlinn;
    else if (v == sgusdShaderMQHLSL)
        return ShaderType::MQHLSL;
    return ShaderType::Unknown;
}
template<class T> inline void GetValue(const T& src, ShaderType& v, UsdTimeCode t = { default_time })
{
    if (src) {
        TfToken tmp;
        src.Get(&tmp, t);
        v = ToShaderType(tmp.GetString());
    }
}

inline WrapMode ToWrapMode(const std::string& v)
{
    if (v == sgusdWrapClamp)
        return WrapMode::Clamp;
    else if (v == sgusdWrapRepeat)
        return WrapMode::Repeat;
    else if (v == sgusdWrapMirror)
        return WrapMode::Mirror;
    else if (v == sgusdWrapBlack)
        return WrapMode::Black;
    return WrapMode::Clamp;
}
template<class T> inline void GetValue(const T& src, WrapMode& v, UsdTimeCode t = { default_time })
{
    if (src) {
        TfToken tmp;
        src.Get(&tmp, t);
        v = ToWrapMode(tmp.GetString());
    }
}

template<class T> inline void SetValue(const T& dst, const float& v, UsdTimeCode t = { default_time })
{
    if (dst)
        dst.Set(v, t);
}
template<class T> inline void SetValue(const T& dst, const float2& v, UsdTimeCode t = { default_time })
{
    if (dst)
        dst.Set((GfVec2f&)v, t);
}
template<class T> inline void SetValue(const T& dst, const float3& v, UsdTimeCode t = { default_time })
{
    if (dst)
        dst.Set((GfVec3f&)v, t);
}
template<class T> inline void SetValue(const T& dst, const float4& v, UsdTimeCode t = { default_time })
{
    if (dst)
        dst.Set((GfVec4f&)v, t);
}
template<class T> inline void SetValue(const T& dst, const bool& v, UsdTimeCode t = { default_time })
{
    if (dst) {
        int tmp = v ? 1 : 0;
        dst.Set(tmp, t);
    }
}
template<class T> inline void SetValue(const T& dst, const std::string& v, UsdTimeCode t = { default_time })
{
    if (dst) {
        if (dst.GetTypeName() == SdfValueTypeNames->Token)
            dst.Set(TfToken(v), t);
        else if (dst.GetTypeName() == SdfValueTypeNames->Asset)
            dst.Set(SdfAssetPath(v), t);
    }
}

inline std::string ToString(ShaderType v)
{
    switch (v) {
    case ShaderType::MQClassic: return sgusdShaderMQClassic;
    case ShaderType::MQConstant:return sgusdShaderMQConstant;
    case ShaderType::MQLambert: return sgusdShaderMQLambert;
    case ShaderType::MQPhong:   return sgusdShaderMQPhong;
    case ShaderType::MQBlinn:   return sgusdShaderMQBlinn;
    case ShaderType::MQHLSL:    return sgusdShaderMQHLSL;
    default: return "";
    }
}
template<class T> inline void SetValue(const T& dst, const ShaderType& v, UsdTimeCode t = { default_time })
{
    if (dst)
        dst.Set(TfToken(ToString(v)), t);
}

inline std::string ToString(WrapMode v)
{
    switch (v) {
    case WrapMode::Clamp:   return sgusdWrapClamp;
    case WrapMode::Repeat:  return sgusdWrapRepeat;
    case WrapMode::Mirror:  return sgusdWrapMirror;
    default: return "";
    }
}
template<class T> inline void SetValue(const T& dst, const WrapMode& v, UsdTimeCode t = { default_time })
{
    if (dst)
        dst.Set(TfToken(ToString(v)), t);
}


} // namespace sg
