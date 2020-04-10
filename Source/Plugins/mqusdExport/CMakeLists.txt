file(GLOB sources *.cpp *.h)
add_library(${MQUSD_EXPORT} MODULE ${sources})
set_target_properties(${MQUSD_EXPORT} PROPERTIES BUNDLE ON)
set_target_properties(${MQUSD_EXPORT} PROPERTIES BUNDLE_EXTENSION "plugin")
set_target_properties(${MQUSD_EXPORT} PROPERTIES OUTPUT_NAME "mqusdExport")

add_dependencies(${MQUSD_EXPORT} MeshUtils ${MQSDK})
target_include_directories(${MQUSD_EXPORT}
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/Plugins
        ${MQSDK_DIR}
)
target_link_libraries(${MQUSD_EXPORT}
    PRIVATE
        MeshUtils
        ${MQSDK}
        ${COCOA_LIBRARY}
)

install(DIRECTORY $<TARGET_BUNDLE_DIR:${MQUSD_EXPORT}> DESTINATION ${DIST_EXPORT})
