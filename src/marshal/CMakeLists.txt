add_library(marshal_package marshal.hpp marshal.cpp)

cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH MY_PARENT_DIR)

target_include_directories(marshal_package PUBLIC ${MY_PARENT_DIR})
target_link_libraries(marshal_package SEAL::seal)