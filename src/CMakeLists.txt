find_package(SEAL 4.0.0 REQUIRED)

add_subdirectory(math_utils)
add_subdirectory(marshal)
add_subdirectory(network_layer)

add_library(distribicom_cpp distributed_pir.hpp distributed_pir.cpp master.hpp master.cpp worker.hpp worker.cpp
        FreivaldsVector.hpp FreivaldsVector.cpp)

target_include_directories(distribicom_cpp PUBLIC marshal math_utils ${com_sealpir_SOURCE_DIR}/src)
target_link_libraries(distribicom_cpp marshal_package multiplication_utils sealpir)


add_executable(main1 main.cpp)
#target_include_directories(main1 PUBLIC ${com_sealpir_SOURCE_DIR}/src)
target_link_libraries(main1 distribicom_cpp)
