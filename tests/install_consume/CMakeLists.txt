cmake_minimum_required(VERSION 3.10)
project(rudp_install_consume LANGUAGES CXX)

find_package(rudp CONFIG REQUIRED)

add_executable(rudp_install_consume main.cpp)
target_link_libraries(rudp_install_consume PRIVATE rudp::rudp)
target_compile_features(rudp_install_consume PRIVATE cxx_std_11)
