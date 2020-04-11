#pragma once
#include "mqsdk/sdk_Include.h"

namespace mqusd {


class DummyExportPlugin : public MQExportPlugin
{
public:
    void GetPlugInID(DWORD* Product, DWORD* ID) override {}
    const char* GetPlugInName(void) override { return ""; }
    const char* EnumFileType(int index) override { return nullptr; }
    const char* EnumFileExt(int index) override { return nullptr; }
    BOOL ExportFile(int index, const wchar_t* filename, MQDocument doc) override { return false; }

    static DummyExportPlugin& getInstance()
    {
        static DummyExportPlugin s_inst;
        return s_inst;
    }
};

class DummyImportPlugin : public MQImportPlugin
{
public:
    void GetPlugInID(DWORD* Product, DWORD* ID) override {}
    const char* GetPlugInName(void) override { return ""; }
    const char* EnumFileType(int index) override { return nullptr; }
    const char* EnumFileExt(int index) override { return nullptr; }
    BOOL ImportFile(int index, const wchar_t* filename, MQDocument doc) override { return false; }

    static DummyImportPlugin& getInstance()
    {
        static DummyImportPlugin s_inst;
        return s_inst;
    }
};

class DummyStationPlugin : public MQStationPlugin
{
public:
#if defined(__APPLE__) || defined(__linux__)
    MQBasePlugin* CreateNewPlugin() override;
#endif
    void GetPlugInID(DWORD* Product, DWORD* ID) override {}
    const char* GetPlugInName(void) override { return ""; }
#if MQPLUGIN_VERSION >= 0x0470
    const wchar_t* EnumString(void) override { return L""; }
#else
    const char* EnumString(void) override { return ""; }
#endif
    const char* EnumSubCommand(int index) override { return ""; }
    const wchar_t* GetSubCommandString(int index) override { return L""; }

    BOOL Initialize() override { return false; }
    void Exit() override {}
    BOOL Activate(MQDocument doc, BOOL flag) override { return false; }
    BOOL IsActivated(MQDocument doc) override { return false; }
    bool ExecuteCallback(MQDocument doc, void* option) override { return false; }

    static DummyStationPlugin& getInstance()
    {
        static DummyStationPlugin s_inst;
        return s_inst;
    }
};

} // namespace mqusd
