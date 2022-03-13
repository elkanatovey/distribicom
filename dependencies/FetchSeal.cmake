#cmake_minimum_required(VERSION 3.13)
#
#project(seal_download NONE)
#
#
#include(FetchContent)
#FetchContent_Declare(
#        com_microsoft_seal
#        GIT_REPOSITORY https://github.com/microsoft/SEAL
#        GIT_TAG        79234726053c45eede688400aa219fdec0810bd8 #v3.7.2
#)
#
#if(NOT com_microsoft_seal_POPULATED)
#    FetchContent_MakeAvailable(com_microsoft_seal)
#endif()
