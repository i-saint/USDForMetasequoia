#include "pch.h"
#include "mqUtils.h"

namespace mqusd {

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

std::string GetName(MQObject obj)
{
#if MQPLUGIN_VERSION >= 0x0470
    wchar_t buf[256] = L"";
    obj->GetNameW(buf, sizeof(buf));
    return mu::ToMBS(buf);
#else
    char buf[256] = "";
    obj->GetName(buf, sizeof(buf));
    return buf;
#endif
}

std::string GetPath(MQDocument doc, MQObject obj)
{
    std::string ret;
    if (auto parent = doc->GetParentObject(obj))
        ret += GetPath(doc, parent);
    ret += '/';
    ret += GetName(obj);
    return ret;
}

std::string GetName(MQMaterial obj)
{
#if MQPLUGIN_VERSION >= 0x0470
    wchar_t buf[256] = L"";
    obj->GetNameW(buf, sizeof(buf));
    return mu::ToMBS(buf);
#else
    char buf[256] = "";
    obj->GetName(buf, sizeof(buf));
    return buf;
#endif
}

void SetName(MQObject obj, const std::string& name)
{
#if MQPLUGIN_VERSION >= 0x0470
    obj->SetName(mu::ToWCS(name).c_str());
#else
    obj->SetName(name.c_str());
#endif
}
void SetName(MQMaterial obj, const std::string& name)
{
#if MQPLUGIN_VERSION >= 0x0470
    obj->SetName(mu::ToWCS(name).c_str());
#else
    obj->SetName(name.c_str());
#endif
}

} // namespace mqusd
