find_package(Metasequoia470)

file(GLOB sources *.cpp *.h)
add_executable(Test ${sources})

add_dependencies(Test MeshUtils mqusdFoundation470)
target_include_directories(Test
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/mqusd
        ${MQSDK_DIR}
)
target_link_libraries(Test
    PRIVATE
        MeshUtils
        mqusdFoundation470
)
if(LINUX)
    target_link_libraries(Test pthread "-Wl,--no-undefined")
endif()
