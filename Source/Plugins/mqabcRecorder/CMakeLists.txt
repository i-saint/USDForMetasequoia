file(GLOB sources *.cpp *.h)
add_library(${MQABC_RECORDER} MODULE ${sources})
set_target_properties(${MQABC_RECORDER} PROPERTIES BUNDLE ON)
set_target_properties(${MQABC_RECORDER} PROPERTIES BUNDLE_EXTENSION "plugin")
set_target_properties(${MQABC_RECORDER} PROPERTIES OUTPUT_NAME "mqabcRecorder")

add_dependencies(${MQABC_RECORDER} MeshUtils ${MQSDK})
target_include_directories(${MQABC_RECORDER}
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/Plugins
        ${MQSDK_DIR}
)
target_link_libraries(${MQABC_RECORDER}
    PRIVATE
        MeshUtils
        ${MQSDK}
        ${COCOA_LIBRARY}
)

install(DIRECTORY $<TARGET_BUNDLE_DIR:${MQABC_RECORDER}> DESTINATION ${DIST_STATION})
