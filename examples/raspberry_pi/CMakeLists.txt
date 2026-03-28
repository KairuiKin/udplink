cmake_minimum_required(VERSION 3.15)
project(raspberry_pi_udp_example)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 udplink
find_package(rudp QUIET)

if(NOT rudp_FOUND)
    # 如果没有安装，从上级目录构建
    add_subdirectory(${CMAKE_SOURCE_DIR}/.. rudp_build)
endif()

add_executable(raspberry_pi_udp raspberry_pi_udp.cpp)
if(TARGET rudp::rudp)
    target_link_libraries(raspberry_pi_udp PRIVATE rudp::rudp)
else()
    target_link_libraries(raspberry_pi_udp PRIVATE rudp)
endif()
