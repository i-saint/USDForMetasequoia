#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <io.h>
#include <fcntl.h>
#endif

#include <iostream>
#include <fstream>
#include <functional>
#include "../SceneGraph.h"
#include "../SceneGraphRemote.h"

using namespace sg;


int main(int argc, char* argv[])
{
#ifdef _WIN32
    // set stdin&out binary mode
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    std::string usd_path;
    std::string file_path;
    double time = sg::default_time;
    bool mode_export = false;
    bool mode_test = false;
    bool mode_header= false;
#ifdef mqusdDebug
    bool mode_debug = false;
#endif
    bool version = false;
    for (int ai = 1; ai < argc;) {
        if (argv[ai][0] == '-') {
            if (strcmp(argv[ai], "-version") == 0)
                version = true;
            if (strcmp(argv[ai], "-export") == 0)
                mode_export = true;
            if (strcmp(argv[ai], "-tree") == 0)
                mode_header = true;
            if (strcmp(argv[ai], "-test") == 0)
                mode_test = true;
            if (strcmp(argv[ai], "-time") == 0)
                sscanf(argv[++ai], "%lf", &time);
            if (strcmp(argv[ai], "-file") == 0)
                file_path = argv[++ai];
#ifdef _WIN32
            if (strcmp(argv[ai], "-hide") == 0)
                ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
#endif
#ifdef mqusdDebug
            if (strcmp(argv[ai], "-debug") == 0)
                mode_debug = true;
#endif
        }
        else
            usd_path = argv[ai];
        ++ai;
    }

    if (version) {
        printf("version: " sgVersionString "\n");
        return 0;
    }
    else if (usd_path.empty()) {
        printf(
            "usage: %s [options] path_to_usd.usd\n"
            "   options:\n"
            "    -version: output version info.\n"
            "    -export: export mode. default is import.\n"
            "    -tree: construct node tree but don't read data.\n"
            "    -test: test to open usd.\n"
            "    -time time_in_seconds\n"
            "    -file path_to_inout_file\n"
#ifdef _WIN32
            "    -hide: hide console window.\n"
#endif
            , argv[0]);
        return 0;
    }

    auto scene = sg::CreateUSDScene();
    if (!scene)
        return 1;

    if (mode_test) {
        if (mode_export) {
            if (!scene->create(usd_path.c_str()))
                return 1;
        }
        else {
            if (!scene->open(usd_path.c_str()))
                return 1;
        }
   }
#ifdef mqusdDebug
    else if (mode_debug) {
        std::string dst_dir = mu::GetDirectory(usd_path.c_str());
        std::string dst_bin = dst_dir + muPathSep "debug.bin";
        std::string dst_usd = dst_dir + muPathSep "debug.usda";

        if (scene->open(usd_path.c_str())) {
            scene->read(time);
            {
                std::fstream f(dst_bin, std::ios::out | std::ios::binary);
                sg::serializer s(f);
                scene->serialize(s);
            }

            scene = sg::CreateUSDScene();
            if (scene->create(dst_usd.c_str())) {
                std::fstream f(dst_bin, std::ios::in | std::ios::binary);
                sg::deserializer s(f);
                scene->deserialize(s);
                scene->write(scene->time_current);
                scene->save();
            }
        }
    }
#endif
    else if (mode_export) {
        try {
            if (!scene->create(usd_path.c_str()))
                return 1;

            if (!file_path.empty()) {
                std::fstream f(file_path.c_str(), std::ios::in | std::ios::binary);
                sg::deserializer d(f);
                scene->deserialize(d);
            }
            else {
                sg::deserializer d(std::cin);
                scene->deserialize(d);
            }
            scene->write(scene->time_current);
            scene->save();
        }
        catch (std::exception& e) {
            printf("error: %s\n", e.what());
        }
    }
    else {
        if (!scene->open(usd_path.c_str()))
            return 1;

        if (!mode_header)
            scene->read(time);

        if (!file_path.empty()) {
            std::fstream of(file_path.c_str(), std::ios::out | std::ios::binary);
            sg::serializer s(of);
            scene->serialize(s);
        }
        else {
            sg::serializer s(std::cout);
            scene->serialize(s);
        }
    }
    return 0;
}
