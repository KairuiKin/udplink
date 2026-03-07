cmake_minimum_required(VERSION 3.10)
project(udplink LANGUAGES CXX)

option(RUDP_BUILD_TESTS "Build rudp self test" ON)

add_library(rudp STATIC
    src/rudp.cpp
)

target_include_directories(rudp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(rudp PUBLIC cxx_std_11)
target_compile_options(rudp PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

if(RUDP_BUILD_TESTS)
    add_executable(rudp_self_test tests/self_test.cpp)
    target_link_libraries(rudp_self_test PRIVATE rudp)
    add_executable(rudp_bench tests/bench.cpp)
    target_link_libraries(rudp_bench PRIVATE rudp)
endif()
