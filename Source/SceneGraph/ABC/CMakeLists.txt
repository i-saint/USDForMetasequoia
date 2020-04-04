file(GLOB sources *.h *.cpp)
add_library(SceneGraphABC STATIC ${sources})

target_include_directories(SceneGraphABC
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/SceneGraph
        ${ALEMBIC_INCLUDE_DIRS}
)
