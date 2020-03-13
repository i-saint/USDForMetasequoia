string(REGEX MATCH "([0-9]+)$" MQ_VERSION "${CMAKE_CURRENT_SOURCE_DIR}")
set(MQSDK "mqsdk${MQ_VERSION}")
find_package(Metasequoia${MQ_VERSION})

file(GLOB sources *.cpp *.c *.mm *.h)
add_library(${MQSDK} STATIC ${sources})
target_include_directories(${MQSDK} PUBLIC "${CMAKE_SOURCE_DIR}" "${MQSDK_DIR}")
