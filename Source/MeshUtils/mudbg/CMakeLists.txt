file(GLOB sources *.h *.cpp)
add_library(mudbg SHARED ${sources})

add_dependencies(mudbg MeshUtils)
target_include_directories(mudbg
    PRIVATE
        ${CMAKE_SOURCE_DIR}
)
target_link_libraries(mudbg
    PRIVATE
        MeshUtils
)
