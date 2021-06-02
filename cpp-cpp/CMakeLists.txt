cmake_minimum_required(VERSION 3.14)
project(cpp-cpp)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set (CMAKE_CXX_STANDARD 17)

include(conanbuildinfo.cmake)
conan_basic_setup()
find_package(Protobuf REQUIRED)

add_subdirectory(src)
