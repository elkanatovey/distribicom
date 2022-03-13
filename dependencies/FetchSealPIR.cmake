cmake_minimum_required(VERSION 3.13)

project(sealpir_download NONE)

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/seal_download/_deps/com_microsoft_seal-build/cmake)
include(FetchContent)
FetchContent_Declare(
        com_sealpir
        GIT_REPOSITORY https://github.com/elkanatovey/sealpir-distribicom.git
        GIT_TAG        e68e52c7d5fc9fea26287cfe4828d9de293b3c01 #distribicom sealpir version
        USES_TERMINAL_DOWNLOAD ON
)

if(NOT com_sealpir_POPULATED)
    FetchContent_MakeAvailable(com_sealpir)
endif()
