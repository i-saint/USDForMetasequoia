#include "pch.h"
#include "sgusdSceneGraph.h"
#include "sgusdUtils.h"

namespace sg {

void PrintPrim(UsdPrim prim, PrintFlags flags)
{
    std::stringstream ss;
    if (flags & PF_Path) {
        mu::Print("prim %s (%s)\n",
            prim.GetPath().GetText(),
            prim.GetTypeName().GetText());
    }
    if (flags & PF_Attr) {
        for (auto& attr : prim.GetAuthoredAttributes()) {
            VtValue tv;
            attr.Get(&tv);
            ss << tv;
            mu::Print("  attr %s (%s): %s\n",
                attr.GetName().GetText(),
                attr.GetTypeName().GetAsToken().GetText(),
                ss.str().c_str());
            ss.str({});

            if (flags & PF_Meta) {
                for (auto& kvp : prim.GetAllAuthoredMetadata()) {
                    ss << kvp.second;
                    mu::Print("    meta %s (%s) - %s\n",
                        kvp.first.GetText(),
                        kvp.second.GetTypeName().c_str(),
                        ss.str().c_str());
                    ss.str({});
                }
            }
        }
    }
    if(flags & PF_Rel) {
        for (auto& rel : prim.GetAuthoredRelationships()) {
            SdfPathVector paths;
            rel.GetTargets(&paths);
            for (auto& path : paths) {
                mu::Print("  rel %s %s\n", rel.GetName().GetText(), path.GetText());
            }
        }
    }
}


void GetBinary(UsdAttribute& attr, std::string& v, UsdTimeCode t)
{
    std::string tmp;
    attr.Get(&tmp, t);

    v.clear();
    size_t n = tmp.size() / 2;
    const char* buf = tmp.data();
    for (size_t i = 0; i < n; ++i) {
        int c;
        sscanf(buf, "%02x", &c);
        v += (char)c;
        buf += 2;
    }
}

void SetBinary(UsdAttribute& attr, const std::string& v, UsdTimeCode t)
{
    std::string tmp;
    char buf[4];
    size_t n = v.size();
    auto* c = (const byte*)v.data();
    for (size_t i = 0; i < n; ++i) {
        sprintf(buf, "%02x", (int)*c);
        ++c;
        tmp += buf;
    }
    attr.Set(tmp, t);
}

} // namespace sg
