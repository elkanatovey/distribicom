add_library(services worker.hpp worker.cpp)

target_link_libraries(services SEAL::seal)
