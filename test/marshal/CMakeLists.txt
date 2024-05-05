


# create the testing file and list of tests
create_test_sourcelist(Tests
        marshal_tests.cpp
        basic_test.cpp
        all_functions_test.cpp
        )

# add the executable
add_executable(distribicom_marshal_tests ${Tests})
set_target_properties(
        distribicom_marshal_tests
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test
)
target_link_libraries(distribicom_marshal_tests multiplication_utils test_utils)

# remove the test driver source file
set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun marshal_tests.cpp)

# Add all the ADD_TEST for each test
foreach (test ${TestsToRun})
    get_filename_component(TName ${test} NAME_WE)
    add_test(NAME "marshal/${TName}" COMMAND distribicom_marshal_tests ${TName})
endforeach ()