

# create the testing file and list of tests
create_test_sourcelist(Tests
        math_utils_tests.cpp
        left_int_vec_multiply.cpp
        right_vec_multiply.cpp
        evaluator_wrapper_test.cpp
        query_expander_test.cpp
        mult_benchmark.cpp
        frievalds_test.cpp
        async_mat_mult.cpp
        threadpool_test.cpp
        thrashing_test.cpp
        benchmarks.cpp
        )

# add the executable
add_executable(distribicom_math_utils_tests ${Tests})
set_target_properties(
        distribicom_math_utils_tests
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test
)

target_link_libraries(distribicom_math_utils_tests multiplication_utils distribicom_cpp test_utils)

# remove the test driver source file
set(TestsToRun ${Tests})
remove(TestsToRun math_utils_tests.cpp)

# Add all the ADD_TEST for each test
foreach (test ${TestsToRun})
    get_filename_component(TName ${test} NAME_WE)
    add_test(NAME "math_utils/${TName}" COMMAND distribicom_math_utils_tests ${TName})
endforeach ()