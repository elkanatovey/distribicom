add_library(marshal_package marshal.hpp marshal.cpp)

target_link_libraries(marshal_package SEAL::seal)