file(GLOB sources *.h *.cpp)
list(REMOVE_ITEM sources mqusdCoreExe.cpp)
add_library(SceneGraphUSD SHARED ${sources})

add_dependencies(SceneGraphUSD MeshUtils SceneGraph)
target_include_directories(SceneGraphUSD
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/SceneGraph
        ${USD_INCLUDE_DIRS}
)
target_link_libraries(SceneGraphUSD
    PRIVATE
        MeshUtils
        SceneGraph
        ${USD_LIBRARIES}
        ${COCOA_LIBRARY}
)
