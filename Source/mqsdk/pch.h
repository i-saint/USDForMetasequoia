#pragma once

#ifdef _WIN32
    #define _CRT_SECURE_NO_WARNINGS
    #define NOMINMAX
    #define WIN32_LEAN_AND_MEAN
    #define _WINDOWS
    #include <windows.h>
    #include <comdef.h>
#endif // _WIN32

#include "sdk_Include.h"
