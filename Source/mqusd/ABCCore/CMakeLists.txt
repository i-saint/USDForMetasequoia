file(GLOB sources *.h *.cpp ../*.h)
add_library(${MQABC_CORE} STATIC ${sources})
set_target_properties(${MQABC_CORE} PROPERTIES OUTPUT_NAME "mqabcCore")

target_include_directories(${MQABC_CORE}
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/mqusd
        ${MQSDK_DIR}
        ${ALEMBIC_INCLUDE_DIRS}
)
