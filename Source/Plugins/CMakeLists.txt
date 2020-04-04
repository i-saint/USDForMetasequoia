string(REGEX MATCH "([0-9]+)$" MQ_VERSION "${CMAKE_CURRENT_SOURCE_DIR}")
find_package(Metasequoia${MQ_VERSION})

set(MQSDK "mqsdk${MQ_VERSION}")
set(MQCOMMON "mqCommon${MQ_VERSION}")
set(MQUSD "mqusd${MQ_VERSION}")
set(MQABC "mqabc${MQ_VERSION}")
set(DIST_DIR "${CMAKE_INSTALL_PREFIX}/mqusd_${PLATFORM}")

add_subdirectory(mqsdk)
add_subdirectory(mqCommon)
add_subdirectory(mqabc)
add_subdirectory(mqusd)