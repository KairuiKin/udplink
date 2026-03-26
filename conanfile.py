from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout


class RudpConan(ConanFile):
    name = "rudp"
    version = "0.1.1"
    license = "Apache-2.0"
    url = "https://gitee.com/wanghaonan199105240070/udplink"
    description = "MCU-friendly reliable UDP library in C++11"
    package_type = "library"
    settings = "os", "arch", "compiler", "build_type"
    exports_sources = "CMakeLists.txt", "include/*", "src/*", "cmake/*"
    generators = "CMakeToolchain", "CMakeDeps"

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure(variables={"RUDP_BUILD_TESTS": False})
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["rudp"]
