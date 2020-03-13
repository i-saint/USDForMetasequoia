#pragma once

#ifdef _WIN32
    #define _CRT_SECURE_NO_WARNINGS
    #define NOMINMAX
    #define WIN32_LEAN_AND_MEAN
    #define _WINDOWS
    #include <windows.h>
    #include <d3d11.h>
    #include <d3d11_4.h>
    #include <d3d12.h>
    #include <dxgi1_4.h>
    #include <dxgiformat.h>
    #include <dxcapi.h>
    #include <comdef.h>
#endif // _WIN32

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <fstream>
#include <future>

#pragma warning(push)
#pragma warning(disable:4244)
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usd/variantSets.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/xformCommonAPI.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/points.h"
#include "pxr/base/gf/transform.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/sdf/fileFormat.h"
#pragma warning(pop)
PXR_NAMESPACE_USING_DIRECTIVE;

#include "mqsdk/sdk_Include.h"
