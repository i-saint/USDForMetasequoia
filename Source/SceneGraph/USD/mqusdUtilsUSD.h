#pragma once

namespace mqusd {

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

template<class T> inline void GetValue(T& src, float& v, UsdTimeCode t = { default_time })
{
    if (src)
        src.Get(&v, t);
}
template<class T> inline void GetValue(T& src, float2& v, UsdTimeCode t = { default_time })
{
    if (src)
        src.Get((GfVec2f*)&v, t);
}
template<class T> inline void GetValue(T& src, float3& v, UsdTimeCode t = { default_time })
{
    if (src)
        src.Get((GfVec3f*)&v, t);
}
template<class T> inline void GetValue(T& src, float4& v, UsdTimeCode t = { default_time })
{
    if (src)
        src.Get((GfVec4f*)&v, t);
}
template<class T> inline void GetValue(T& src, bool& v, UsdTimeCode t = { default_time })
{
    if (src) {
        int tmp = 0;
        if (src.Get(&tmp, t))
            v = tmp != 0;
    }
}
template<class T> inline void GetValue(T& src, std::string& v, UsdTimeCode t = { default_time })
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
    if (v == mqusdShaderMQClassic)
        return ShaderType::MQClassic;
    else if (v == mqusdShaderMQConstant)
        return ShaderType::MQConstant;
    else if (v == mqusdShaderMQLambert)
        return ShaderType::MQLambert;
    else if (v == mqusdShaderMQPhong)
        return ShaderType::MQPhong;
    else if (v == mqusdShaderMQBlinn)
        return ShaderType::MQBlinn;
    else if (v == mqusdShaderMQHLSL)
        return ShaderType::MQHLSL;
    return ShaderType::Unknown;
}
template<class T> inline void GetValue(T& src, ShaderType& v, UsdTimeCode t = { default_time })
{
    if (src) {
        TfToken tmp;
        src.Get(&tmp, t);
        v = ToShaderType(tmp.GetString());
    }
}

inline WrapMode ToWrapMode(const std::string& v)
{
    if (v == mqusdWrapClamp)
        return WrapMode::Clamp;
    else if (v == mqusdWrapRepeat)
        return WrapMode::Repeat;
    else if (v == mqusdWrapMirror)
        return WrapMode::Mirror;
    else if (v == mqusdWrapBlack)
        return WrapMode::Black;
    return WrapMode::Clamp;
}
template<class T> inline void GetValue(T& src, WrapMode& v, UsdTimeCode t = { default_time })
{
    if (src) {
        TfToken tmp;
        src.Get(&tmp, t);
        v = ToWrapMode(tmp.GetString());
    }
}

template<class T> inline void SetValue(T& dst, float v, UsdTimeCode t = { default_time })
{
    if (dst)
        dst.Set(v, t);
}
template<class T> inline void SetValue(T& dst, float2 v, UsdTimeCode t = { default_time })
{
    if (dst)
        dst.Set((GfVec2f&)v, t);
}
template<class T> inline void SetValue(T& dst, float3 v, UsdTimeCode t = { default_time })
{
    if (dst)
        dst.Set((GfVec3f&)v, t);
}
template<class T> inline void SetValue(T& dst, float4 v, UsdTimeCode t = { default_time })
{
    if (dst)
        dst.Set((GfVec4f&)v, t);
}
template<class T> inline void SetValue(T& dst, bool v, UsdTimeCode t = { default_time })
{
    if (dst) {
        int tmp = v ? 1 : 0;
        dst.Set(tmp, t);
    }
}
template<class T> inline void SetValue(T& dst, const std::string& v, UsdTimeCode t = { default_time })
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
    case ShaderType::MQClassic: return mqusdShaderMQClassic;
    case ShaderType::MQConstant:return mqusdShaderMQConstant;
    case ShaderType::MQLambert: return mqusdShaderMQLambert;
    case ShaderType::MQPhong:   return mqusdShaderMQPhong;
    case ShaderType::MQBlinn:   return mqusdShaderMQBlinn;
    case ShaderType::MQHLSL:    return mqusdShaderMQHLSL;
    default: return "";
    }
}
template<class T> inline void SetValue(T& dst, ShaderType v, UsdTimeCode t = { default_time })
{
    if (dst)
        dst.Set(TfToken(ToString(v)), t);
}

inline std::string ToString(WrapMode v)
{
    switch (v) {
    case WrapMode::Clamp:   return mqusdWrapClamp;
    case WrapMode::Repeat:  return mqusdWrapRepeat;
    case WrapMode::Mirror:  return mqusdWrapMirror;
    default: return "";
    }
}
template<class T> inline void SetValue(T& dst, WrapMode v, UsdTimeCode t = { default_time })
{
    if (dst)
        dst.Set(TfToken(ToString(v)), t);
}


} // namespace mqusd
