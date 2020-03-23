string(REGEX MATCH "([0-9]+)$" MQ_VERSION "${CMAKE_CURRENT_SOURCE_DIR}")
find_package(Metasequoia${MQ_VERSION})

set(MQSDK "mqsdk${MQ_VERSION}")
set(MQUSD_FOUNDATION "mqusdFoundation${MQ_VERSION}")
set(MQUSD_CORE "mqusdCore${MQ_VERSION}")
set(MQUSD_PLAYER "mqusdPlayer${MQ_VERSION}")
set(MQUSD_RECORDER "mqusdRecorder${MQ_VERSION}")
set(DIST_DIR "${CMAKE_INSTALL_PREFIX}/mqusd_${PLATFORM}")

add_subdirectory(Foundation)
add_subdirectory(Core)
add_subdirectory(Player)
add_subdirectory(Recorder)
