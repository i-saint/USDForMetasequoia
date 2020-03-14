# * TBB_INCLUDE_DIR
# * TBB_LIBRARY

find_path(TBB_INCLUDE_DIR
    NAMES
        "tbb/tbb.h"
    PATHS
        "/usr/include"
        "/usr/local/include"
        "/opt/intel/include"
)
mark_as_advanced(TBB_INCLUDE_DIR)

find_file(TBB_LIBRARY tbb.dylib PATH_SUFFIXES lib/)
mark_as_advanced(TBB_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args("TBB"
    DEFAULT_MSG
    TBB_INCLUDE_DIR
	TBB_LIBRARY
)
