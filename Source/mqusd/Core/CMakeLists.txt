file(GLOB sources
    ../Foundation/mqusdSceneGraph.cpp
    ../Foundation/mqusdSceneGraph.h
    ../*.cpp ../*.h *.cpp *.h)
add_library(${MQUSD_CORE} SHARED ${sources})
set_target_properties(${MQUSD_CORE} PROPERTIES OUTPUT_NAME "mqusdCore")

add_dependencies(${MQUSD_CORE} MeshUtils)
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
        ${USD_LIBRARIES}
        ${COCOA_LIBRARY}
)
