#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <sstream>
#include <fstream>
#include <future>
#include <random>
#include <numeric>


#define mqabcEnableHDF5
#pragma warning(push)
#pragma warning(disable:4244 4245 4100)
#include <Alembic/AbcCoreAbstract/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
#ifdef mqabcEnableHDF5
    #include <Alembic/AbcCoreHDF5/All.h>
#endif
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcMaterial/All.h>
#pragma warning(pop)