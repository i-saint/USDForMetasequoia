file(GLOB sources ../Foundation/*.cpp ../Foundation/*.h ../*.cpp ../*.h *.cpp *.h)
add_library(${MQUSD_RECORDER} MODULE ${sources})
set_target_properties(${MQUSD_RECORDER} PROPERTIES BUNDLE ON)
set_target_properties(${MQUSD_RECORDER} PROPERTIES BUNDLE_EXTENSION "plugin")
set_target_properties(${MQUSD_RECORDER} PROPERTIES OUTPUT_NAME "mqusdRecorder")

add_dependencies(${MQUSD_RECORDER} MeshUtils ${MQSDK} ${MQUSD_FOUNDATION})
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
        ${MQUSD_FOUNDATION}
        ${COCOA_LIBRARY}
)
add_custom_command(TARGET ${MQUSD_RECORDER} POST_BUILD
    COMMAND cp $<TARGET_FILE:${MQUSD_CORE}> ${USD_LIBRARIES} $<TARGET_FILE_DIR:${MQUSD_RECORDER}>
    COMMAND cp -r ${USD_PLUGIN_DIR} $<TARGET_FILE_DIR:${MQUSD_RECORDER}>
)

install(DIRECTORY $<TARGET_BUNDLE_DIR:${MQUSD_RECORDER}> DESTINATION ${DIST_DIR})
