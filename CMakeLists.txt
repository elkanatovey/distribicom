set(CMAKE_CXX_FLAGS "-static -O3")
option(DONT_COMPILE_DEPENDENCIES "GET_SEAL" OFF)

option(USE_PRECOMPILED_SEAL "Use preinstalled seal" OFF)
option(USE_PREINSTALLED_GRPC "Use preinstalled gRPC" OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

cmake_minimum_required(VERSION 3.20)
unset(CMAKE_CXX_COMPILER CACHE)
project(distribicom VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories("${PROJECT_SOURCE_DIR}/util" "${PROJECT_BINARY_DIR}/util")
add_subdirectory(util)

include(dependencies/FetchDependencies.cmake)

add_subdirectory(src)

enable_testing()
add_subdirectory(test)

