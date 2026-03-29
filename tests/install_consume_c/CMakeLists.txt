cmake_minimum_required(VERSION 3.10)
project(rudp_install_consume_c LANGUAGES C CXX)

find_package(rudp CONFIG REQUIRED)

add_executable(rudp_install_consume_c main.c)
target_link_libraries(rudp_install_consume_c PRIVATE rudp::rudp)
set_target_properties(rudp_install_consume_c PROPERTIES LINKER_LANGUAGE CXX)
