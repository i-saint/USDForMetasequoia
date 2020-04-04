file(GLOB sources *.h *.cpp)
add_library(SceneGraph STATIC ${sources})
target_include_directories(SceneGraph
    PRIVATE
        ${CMAKE_SOURCE_DIR}
)

add_subdirectory(ABC)
add_subdirectory(USD)