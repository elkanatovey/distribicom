set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DMY_DEBUG")


# create the testing file and list of tests
create_test_sourcelist (Tests
        CommonCxxTests.cpp
        left_int_vec_multiply.cpp
        right_vec_multiply.cpp
        evaluator_wrapper_test.cpp
        )

# add the executable
add_executable (CommonCppTests ${Tests} "../test_utils.cpp")
target_link_libraries(CommonCppTests multiplication_utils)

# remove the test driver source file
set (TestsToRun ${Tests})
remove (TestsToRun CommonCxxTests.cpp)

# Add all the ADD_TEST for each test
foreach (test ${TestsToRun})
    get_filename_component (TName ${test} NAME_WE)
    add_test (NAME "math_utils/${TName}" COMMAND CommonCppTests ${TName})
endforeach ()