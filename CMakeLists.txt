cmake_minimum_required(VERSION 3.10)
project(udplink VERSION 0.1.1 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(RUDP_BUILD_TESTS "Build rudp self test" ON)
option(RUDP_INSTALL "Enable install/export targets" ON)
option(RUDP_BUILD_EXAMPLES "Build rudp examples" ON)
option(COVERAGE "Enable coverage flags (GCC/Clang)" OFF)

add_library(rudp STATIC
    src/rudp.cpp
    src/manager.cpp
)

add_library(rudp::rudp ALIAS rudp)

target_include_directories(rudp PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(rudp PUBLIC cxx_std_11)
target_compile_options(rudp PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

if(COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g --coverage")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    else()
        message(WARNING "COVERAGE=ON is only supported for GCC/Clang")
    endif()
endif()

set_target_properties(rudp PROPERTIES
    OUTPUT_NAME rudp
    EXPORT_NAME rudp
)

if(RUDP_BUILD_TESTS)
    add_executable(rudp_self_test tests/self_test.cpp)
    target_link_libraries(rudp_self_test PRIVATE rudp)
    add_executable(rudp_bench tests/bench.cpp)
    target_link_libraries(rudp_bench PRIVATE rudp)
    add_executable(rudp_manager_test tests/manager_test.cpp)
    target_link_libraries(rudp_manager_test PRIVATE rudp)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        add_custom_target(clang-tidy
            COMMAND ${CLANG_TIDY_EXE}
                    src/rudp.cpp src/manager.cpp
                    tests/self_test.cpp tests/manager_test.cpp tests/bench.cpp
                    examples/basic_endpoint.cpp examples/basic_manager.cpp
                    -p ${CMAKE_BINARY_DIR}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Running clang-tidy"
        )
    endif()
endif()

if(RUDP_BUILD_EXAMPLES)
    add_executable(rudp_example_endpoint examples/basic_endpoint.cpp)
    target_link_libraries(rudp_example_endpoint PRIVATE rudp)
    add_executable(rudp_example_manager examples/basic_manager.cpp)
    target_link_libraries(rudp_example_manager PRIVATE rudp)
    if(WIN32)
        add_executable(rudp_example_winsock examples/socket_winsock.cpp)
        target_link_libraries(rudp_example_winsock PRIVATE rudp ws2_32)
    else()
        add_executable(rudp_example_posix examples/socket_posix.cpp)
        target_link_libraries(rudp_example_posix PRIVATE rudp)
    endif()
endif()

if(RUDP_INSTALL)
    install(TARGETS rudp
        EXPORT rudpTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(DIRECTORY include/rudp
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hpp"
    )

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/rudpConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/rudpConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/rudpConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/rudp"
    )

    install(EXPORT rudpTargets
        FILE rudpTargets.cmake
        NAMESPACE rudp::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/rudp"
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/rudpConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/rudpConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/rudp"
    )

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/rudp.pc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/rudp.pc"
        @ONLY
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/rudp.pc"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
endif()
