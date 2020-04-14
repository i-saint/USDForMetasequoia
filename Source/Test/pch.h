#pragma once

#ifdef _WIN32
    #define _CRT_SECURE_NO_WARNINGS
    #define NOMINMAX
    #define WIN32_LEAN_AND_MEAN
    #define _WINDOWS
    #include <windows.h>
    #include <comdef.h>
#endif // _WIN32

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cstdarg>
#include <cmath>
#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <algorithm>
#include <numeric>
#include <functional>
#include <memory>
#include <iostream>
#include <sstream>
#include <fstream>
#include <thread>
#include <future>
#include <random>
#include <regex>
#include <iterator>
