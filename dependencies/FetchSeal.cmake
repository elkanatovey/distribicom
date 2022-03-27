cmake_minimum_required(VERSION 3.13)

project(seal_download NONE)


include(FetchContent)
FetchContent_Declare(
        com_microsoft_seal
        GIT_REPOSITORY https://github.com/microsoft/SEAL
        GIT_TAG        a0fc0b732f44fa5242593ab488c8b2b3076a5f76 #v4.0.0
)

if(NOT com_microsoft_seal_POPULATED)
    FetchContent_MakeAvailable(com_microsoft_seal)
endif()
