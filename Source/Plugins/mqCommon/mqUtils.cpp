#include "mqUtils.h"
#include "pch.h"
#include "mqUtils.h"

namespace mqusd {


static Language g_language;

void MQSetLanguage(MQBasePlugin* v)
{
    auto lang = v->GetSettingValue(MQSETTINGVALUE_LANGUAGE);
    if (lang == L"Japanese")
        g_language = Language::Japanese;
    else
        g_language = Language::English;
}

Language MQGetLanguage()
{
    return g_language;
}

void MQShowError(const char* message)
{
    MQWindow parent = MQWindow::GetMainWindow();
    MQDialog::MessageWarningBox(parent, mu::ToWCS(message), L"Error");
}

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

MQWindow& MQGetMainWindow()
{
    static MQWindow s_window;
    s_window = MQWindow::GetMainWindow();
    return s_window;
}

} // namespace mqusd
