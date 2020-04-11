file(GLOB sources *.cpp *.h)
add_library(${MQABC_IMPORTER} MODULE ${sources})
set_target_properties(${MQABC_IMPORTER} PROPERTIES BUNDLE ON)
set_target_properties(${MQABC_IMPORTER} PROPERTIES BUNDLE_EXTENSION "plugin")
set_target_properties(${MQABC_IMPORTER} PROPERTIES OUTPUT_NAME "mqabcImporter")

add_dependencies(${MQABC_IMPORTER} MeshUtils ${MQSDK})
target_include_directories(${MQABC_IMPORTER}
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/Plugins
        ${MQSDK_DIR}
)
target_link_libraries(${MQABC_IMPORTER}
    PRIVATE
        MeshUtils
        ${MQSDK}
        ${COCOA_LIBRARY}
)

install(DIRECTORY $<TARGET_BUNDLE_DIR:${MQABC_IMPORTER}> DESTINATION ${DIST_IMPORT})
