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


###################################################

# get dependencies (just sealpir for now)
configure_file(dependencies/FetchSealPIR.cmake seal_download/CMakeLists.txt)

# get sealpir
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/seal_download )
if(result)
    message(FATAL_ERROR "CMake step for sealpir failed: ${result}")
endif()

# build sealpir
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/seal_download )
if(result)
    message(FATAL_ERROR "Build step for sealpir failed: ${result}")
endif()