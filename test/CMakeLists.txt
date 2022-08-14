
add_library(test_utils test_utils.hpp test_utils.cpp)

target_include_directories(test_utils PUBLIC ${SealPIR_SOURCE_DIR}/src)
target_include_directories(test_utils PUBLIC ${distribicom_SOURCE_DIR}/src)
target_include_directories(test_utils PUBLIC ${distribicom_SOURCE_DIR}/src/math_utils)
target_include_directories(test_utils PUBLIC ${distribicom_SOURCE_DIR}/src/marshal)

target_link_libraries(test_utils sealpir multiplication_utils marshal_package)

add_subdirectory(general_tests)
add_subdirectory(math_utils)
add_subdirectory(marshal)

# create the testing file and list of tests
create_test_sourcelist(Tests
        CommonTests.cpp
        db_serialisation_test.cpp
        distributed_freivalds_verification_test.cpp
        freivalds_verification_test.cpp
        distribute_query_ser_test.cpp
        distribute_query_test.cpp
        )

# add the executable
add_executable(CommonTests ${Tests})
target_link_libraries(CommonTests distribicom_cpp test_utils)

# remove the test driver source file
set(TestsToRun ${Tests})
remove(TestsToRun CommonTests.cpp)

# Add all the ADD_TEST for each test
foreach (test ${TestsToRun})
    get_filename_component(TName ${test} NAME_WE)
    add_test(NAME ${TName} COMMAND CommonTests ${TName})
endforeach ()

