string(REGEX MATCH "([0-9]+)$" MQ_VERSION "${CMAKE_CURRENT_SOURCE_DIR}")
find_package(Metasequoia${MQ_VERSION})

set(MQSDK "mqsdk${MQ_VERSION}")
set(MQCOMMON "mqCommon${MQ_VERSION}")
set(MQABC "mqabc${MQ_VERSION}")
set(MQUSD "mqusd${MQ_VERSION}")
set(MQABC_IMPORTER "mqabcImporter${MQ_VERSION}")
set(MQUSD_IMPORTER "mqusdImporter${MQ_VERSION}")
set(MQABC_EXPORTER "mqabcExporter${MQ_VERSION}")
set(MQUSD_EXPORTER "mqusdExporter${MQ_VERSION}")
set(MQABC_RECORDER "mqabcRecorder${MQ_VERSION}")
set(MQUSD_RECORDER "mqusdRecorder${MQ_VERSION}")

set(DIST_DIR "${CMAKE_INSTALL_PREFIX}/mqusd_${PLATFORM}")
set(DIST_STATION "${DIST_DIR}/Station")
set(DIST_IMPORT "${DIST_DIR}/Import")
set(DIST_EXPORT "${DIST_DIR}/Export")
set(DIST_MISC "${DIST_DIR}/Misc")

add_subdirectory(mqsdk)
add_subdirectory(mqCommon)
add_subdirectory(mqabc)
add_subdirectory(mqusd)
add_subdirectory(mqabcImporter)
add_subdirectory(mqusdImporter)
add_subdirectory(mqabcExporter)
add_subdirectory(mqusdExporter)
add_subdirectory(mqabcRecorder)
add_subdirectory(mqusdRecorder)
