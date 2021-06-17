cmake_minimum_required(VERSION 3.7.1)
project(cpp_media_server)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wno-deprecated -Wno-deprecated-declarations -Wno-reorder -Wall -fexceptions -frtti -D__STDC_FORMAT_MACROS -fPIC")
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_OUTPUT_BASE ${CMAKE_BINARY_DIR}/output)
set(BUILD_OUTPUT_BASE ${CMAKE_BINARY_DIR}/output)
set(PREFIX_DIR "${BUILD_OUTPUT_BASE}")

# set output static libary
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/output/lib)

# set pkgconfig path
set(CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/output/lib/pkgconfig")


# set include path
include_directories(./net
                    ${CMAKE_BINARY_DIR}/output/include
                    /usr/local/include/)

# set lib path
link_directories(${CMAKE_BINARY_DIR}/output/lib /usr/local/lib)

add_executable(httpClientDemo net/httpClientDemo.cpp)
IF (APPLE)
target_link_libraries(httpClientDemo dl z m ssl crypto boost_thread boost_system)
ELSEIF (UNIX)
target_link_libraries(httpClientDemo rt dl z m ssl crypto boost_thread boost_system)
ENDIF ()
