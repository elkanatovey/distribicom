add_library(multiplication_utils matrix_multiplier.hpp matrix_multiplier.cpp evaluator_wrapper.hpp evaluator_wrapper.cpp query_expander.hpp query_expander.cpp)

target_include_directories(multiplication_utils PUBLIC ${com_sealpir_SOURCE_DIR}/src)

target_link_libraries(multiplication_utils SEAL::seal)