cmake_minimum_required(VERSION 3.20)
project(knight LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED)
include(CTest)

set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)

set(ABI_DECODER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ABI_DECODER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ABI_DECODER_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(external/spdlog)
add_subdirectory(external/solabi)
add_subdirectory(src)

if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    add_subdirectory(external/googletest)
    add_subdirectory(tests)
endif()
