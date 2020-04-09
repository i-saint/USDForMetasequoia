#include "pch.h"
#include "mqUtils.h"

namespace mqusd {

std::string MQGetName(MQObject obj)
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

std::string MQGetPath(MQDocument doc, MQObject obj)
{
    std::string ret;
    if (auto parent = doc->GetParentObject(obj))
        ret += MQGetPath(doc, parent);
    ret += '/';
    ret += MQGetName(obj);
    return ret;
}

std::string MQGetName(MQMaterial obj)
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

void MQSetName(MQObject obj, const std::string& name)
{
#if MQPLUGIN_VERSION >= 0x0470
    obj->SetName(mu::ToWCS(name).c_str());
#else
    obj->SetName(name.c_str());
#endif
}
void MQSetName(MQMaterial obj, const std::string& name)
{
#if MQPLUGIN_VERSION >= 0x0470
    obj->SetName(mu::ToWCS(name).c_str());
#else
    obj->SetName(name.c_str());
#endif
}

} // namespace mqusd
