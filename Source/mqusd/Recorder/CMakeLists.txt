string(REGEX MATCH "([0-9]+)" MQ_VERSION "${CMAKE_CURRENT_SOURCE_DIR}")
set(MQSDK "mqsdk${MQ_VERSION}")
set(MQABC_RECORDER "mqabcRecorder${MQ_VERSION}")
find_package(Metasequoia${MQ_VERSION})

file(GLOB sources ../*.cpp ../*.h *.cpp *.h)
add_library(${MQABC_RECORDER} MODULE ${sources})
set_target_properties(${MQABC_RECORDER} PROPERTIES BUNDLE ON)
set_target_properties(${MQABC_RECORDER} PROPERTIES OUTPUT_NAME "mqabcRecorder")

add_dependencies(${MQABC_RECORDER} MeshUtils ${MQSDK})
target_include_directories(${MQABC_RECORDER}
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/mqabc
        ${MQSDK_DIR}
        ${ALEMBIC_INCLUDE_DIRS}
)
target_link_libraries(${MQABC_RECORDER}
    PRIVATE
        MeshUtils
        ${MQSDK}
        ${ALEMBIC_LIBRARIES}
        ${COCOA_LIBRARY}
)
set(DIST_DIR "${CMAKE_INSTALL_PREFIX}/mqabc_${PLATFORM}")
install(CODE "execute_process(COMMAND mkdir -p \"${DIST_DIR}/mqabcRecorder.plugin/\")")
install(CODE "execute_process(COMMAND rm -rf \"${DIST_DIR}/mqabcRecorder.plugin/\")")
install(CODE "execute_process(COMMAND cp -r \"${CMAKE_CURRENT_BINARY_DIR}/mqabcRecorder.bundle\" \"${DIST_DIR}/mqabcRecorder.plugin\")")
