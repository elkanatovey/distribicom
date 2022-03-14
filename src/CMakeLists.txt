find_package(SEAL 3.7.2 REQUIRED)


add_library(distribicom_cpp distributed_pir.hpp distributed_pir.cpp  master_server.hpp master_server.cpp clientside_server.hpp clientside_server.cpp)

target_include_directories(distribicom_cpp PUBLIC ${com_sealpir_SOURCE_DIR}/src)
target_link_libraries(distribicom_cpp sealpir)


add_executable(main1 main.cpp)
#target_include_directories(main1 PUBLIC ${com_sealpir_SOURCE_DIR}/src)
target_link_libraries(main1 distribicom_cpp)
