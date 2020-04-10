#pragma once

#ifdef _WIN32
    #define mudbgAPI extern "C" __declspec(dllexport)
#else
    #define mudbgAPI extern "C" __attribute__((visibility("default")))
#endif
