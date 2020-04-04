file(GLOB sources *.cpp *.h)
add_library(${MQABC} MODULE ${sources})
set_target_properties(${MQABC} PROPERTIES BUNDLE ON)
set_target_properties(${MQABC} PROPERTIES BUNDLE_EXTENSION "plugin")
set_target_properties(${MQABC} PROPERTIES OUTPUT_NAME "mqabc")

add_dependencies(${MQABC} MeshUtils SceneGraph SceneGraphABC ${MQSDK} ${MQCOMMON})
target_include_directories(${MQABC}
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/Plugins
        ${MQSDK_DIR}
)
target_link_libraries(${MQABC}
    PRIVATE
        MeshUtils
        SceneGraph
        SceneGraphABC
        ${MQSDK}
        ${MQCOMMON}
        ${ALEMBIC_LIBRARIES}
        ${COCOA_LIBRARY}
)

install(DIRECTORY $<TARGET_BUNDLE_DIR:${MQABC}> DESTINATION ${DIST_DIR})
