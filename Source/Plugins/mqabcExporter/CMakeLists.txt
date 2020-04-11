file(GLOB sources *.cpp *.h)
add_library(${MQABC_EXPORTER} MODULE ${sources})
set_target_properties(${MQABC_EXPORTER} PROPERTIES BUNDLE ON)
set_target_properties(${MQABC_EXPORTER} PROPERTIES BUNDLE_EXTENSION "plugin")
set_target_properties(${MQABC_EXPORTER} PROPERTIES OUTPUT_NAME "mqabcExporter")

add_dependencies(${MQABC_EXPORTER} MeshUtils ${MQSDK})
target_include_directories(${MQABC_EXPORTER}
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/Plugins
        ${MQSDK_DIR}
)
target_link_libraries(${MQABC_EXPORTER}
    PRIVATE
        MeshUtils
        ${MQSDK}
        ${COCOA_LIBRARY}
)

install(DIRECTORY $<TARGET_BUNDLE_DIR:${MQABC_EXPORTER}> DESTINATION ${DIST_EXPORT})
