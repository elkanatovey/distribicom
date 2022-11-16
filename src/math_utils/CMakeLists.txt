add_library(multiplication_utils matrix_multiplier.hpp matrix_multiplier.cpp evaluator_wrapper.hpp evaluator_wrapper.cpp query_expander.hpp query_expander.cpp matrix.h channel.h)

target_include_directories(multiplication_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(multiplication_utils SEAL::seal)