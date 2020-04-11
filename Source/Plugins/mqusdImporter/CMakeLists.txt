file(GLOB sources *.cpp *.h)
add_library(${MQUSD_IMPORTER} MODULE ${sources})
set_target_properties(${MQUSD_IMPORTER} PROPERTIES BUNDLE ON)
set_target_properties(${MQUSD_IMPORTER} PROPERTIES BUNDLE_EXTENSION "plugin")
set_target_properties(${MQUSD_IMPORTER} PROPERTIES OUTPUT_NAME "mqusdImporter")

add_dependencies(${MQUSD_IMPORTER} MeshUtils ${MQSDK})
target_include_directories(${MQUSD_IMPORTER}
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/Plugins
        ${MQSDK_DIR}
)
target_link_libraries(${MQUSD_IMPORTER}
    PRIVATE
        MeshUtils
        ${MQSDK}
        ${COCOA_LIBRARY}
)

install(DIRECTORY $<TARGET_BUNDLE_DIR:${MQUSD_IMPORTER}> DESTINATION ${DIST_IMPORT})
