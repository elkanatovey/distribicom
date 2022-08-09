set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DMY_DEBUG")


# create the testing file and list of tests
create_test_sourcelist(Tests
        marshal_tests.cpp
        basic_test.cpp
        all_functions_test.cpp
        )

# add the executable
add_executable(marshal_tests ${Tests})
target_link_libraries(marshal_tests multiplication_utils test_utils)

# remove the test driver source file
set(TestsToRun ${Tests})
remove(TestsToRun marshal_tests.cpp)

# Add all the ADD_TEST for each test
foreach (test ${TestsToRun})
    get_filename_component(TName ${test} NAME_WE)
    add_test(NAME "marshal/${TName}" COMMAND marshal_tests ${TName})
endforeach ()