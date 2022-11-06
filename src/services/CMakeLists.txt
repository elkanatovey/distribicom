add_library(services worker.hpp factory.hpp worker.cpp constants.hpp manager.hpp manager.cpp server.hpp server.cpp)

target_link_libraries(services distribicom_grpc boot_grpc SEAL::seal multiplication_utils marshal_package)