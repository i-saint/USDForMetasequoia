file(GLOB sources *.h *.cpp ../*.h)
add_library(${MQCOMMON} STATIC ${sources})
set_target_properties(${MQCOMMON} PROPERTIES OUTPUT_NAME "mqCommon")

target_include_directories(${MQCOMMON}
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/Plugins
        ${MQSDK_DIR}
        ${USD_INCLUDE_DIRS}
)
