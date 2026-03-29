cmake_minimum_required(VERSION 3.10)
project(rudp_embedded_template_compile LANGUAGES CXX)

set(RUDP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(RUDP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(RUDP_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../.. rudp_src)

add_library(rudp_template_compile_esp32 STATIC ../../examples/esp32/esp32_udp.cpp)
target_link_libraries(rudp_template_compile_esp32 PRIVATE rudp::rudp)
target_compile_features(rudp_template_compile_esp32 PRIVATE cxx_std_11)
target_compile_definitions(rudp_template_compile_esp32 PRIVATE RUDP_TEMPLATE_COMPILE_ONLY)

add_library(rudp_template_compile_stm32 STATIC ../../examples/stm32/stm32_f4_udp.cpp)
target_link_libraries(rudp_template_compile_stm32 PRIVATE rudp::rudp)
target_compile_features(rudp_template_compile_stm32 PRIVATE cxx_std_11)

add_custom_target(embedded-template-compile
    DEPENDS rudp_template_compile_esp32 rudp_template_compile_stm32)
