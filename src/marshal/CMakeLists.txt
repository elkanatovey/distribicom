add_library(marshal_package marshal.hpp marshal.cpp)

#target_include_directories(marshal_package PRIVATE ${com_sealpir_SOURCE_DIR}/src)
target_link_libraries(marshal_package SEAL::seal)