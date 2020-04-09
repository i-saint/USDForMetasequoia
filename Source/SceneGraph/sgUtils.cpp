#include "pch.h"
#include "SceneGraph.h"
#include "sgUtils.h"

namespace sg {

std::string FromBinary(const std::string& v)
{
    size_t n = v.size() / 2;
    const char* buf = v.data();

    std::string r;
    for (size_t i = 0; i < n; ++i) {
        int c;
        sscanf(buf, "%02x", &c);
        r += (char)c;
        buf += 2;
    }
    return r;
}

std::string ToBinary(const std::string& v)
{
    char buf[4];
    size_t n = v.size();
    auto* c = (const byte*)v.data();

    std::string r;
    for (size_t i = 0; i < n; ++i) {
        sprintf(buf, "%02x", (int)*c);
        r += buf;
        ++c;
    }
    return r;
}


static void EncodeNodeNameImpl(std::string& dst, const char *src, size_t n)
{
    // USD allows only alphabet, digit and '_' for node name.
    // in addition, the first character must not be a digit.

    char buf[8];
    for (size_t i = 0; i < n; ++i) {
        char c = *src++;
        if (!std::isalnum(c) && c != '_') {
            sprintf(buf, "0x%02x", (int)(uint8_t&)c);
            dst += buf;
        }
        else {
            dst += c;
        }
    }
    if (dst.empty() || std::isdigit(dst.front()))
        dst = "_" + dst;
}

std::string EncodeNodeName(const std::string& v)
{
    std::string r;
    EncodeNodeNameImpl(r, v.data(), v.size());
    return r;
}

std::string EncodeNodePath(const std::string& v)
{
    std::string r;

    const char* s = v.data();
    for (;;) {
        if (*s == '/') {
            r += *s++;

            size_t n = 0;
            for (; ; ++n) {
                if (s[n] == '/' || s[n] == '\0')
                    break;
            }
            EncodeNodeNameImpl(r, s, n);
            s += n;
        }
        else
            break;
    }
    return r;
}


static void DecodeNodeNameImpl(std::string& dst, const char* s, size_t n)
{
    if (n >= 2) {
        // skip first '_'. see EncodeNodeNameImpl()
        if (s[0] == '_' && std::isdigit(s[1])) {
            ++s;
            --n;
        }
    }

    int c;
    for (;;) {
        if (n < 4) {
            dst.insert(dst.end(), s, s + n);
            break;
        }
        else if (sscanf(s, "0x%02x", &c) == 1) {
            dst += (char)c;
            s += 4;
            n -= 4;
        }
        else {
            dst += *s++;
            --n;
        }
    }
}

std::string DecodeNodeName(const std::string& v)
{
    std::string r;
    DecodeNodeNameImpl(r, v.data(), v.size());
    return r;
}

std::string DecodeNodePath(const std::string& v)
{
    std::string r;

    const char* s = v.data();
    for (;;) {
        if (*s == '/') {
            r += *s++;

            size_t n = 0;
            for (; ; ++n) {
                if (s[n] == '/' || s[n] == '\0')
                    break;
            }
            DecodeNodeNameImpl(r, s, n);
            s += n;
        }
        else
            break;
    }
    return r;
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
