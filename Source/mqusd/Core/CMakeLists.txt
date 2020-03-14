string(REGEX MATCH "([0-9]+)" MQ_VERSION "${CMAKE_CURRENT_SOURCE_DIR}")
set(MQSDK "mqsdk${MQ_VERSION}")
set(MQUSD_CORE "mqusdCore${MQ_VERSION}")
find_package(Metasequoia${MQ_VERSION})

file(GLOB sources ../*.cpp ../*.h *.cpp *.h)
add_library(${MQUSD_CORE} SHARED ${sources})
set_target_properties(${MQUSD_CORE} PROPERTIES OUTPUT_NAME "mqusdCore")
set_target_properties(${MQUSD_CORE} PROPERTIES PREFIX "")

add_dependencies(${MQUSD_CORE} MeshUtils ${MQSDK})
target_include_directories(${MQUSD_CORE}
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/mqusd
        ${MQSDK_DIR}
        ${USD_INCLUDE_DIRS}
)
target_link_libraries(${MQUSD_CORE}
    PRIVATE
        MeshUtils
        ${MQSDK}
        ${USD_LIBRARIES}
        ${COCOA_LIBRARY}
)

install(FILES $<TARGET_FILE:${MQUSD_CORE}> ${USD_LIBRARIES} DESTINATION ${CORE_DIR})
install(DIRECTORY ${USD_PLUGIN_DIR} DESTINATION ${CORE_DIR})
