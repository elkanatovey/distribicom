

# create the testing file and list of tests
create_test_sourcelist(Tests
        distribicom_services_tests.cpp
        worker_test.cpp
        client_test.cpp
        db_many_reads_test.cpp
        work_distribution_test.cpp
        )

# add the executable
add_executable(distribicom_services_tests ${Tests})
set_target_properties(
        distribicom_services_tests
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test
)

target_link_libraries(distribicom_services_tests services multiplication_utils distribicom_cpp test_utils)

# remove the test driver source file
set(TestsToRun ${Tests})
remove(TestsToRun distribicom_services_tests.cpp)

# Add all the ADD_TEST for each test
foreach (test ${TestsToRun})
    get_filename_component(TName ${test} NAME_WE)
    add_test(NAME "services/${TName}" COMMAND distribicom_services_tests ${TName})
endforeach ()