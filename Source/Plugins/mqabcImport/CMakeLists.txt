file(GLOB sources *.cpp *.h)
add_library(${MQABC_IMPORT} MODULE ${sources})
set_target_properties(${MQABC_IMPORT} PROPERTIES BUNDLE ON)
set_target_properties(${MQABC_IMPORT} PROPERTIES BUNDLE_EXTENSION "plugin")
set_target_properties(${MQABC_IMPORT} PROPERTIES OUTPUT_NAME "mqabcImport")

add_dependencies(${MQABC_IMPORT} MeshUtils ${MQSDK})
target_include_directories(${MQABC_IMPORT}
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/Plugins
        ${MQSDK_DIR}
)
target_link_libraries(${MQABC_IMPORT}
    PRIVATE
        MeshUtils
        ${MQSDK}
        ${COCOA_LIBRARY}
)

install(DIRECTORY $<TARGET_BUNDLE_DIR:${MQABC_IMPORT}> DESTINATION ${DIST_IMPORT})
