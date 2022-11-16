add_library(multiplication_utils matrix_operations.hpp matrix_operations.cpp evaluator_wrapper.hpp evaluator_wrapper.cpp query_expander.hpp query_expander.cpp matrix.h)

cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH MY_PARENT_DIR)

target_include_directories(multiplication_utils PUBLIC ${MY_PARENT_DIR})
target_link_libraries(multiplication_utils concurrency_utils SEAL::seal)