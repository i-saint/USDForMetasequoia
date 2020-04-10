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

USDNode* GetUSDNode(Node* n)
{
    return static_cast<USDNode*>(n->impl);
}

std::string GetUSDName(Node* n)
{
    return GetUSDNode(n)->getName();
}

std::string GetUSDPath(Node* n)
{
    return GetUSDNode(n)->getPath();
}

void GetBinary(UsdAttribute& attr, std::string& v, UsdTimeCode t)
{
    std::string tmp;
    attr.Get(&tmp, t);
    v = BinaryDecode(tmp);
}

void SetBinary(UsdAttribute& attr, const std::string& v, UsdTimeCode t)
{
    attr.Set(BinaryEncode(v), t);
}

} // namespace sg
