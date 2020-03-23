file(GLOB sources *.h *.cpp ../*.h)
add_library(${MQUSD_FOUNDATION} STATIC ${sources})
set_target_properties(${MQUSD_FOUNDATION} PROPERTIES OUTPUT_NAME "mqusdFoundation")

target_include_directories(${MQUSD_FOUNDATION}
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/mqusd
        ${MQSDK_DIR}
        ${USD_INCLUDE_DIRS}
)
