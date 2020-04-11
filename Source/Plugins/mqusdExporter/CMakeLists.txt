file(GLOB sources *.cpp *.h)
add_library(${MQUSD_EXPORTER} MODULE ${sources})
set_target_properties(${MQUSD_EXPORTER} PROPERTIES BUNDLE ON)
set_target_properties(${MQUSD_EXPORTER} PROPERTIES BUNDLE_EXTENSION "plugin")
set_target_properties(${MQUSD_EXPORTER} PROPERTIES OUTPUT_NAME "mqusdExporter")

add_dependencies(${MQUSD_EXPORTER} MeshUtils ${MQSDK})
target_include_directories(${MQUSD_EXPORTER}
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/Plugins
        ${MQSDK_DIR}
)
target_link_libraries(${MQUSD_EXPORTER}
    PRIVATE
        MeshUtils
        ${MQSDK}
        ${COCOA_LIBRARY}
)

install(DIRECTORY $<TARGET_BUNDLE_DIR:${MQUSD_EXPORTER}> DESTINATION ${DIST_EXPORT})
