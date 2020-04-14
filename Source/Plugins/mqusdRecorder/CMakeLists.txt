file(GLOB sources *.cpp *.h)
add_library(${MQUSD_RECORDER} MODULE ${sources})
set_target_properties(${MQUSD_RECORDER} PROPERTIES BUNDLE ON)
set_target_properties(${MQUSD_RECORDER} PROPERTIES BUNDLE_EXTENSION "plugin")
set_target_properties(${MQUSD_RECORDER} PROPERTIES OUTPUT_NAME "mqusdRecorder")

add_dependencies(${MQUSD_RECORDER} MeshUtils ${MQSDK})
target_include_directories(${MQUSD_RECORDER}
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/Plugins
        ${MQSDK_DIR}
)
target_link_libraries(${MQUSD_RECORDER}
    PRIVATE
        MeshUtils
        ${MQSDK}
        ${COCOA_LIBRARY}
)

#install(DIRECTORY $<TARGET_BUNDLE_DIR:${MQUSD_RECORDER}> DESTINATION ${DIST_STATION})
