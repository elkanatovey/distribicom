set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DMY_DEBUG")


# create the testing file and list of tests
create_test_sourcelist (Tests
        general_tests.cpp
        associativity_addition_test.cpp
        associativity_mult_test.cpp
        matrix_mult_order_test.cpp
        matrix_mult_order_test_version_2.cpp
        matrix_mult_str_test.cpp
        )

# add the executable
add_executable (general_tests ${Tests})
target_link_libraries(general_tests multiplication_utils test_utils)

# remove the test driver source file
set (TestsToRun ${Tests})
remove (TestsToRun general_tests.cpp)

# Add all the ADD_TEST for each test
foreach (test ${TestsToRun})
    get_filename_component (TName ${test} NAME_WE)
    add_test (NAME "math_utils/${TName}" COMMAND general_tests ${TName})
endforeach ()