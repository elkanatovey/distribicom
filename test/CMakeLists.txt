
add_library(test_utils test_utils.hpp test_utils.cpp)

target_include_directories(test_utils PUBLIC ${SealPIR_SOURCE_DIR}/src)
target_include_directories(test_utils PUBLIC ${distribicom_SOURCE_DIR}/src)
target_include_directories(test_utils PUBLIC ${distribicom_SOURCE_DIR}/src/math_utils)
target_include_directories(test_utils PUBLIC ${distribicom_SOURCE_DIR}/src/services)

target_include_directories(test_utils PUBLIC ${distribicom_SOURCE_DIR}/src/marshal)

target_link_libraries(test_utils sealpir services multiplication_utils marshal_package)

add_subdirectory(general_tests)
add_subdirectory(math_utils)
add_subdirectory(marshal)
add_subdirectory(services)

