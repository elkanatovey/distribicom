set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DMY_DEBUG")

include_directories(${SealPIR_SOURCE_DIR}/src)
include_directories(${distribicom_SOURCE_DIR}/src)
include_directories(${distribicom_SOURCE_DIR}/src/math_utils)

add_subdirectory(math_utils)

# create the testing file and list of tests
create_test_sourcelist (Tests
        CommonTests.cpp
        db_serialisation_test.cpp
        distributed_freivalds_verification_test.cpp
        freivalds_verification_test.cpp
        distribute_query_ser_test.cpp
        distribute_query_test.cpp
        associativity_addition_test.cpp
        associativity_mult_test.cpp
        matrix_mult_order_test.cpp
        matrix_mult_order_test_version_2.cpp
        matrix_mult_str_test.cpp
        )

# add the executable
add_executable (CommonTests ${Tests} "test_utils.cpp")
target_link_libraries(CommonTests distribicom_cpp)

# remove the test driver source file
set (TestsToRun ${Tests})
remove (TestsToRun CommonTests.cpp)

# Add all the ADD_TEST for each test
foreach (test ${TestsToRun})
    get_filename_component (TName ${test} NAME_WE)
    add_test (NAME ${TName} COMMAND CommonTests ${TName})
endforeach ()

