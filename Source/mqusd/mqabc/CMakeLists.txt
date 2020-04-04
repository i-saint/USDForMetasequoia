file(GLOB sources *.cpp *.h)
add_library(${MQABC_PLAYER} MODULE ${sources})
set_target_properties(${MQABC_PLAYER} PROPERTIES BUNDLE ON)
set_target_properties(${MQABC_PLAYER} PROPERTIES BUNDLE_EXTENSION "plugin")
set_target_properties(${MQABC_PLAYER} PROPERTIES OUTPUT_NAME "mqabcPlayer")

add_dependencies(${MQABC_PLAYER} MeshUtils ${MQSDK} ${MQUSD_FOUNDATION} ${MQABC_CORE})
target_include_directories(${MQABC_PLAYER}
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/mqusd
        ${MQSDK_DIR}
)
target_link_libraries(${MQABC_PLAYER}
    PRIVATE
        MeshUtils
        ${MQSDK}
        ${MQUSD_FOUNDATION}
        ${MQABC_CORE}
        ${ALEMBIC_LIBRARIES}
        ${COCOA_LIBRARY}
)

install(DIRECTORY $<TARGET_BUNDLE_DIR:${MQABC_PLAYER}> DESTINATION ${DIST_DIR})
