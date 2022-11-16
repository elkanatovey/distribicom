add_library(marshal_package marshal.hpp marshal.cpp)

target_include_directories(marshal_package PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(marshal_package SEAL::seal)