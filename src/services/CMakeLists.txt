add_library(services worker.hpp worker.cpp factory.hpp)

target_link_libraries(services distribicom_grpc boot_grpc SEAL::seal multiplication_utils marshal_package)