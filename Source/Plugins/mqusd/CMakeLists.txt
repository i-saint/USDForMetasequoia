file(GLOB sources *.cpp *.h)
add_library(${MQUSD} MODULE ${sources})
set_target_properties(${MQUSD} PROPERTIES BUNDLE ON)
set_target_properties(${MQUSD} PROPERTIES BUNDLE_EXTENSION "plugin")
set_target_properties(${MQUSD} PROPERTIES OUTPUT_NAME "mqusd")

add_dependencies(${MQUSD} MeshUtils SceneGraph ${MQSDK} ${MQCOMMON})
target_include_directories(${MQUSD}
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/Plugins
        ${MQSDK_DIR}
)
target_link_libraries(${MQUSD}
    PRIVATE
        MeshUtils
        SceneGraph
        ${MQSDK}
        ${MQCOMMON}
        ${COCOA_LIBRARY}
)
add_custom_command(TARGET ${MQUSD} POST_BUILD
    COMMAND cp $<TARGET_FILE:SceneGraphUSD> ${USD_LIBRARIES} $<TARGET_FILE_DIR:${MQUSD}>
    COMMAND cp -r ${USD_PLUGIN_DIR} $<TARGET_FILE_DIR:${MQUSD}>
)

install(DIRECTORY $<TARGET_BUNDLE_DIR:${MQUSD}> DESTINATION ${DIST_STATION})
