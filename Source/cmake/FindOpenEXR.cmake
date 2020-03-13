find_path(OPENEXR_INCLUDE_DIR
    NAMES
        OpenEXR/half.h
    PATHS
        "/usr/include"
        "/usr/local/include"
)

foreach(OPENEXR_LIB
    Half Iex IexMath Imath IlmImf IlmThread
)
    find_library(OPENEXR_${OPENEXR_LIB}_LIBRARY "${OPENEXR_LIB}-2_4" PATH_SUFFIXES lib/)
    mark_as_advanced(OPENEXR_${OPENEXR_LIB}_LIBRARY)

    if(OPENEXR_${OPENEXR_LIB}_LIBRARY)
        list(APPEND OPENEXR_LIBRARIES ${OPENEXR_${OPENEXR_LIB}_LIBRARY})
    endif()
endforeach(OPENEXR_LIB)

list(APPEND OPENEXR_INCLUDE_DIRS ${OPENEXR_INCLUDE_DIR})
list(APPEND OPENEXR_INCLUDE_DIRS ${OPENEXR_INCLUDE_DIR}/OpenEXR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenEXR
    REQUIRED_VARS
        OPENEXR_INCLUDE_DIRS
        OPENEXR_LIBRARIES
)
