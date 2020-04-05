#pragma once
#include "../SceneGraph.h"

#ifdef _WIN32
    #define sgusdAPI extern "C" __declspec(dllexport)
#else
    #define sgusdAPI extern "C" __attribute__((visibility("default")))
#endif

sgusdAPI sg::SceneInterface* sgusdCreateSceneInterface(sg::Scene* scene);
