file(GLOB sources *.cpp *.h)
add_library(${MQABC_EXPORT} MODULE ${sources})
set_target_properties(${MQABC_EXPORT} PROPERTIES BUNDLE ON)
set_target_properties(${MQABC_EXPORT} PROPERTIES BUNDLE_EXTENSION "plugin")
set_target_properties(${MQABC_EXPORT} PROPERTIES OUTPUT_NAME "mqabcExport")

add_dependencies(${MQABC_EXPORT} MeshUtils ${MQSDK})
target_include_directories(${MQABC_EXPORT}
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/Plugins
        ${MQSDK_DIR}
)
target_link_libraries(${MQABC_EXPORT}
    PRIVATE
        MeshUtils
        ${MQSDK}
        ${COCOA_LIBRARY}
)

install(DIRECTORY $<TARGET_BUNDLE_DIR:${MQABC_EXPORT}> DESTINATION ${DIST_EXPORT})
