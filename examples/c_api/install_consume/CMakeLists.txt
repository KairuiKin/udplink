cmake_minimum_required(VERSION 3.10)
project(rudp_c_api_example LANGUAGES C CXX)

find_package(rudp CONFIG REQUIRED)

add_executable(rudp_c_api_example main.c)
target_link_libraries(rudp_c_api_example PRIVATE rudp::rudp)
set_target_properties(rudp_c_api_example PROPERTIES LINKER_LANGUAGE CXX)