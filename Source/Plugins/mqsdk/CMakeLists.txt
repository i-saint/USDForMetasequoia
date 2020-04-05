file(GLOB sources *.cpp *.c *.mm *.h)
add_library(${MQSDK} STATIC ${sources})
target_include_directories(${MQSDK} PUBLIC ${CMAKE_SOURCE_DIR} ${MQSDK_DIR})
