

# create the testing file and list of tests
create_test_sourcelist (Tests
        general_tests.cpp
        associativity_addition_test.cpp
        associativity_mult_test.cpp
        matrix_mult_order_test.cpp
        matrix_mult_str_test.cpp
        )

# add the executable
add_executable (distribicom_general_tests ${Tests})
set_target_properties(
        distribicom_general_tests
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test
)
target_link_libraries(distribicom_general_tests multiplication_utils test_utils)

# remove the test driver source file
set (TestsToRun ${Tests})
remove (TestsToRun general_tests.cpp)

# Add all the ADD_TEST for each test
foreach (test ${TestsToRun})
    get_filename_component (TName ${test} NAME_WE)
    add_test (NAME "general_tests/${TName}" COMMAND distribicom_general_tests ${TName})
endforeach ()