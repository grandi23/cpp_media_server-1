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
                    ./net/http
                    ./net/rtmp
                    ./net/tcp
                    ./utils
                    ${CMAKE_BINARY_DIR}/output/include
                    /usr/local/include/)

# set lib path
link_directories(${CMAKE_BINARY_DIR}/output/lib /usr/local/lib64 /usr/local/lib)

add_executable(echo_demo net/tcp/echo_demo.cpp utils/logger.cpp)
IF (APPLE)
target_link_libraries(echo_demo pthread dl z m boost_thread boost_system)
ELSEIF (UNIX)
target_link_libraries(echo_demo pthread rt dl z m boost_thread boost_system)
ENDIF ()

add_executable(http_server_demo net/http/http_server_demo.cpp utils/logger.cpp)
IF (APPLE)
target_link_libraries(http_server_demo pthread dl z m boost_thread boost_system)
ELSEIF (UNIX)
target_link_libraries(http_server_demo pthread rt dl z m boost_thread boost_system)
ENDIF ()

add_executable(rtmp_server_demo
               net/rtmp/rtmp_server_demo.cpp
               net/rtmp/rtmp_session.cpp
               net/rtmp/rtmp_control_handler.cpp
               net/rtmp/rtmp_handshake.cpp
               utils/logger.cpp)
IF (APPLE)
target_link_libraries(rtmp_server_demo pthread dl z m ssl crypto boost_thread boost_system)
ELSEIF (UNIX)
target_link_libraries(rtmp_server_demo pthread rt dl z m ssl crypto boost_thread boost_system)
ENDIF ()