# Store the old value of the 'CMAKE_RUNTIME_OUTPUT_DIRECTORY'
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_OLD ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
# Make subproject to use 'BUILD_SHARED_LIBS=ON' setting.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/deps CACHE INTERNAL "send deps binaries off")

if(USE_PRECOMPILED_SEAL)
    message(STATUS "cmake prefix path is ${CMAKE_PREFIX_PATH}")
    find_package(SEAL 4.0.0 REQUIRED)
else()
    message("seal dir is ${SEAL_DIR}")
    # get seal if no seal_dir
    if((NOT SEAL_DIR))
        message("Getting dependencies... ")
        # get dependencies (just seal for now)
        configure_file(dependencies/FetchSeal.cmake seal_download/CMakeLists.txt)

        # get seal
        execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
                RESULT_VARIABLE result
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/seal_download )
        if(result)
            message(FATAL_ERROR "CMake step for seal failed: ${result}")
        endif()

        # build seal
        execute_process(COMMAND ${CMAKE_COMMAND} --build .
                RESULT_VARIABLE result
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/seal_download )
        if(result)
            message(FATAL_ERROR "Build step for seal failed: ${result}")
        endif()
        set(SEAL_DIR ${CMAKE_BINARY_DIR}/seal_download/_deps/com_microsoft_seal-build/cmake CACHE STRING "" FORCE)
        message("seal dir is ${SEAL_DIR}")
    endif()
    find_package(SEAL 4.0.0 REQUIRED PATH ${SEAL_DIR})

    message("seal dir is ${SEAL_DIR}")
    set(SEAL_DIR ${SEAL_DIR} CACHE STRING "" FORCE)
endif()

# get sealpir
include(dependencies/FetchSealPIR.cmake)

include(dependencies/FetchGRPC.cmake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_OLD} CACHE PATH "send deps binaries off done" FORCE)