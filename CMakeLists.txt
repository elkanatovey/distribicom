set( CMAKE_CXX_FLAGS "-Wall -Wpedantic -Wextra -m64 -O3")
option(DONT_COMPILE_DEPENDENCIES "GET_SEAL" OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

cmake_minimum_required(VERSION 3.20)
project(distribicom VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(dependencies/FetchDependencies.cmake)

add_subdirectory(src)

enable_testing()
add_subdirectory(test)

