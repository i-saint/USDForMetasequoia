CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT(AlembicForMetasequoia)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
if(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    mark_as_advanced(COCOA_LIBRARY)
    set(PLATFORM "Mac")
    set(MAC ON)
endif()

find_package(TBB REQUIRED)
find_package(USD REQUIRED)
find_package(OpenEXR REQUIRED)
find_package(Alembic REQUIRED)
include(ISPC)


if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE PATH "" FORCE)
endif()
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/_dist" CACHE PATH "" FORCE)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -fPIC -fvisibility=hidden -std=c++14 -Wno-deprecated")

function(add_subdirectory_link to from)
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/${to}")
        execute_process(WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" COMMAND ln -s "${from}" "${to}")
    endif()
    add_subdirectory("${to}")
endfunction()

add_subdirectory(MeshUtils)
add_subdirectory(SceneGraph)
add_subdirectory_link(Plugins470 Plugins)

add_subdirectory(Test)
