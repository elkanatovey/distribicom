find_package(SEAL 4.0.0 REQUIRED)


add_library(distribicom_cpp distributed_pir.hpp distributed_pir.cpp master.hpp master.cpp worker.hpp worker.cpp)

target_include_directories(distribicom_cpp PUBLIC ${com_sealpir_SOURCE_DIR}/src)
target_link_libraries(distribicom_cpp sealpir)


add_executable(main1 main.cpp)
#target_include_directories(main1 PUBLIC ${com_sealpir_SOURCE_DIR}/src)
target_link_libraries(main1 distribicom_cpp)
