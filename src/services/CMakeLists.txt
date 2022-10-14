add_library(services worker.hpp worker.cpp)

target_link_libraries(services distribicom_grpc SEAL::seal)
