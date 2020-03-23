#include "pch.h"
#include "Foundation/mqusdSceneGraph.h"
#include "Foundation/mqusdSceneGraphRemote.h"
#include <io.h>
#include <fcntl.h>
#include <iostream>
using namespace mqusd;

int main(int argc, char *argv[])
{
#ifdef _WIN32
    // set stdin&stdout binary mode
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    std::string usd_path;
    std::string file_path;
    double time = mqusd::default_time;
    bool mode_export = false;
    bool version = false;
    bool mode_test = false;
    for (int ai = 1; ai < argc;) {
        if (argv[ai][0] == '-') {
            if (strcmp(argv[ai], "-version") == 0)
                version = true;
            if (strcmp(argv[ai], "-export") == 0)
                mode_export = true;
            if (strcmp(argv[ai], "-test") == 0)
                mode_test = true;
            if (strcmp(argv[ai], "-time") == 0)
                sscanf(argv[++ai], "%lf", &time);
            if (strcmp(argv[ai], "-file") == 0)
                file_path = argv[++ai];
        }
        else
            usd_path = argv[ai];
        ++ai;
    }

    if (version) {
        printf("version: " mqusdVersionString "\n");
        return 0;
    }
    else if (usd_path.empty()) {
        printf(
            "usage: %s [options] path_to_usd.usd\n"
            "   options:\n"
            "    -version: output version info.\n"
            "    -export: export mode. default is import.\n"
            "    -test: test to open usd.\n"
            "    -time time_in_seconds\n"
            "    -file path_to_inout_file\n"
            , argv[0]);
        return 0;
    }

    mqusd::SetUSDModuleDir(mu::GetCurrentModuleDirectory());
    auto scene = mqusd::CreateUSDScene();
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
    else if (mode_export) {
        try {
            if (!scene->create(usd_path.c_str()))
                return 1;

            if (!file_path.empty()) {
                std::fstream f(file_path.c_str(), std::ios::in | std::ios::binary);
                scene->deserialize(f);
            }
            else {
                scene->deserialize(std::cin);
            }
            scene->write(time);
            scene->save();
        }
        catch (std::exception& e) {
            printf("error: %s\n", e.what());
        }
    }
    else {
        if (!scene->open(usd_path.c_str()))
            return 1;

        scene->read(time);
        if (!file_path.empty()) {
            std::fstream of(file_path.c_str(), std::ios::out | std::ios::binary);
            scene->serialize(of);
        }
        else {
            scene->serialize(std::cout);
        }
    }
    return 0;
}
