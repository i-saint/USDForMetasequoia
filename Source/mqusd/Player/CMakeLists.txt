string(REGEX MATCH "([0-9]+)" MQ_VERSION "${CMAKE_CURRENT_SOURCE_DIR}")
set(MQSDK "mqsdk${MQ_VERSION}")
set(MQABC_PLAYER "mqabcPlayer${MQ_VERSION}")
find_package(Metasequoia${MQ_VERSION})

file(GLOB sources ../*.cpp ../*.h *.cpp *.h)
add_library(${MQABC_PLAYER} MODULE ${sources})
set_target_properties(${MQABC_PLAYER} PROPERTIES BUNDLE ON)
set_target_properties(${MQABC_PLAYER} PROPERTIES OUTPUT_NAME "mqabcPlayer")

add_dependencies(${MQABC_PLAYER} MeshUtils ${MQSDK})
target_include_directories(${MQABC_PLAYER}
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/mqabc
        ${MQSDK_DIR}
        ${ALEMBIC_INCLUDE_DIRS}
)
target_link_libraries(${MQABC_PLAYER}
    PRIVATE
        MeshUtils
        ${MQSDK}
        ${ALEMBIC_LIBRARIES}
        ${COCOA_LIBRARY}
)
set(DIST_DIR "${CMAKE_INSTALL_PREFIX}/mqabc_${PLATFORM}")
install(CODE "execute_process(COMMAND mkdir -p \"${DIST_DIR}/mqabcPlayer.plugin/\")")
install(CODE "execute_process(COMMAND rm -rf \"${DIST_DIR}/mqabcPlayer.plugin/\")")
install(CODE "execute_process(COMMAND cp -r \"${CMAKE_CURRENT_BINARY_DIR}/mqabcPlayer.bundle\" \"${DIST_DIR}/mqabcPlayer.plugin\")")
