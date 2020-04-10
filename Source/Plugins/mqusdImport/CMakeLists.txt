file(GLOB sources *.cpp *.h)
add_library(${MQUSD_IMPORT} MODULE ${sources})
set_target_properties(${MQUSD_IMPORT} PROPERTIES BUNDLE ON)
set_target_properties(${MQUSD_IMPORT} PROPERTIES BUNDLE_EXTENSION "plugin")
set_target_properties(${MQUSD_IMPORT} PROPERTIES OUTPUT_NAME "mqusdImport")

add_dependencies(${MQUSD_IMPORT} MeshUtils ${MQSDK})
target_include_directories(${MQUSD_IMPORT}
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/Plugins
        ${MQSDK_DIR}
)
target_link_libraries(${MQUSD_IMPORT}
    PRIVATE
        MeshUtils
        ${MQSDK}
        ${COCOA_LIBRARY}
)

install(DIRECTORY $<TARGET_BUNDLE_DIR:${MQUSD_IMPORT}> DESTINATION ${DIST_IMPORT})
