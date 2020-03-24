file(GLOB sources
    mqusdCore.h
    mqusdSceneGraphUSD.h
    mqusdSceneGraphUSD.cpp
    ../*.h)
add_library(${MQUSD_CORE} SHARED ${sources})
set_target_properties(${MQUSD_CORE} PROPERTIES OUTPUT_NAME "mqusdCore")

add_dependencies(${MQUSD_CORE} MeshUtils ${MQUSD_FOUNDATION})
target_include_directories(${MQUSD_CORE}
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/mqusd
        ${MQSDK_DIR}
        ${USD_INCLUDE_DIRS}
)
target_link_libraries(${MQUSD_CORE}
    PRIVATE
        MeshUtils
        ${MQUSD_FOUNDATION}
        ${USD_LIBRARIES}
        ${COCOA_LIBRARY}
)
