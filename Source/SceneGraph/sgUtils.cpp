#include "pch.h"
#include "SceneGraph.h"
#include "sgUtils.h"

namespace sg {

std::string SanitizeNodeName(const std::string& name)
{
    std::string ret = name;

    // USD allows only alphabet, digit and '_' for node name.
    // in addition, the first character must not be a digit.

    if (ret.empty() || std::isdigit(ret.front()))
        ret = "_" + ret;

    for (auto& c : ret) {
        if (!std::isalnum(c) && c != '_')
            c = '_';
    }

    return ret;
}

std::string SanitizeNodePath(const std::string& path)
{
    std::string ret = path;
    for (auto& c : ret) {
        if (c == '/')
            continue;
        if (!std::isalnum(c) && c != '_')
            c = '_';
    }
    return ret;
}

std::string GetParentPath(const std::string& path)
{
    auto pos = path.find_last_of('/');
    if (pos == std::string::npos || (pos == 0 && path.size() == 1))
        return "";
    else if (pos == 0)
        return "/";
    else
        return std::string(path.begin(), path.begin() + pos);
}

const char* GetLeafName(const std::string& path)
{
    auto pos = path.find_last_of('/');
    if (pos == std::string::npos)
        return path.c_str();
    else
        return path.c_str() + (pos + 1);
}


#define sgUnknown           "unknown"
#define sgShaderMQClassic   "mq:classic"
#define sgShaderMQConstant  "mq:constant"
#define sgShaderMQLambert   "mq:lambert"
#define sgShaderMQPhong     "mq:phong"
#define sgShaderMQBlinn     "mq:blinn"
#define sgShaderMQHLSL      "mq:hlsl"

#define sgWrapBlack         "black"
#define sgWrapClamp         "clamp"
#define sgWrapRepeat        "repeat"
#define sgWrapMirror        "mirror"

std::string ToString(ShaderType v)
{
    switch (v) {
    case ShaderType::MQClassic: return sgShaderMQClassic;
    case ShaderType::MQConstant:return sgShaderMQConstant;
    case ShaderType::MQLambert: return sgShaderMQLambert;
    case ShaderType::MQPhong:   return sgShaderMQPhong;
    case ShaderType::MQBlinn:   return sgShaderMQBlinn;
    case ShaderType::MQHLSL:    return sgShaderMQHLSL;
    default: return sgUnknown;
    }
}

std::string ToString(WrapMode v)
{
    switch (v) {
    case WrapMode::Clamp:   return sgWrapClamp;
    case WrapMode::Repeat:  return sgWrapRepeat;
    case WrapMode::Mirror:  return sgWrapMirror;
    default: return sgUnknown;
    }
}

ShaderType ToShaderType(const std::string& v)
{
    if (v == sgShaderMQClassic)
        return ShaderType::MQClassic;
    else if (v == sgShaderMQConstant)
        return ShaderType::MQConstant;
    else if (v == sgShaderMQLambert)
        return ShaderType::MQLambert;
    else if (v == sgShaderMQPhong)
        return ShaderType::MQPhong;
    else if (v == sgShaderMQBlinn)
        return ShaderType::MQBlinn;
    else if (v == sgShaderMQHLSL)
        return ShaderType::MQHLSL;
    return ShaderType::Unknown;
}

WrapMode ToWrapMode(const std::string& v)
{
    if (v == sgWrapClamp)
        return WrapMode::Clamp;
    else if (v == sgWrapRepeat)
        return WrapMode::Repeat;
    else if (v == sgWrapMirror)
        return WrapMode::Mirror;
    else if (v == sgWrapBlack)
        return WrapMode::Black;
    return WrapMode::Clamp;
}

} // namespace sg
