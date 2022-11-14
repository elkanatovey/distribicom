add_library(services worker.hpp factory.hpp factory.cpp worker.cpp constants.hpp manager.hpp manager.cpp
        server.hpp server.cpp db.hpp db.cpp utils.hpp utils.cpp)

target_link_libraries(services distribicom_grpc boot_grpc SEAL::seal multiplication_utils marshal_package)