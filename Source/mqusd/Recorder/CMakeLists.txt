string(REGEX MATCH "([0-9]+)" MQ_VERSION "${CMAKE_CURRENT_SOURCE_DIR}")
set(MQSDK "mqsdk${MQ_VERSION}")
set(MQUSD_RECORDER "mqusdRecorder${MQ_VERSION}")
find_package(Metasequoia${MQ_VERSION})

file(GLOB sources ../*.cpp ../*.h *.cpp *.h)
add_library(${MQUSD_RECORDER} MODULE ${sources})
set_target_properties(${MQUSD_RECORDER} PROPERTIES BUNDLE ON)
set_target_properties(${MQUSD_RECORDER} PROPERTIES OUTPUT_NAME "mqusdRecorder")

add_dependencies(${MQUSD_RECORDER} MeshUtils ${MQSDK})
target_include_directories(${MQUSD_RECORDER}
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/mqusd
        ${MQSDK_DIR}
)
target_link_libraries(${MQUSD_RECORDER}
    PRIVATE
        MeshUtils
        ${MQSDK}
        ${COCOA_LIBRARY}
)
set(DIST_DIR "${CMAKE_INSTALL_PREFIX}/mqusd_${PLATFORM}")
install(CODE "execute_process(COMMAND mkdir -p \"${DIST_DIR}/mqusdRecorder.plugin/\")")
install(CODE "execute_process(COMMAND rm -rf \"${DIST_DIR}/mqusdRecorder.plugin/\")")
install(CODE "execute_process(COMMAND cp -r \"${CMAKE_CURRENT_BINARY_DIR}/mqusdRecorder.bundle\" \"${DIST_DIR}/mqusdRecorder.plugin\")")
