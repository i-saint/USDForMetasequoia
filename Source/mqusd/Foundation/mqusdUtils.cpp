#include "pch.h"
#include "mqusdUtils.h"

namespace mqusd {

std::string SanitizeNodeName(const std::string& name)
{
    std::string ret = name;
    for (auto& c : ret) {
        if (!std::isalnum(c))
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
        if (!std::isalnum(c))
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

} // namespace mqusd
