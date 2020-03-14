# * USD_INCLUDE_DIR
# * USD_LIBRARY

find_path(USD_INCLUDE_DIR
    NAMES
        "pxr/usd/usd/api.h"
    PATHS
        "/usr/include"
        "/usr/local/include"
        "/opt/pixar/include"
)
mark_as_advanced(USD_INCLUDE_DIR)

find_file(USD_LIBRARY libusd_ms.dylib PATH_SUFFIXES lib/)
mark_as_advanced(USD_LIBRARY)

get_filename_component(USD_LIBRARY_DIR ${USD_LIBRARY} DIRECTORY)
set(USD_PLUGIN_DIR "${USD_LIBRARY_DIR}/usd")

list(APPEND USD_INCLUDE_DIRS ${USD_INCLUDE_DIR})
list(APPEND USD_INCLUDE_DIRS ${TBB_INCLUDE_DIRS})

list(APPEND USD_LIBRARIES ${USD_LIBRARY})
list(APPEND USD_LIBRARIES ${TBB_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args("USD"
    DEFAULT_MSG
    USD_INCLUDE_DIR
	USD_LIBRARY
)
