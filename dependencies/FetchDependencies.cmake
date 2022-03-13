message(".zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz ")
if(NOT DONT_COMPILE_DEPENDENCIES)
    message("...using Foo found in ")
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

    list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/seal_download/_deps/com_microsoft_seal-build/cmake)
    # get sealpir
    include(dependencies/FetchSealPIR.cmake)

endif()
