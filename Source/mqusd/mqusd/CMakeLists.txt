file(GLOB sources *.cpp *.h)
add_library(${MQUSD_PLAYER} MODULE ${sources})
set_target_properties(${MQUSD_PLAYER} PROPERTIES BUNDLE ON)
set_target_properties(${MQUSD_PLAYER} PROPERTIES BUNDLE_EXTENSION "plugin")
set_target_properties(${MQUSD_PLAYER} PROPERTIES OUTPUT_NAME "mqusdPlayer")

add_dependencies(${MQUSD_PLAYER} MeshUtils ${MQSDK} ${MQUSD_FOUNDATION})
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
        ${MQUSD_FOUNDATION}
        ${COCOA_LIBRARY}
)
add_custom_command(TARGET ${MQUSD_PLAYER} POST_BUILD
    COMMAND cp $<TARGET_FILE:${MQUSD_CORE}> ${USD_LIBRARIES} $<TARGET_FILE_DIR:${MQUSD_PLAYER}>
    COMMAND cp -r ${USD_PLUGIN_DIR} $<TARGET_FILE_DIR:${MQUSD_PLAYER}>
)

install(DIRECTORY $<TARGET_BUNDLE_DIR:${MQUSD_PLAYER}> DESTINATION ${DIST_DIR})
