string(REGEX MATCH "([0-9]+)" MQ_VERSION "${CMAKE_CURRENT_SOURCE_DIR}")
set(MQSDK "mqsdk${MQ_VERSION}")
set(MQUSD_PLAYER "mqusdPlayer${MQ_VERSION}")
find_package(Metasequoia${MQ_VERSION})

file(GLOB sources ../*.cpp ../*.h *.cpp *.h)
add_library(${MQUSD_PLAYER} MODULE ${sources})
set_target_properties(${MQUSD_PLAYER} PROPERTIES BUNDLE ON)
set_target_properties(${MQUSD_PLAYER} PROPERTIES BUNDLE_EXTENSION "plugin")
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

install(DIRECTORY $<TARGET_BUNDLE_DIR:${MQUSD_PLAYER}> DESTINATION ${DIST_DIR})
