
add_subdirectory(concurrency)
add_subdirectory(math_utils)
add_subdirectory(marshal)
add_subdirectory(services)
add_subdirectory(internals)

add_library(distribicom_cpp INTERFACE)


target_include_directories(distribicom_cpp INTERFACE marshal math_utils services ${com_sealpir_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(distribicom_cpp INTERFACE marshal_package multiplication_utils sealpir services)


add_executable(main1 main.cpp)

set_target_properties(
        main1
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/examples
)
#target_include_directories(main1 PUBLIC ${com_sealpir_SOURCE_DIR}/src)
target_link_libraries(main1 distribicom_cpp)

# eval executables:
add_executable(main_server server.cpp)
add_executable(worker worker.cpp)

add_executable(worker_bench worker_benchmark.cpp)

target_link_libraries(main_server distribicom_cpp)
target_link_libraries(worker distribicom_cpp)

target_link_libraries(worker_bench distribicom_cpp)

add_executable(scalar_mult scalar_mult_benchmark.cpp)
target_link_libraries(scalar_mult distribicom_cpp)
