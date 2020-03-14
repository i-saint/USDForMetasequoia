string(REGEX MATCH "([0-9]+)" MQ_VERSION "${CMAKE_CURRENT_SOURCE_DIR}")
set(MQSDK "mqsdk${MQ_VERSION}")
set(MQUSD_PLAYER "mqusdPlayer${MQ_VERSION}")
find_package(Metasequoia${MQ_VERSION})

file(GLOB sources ../*.cpp ../*.h *.cpp *.h)
add_library(${MQUSD_PLAYER} MODULE ${sources})
set_target_properties(${MQUSD_PLAYER} PROPERTIES BUNDLE ON)
set_target_properties(${MQUSD_PLAYER} PROPERTIES OUTPUT_NAME "mqusdPlayer")

add_dependencies(${MQUSD_PLAYER} MeshUtils ${MQSDK})
target_include_directories(${MQUSD_PLAYER}
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/mqusd
        ${MQSDK_DIR}
)
target_link_libraries(${MQUSD_PLAYER}
    PRIVATE
        MeshUtils
        ${MQSDK}
        ${COCOA_LIBRARY}
)
set(DIST_DIR "${CMAKE_INSTALL_PREFIX}/mqusd_${PLATFORM}")
install(CODE "execute_process(COMMAND mkdir -p \"${DIST_DIR}/mqusdPlayer.plugin/\")")
install(CODE "execute_process(COMMAND rm -rf \"${DIST_DIR}/mqusdPlayer.plugin/\")")
install(CODE "execute_process(COMMAND cp -r \"${CMAKE_CURRENT_BINARY_DIR}/mqusdPlayer.bundle\" \"${DIST_DIR}/mqusdPlayer.plugin\")")
