add_library(concurrency_utils concurrency.h channel.hpp counter.hpp counter.cpp promise.hpp threadpool.hpp threadpool.cpp
        safelatch.h safelatch.cpp)
cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH MY_PARENT_DIR)
target_include_directories(concurrency_utils PUBLIC ${MY_PARENT_DIR})
target_link_libraries(concurrency_utils SEAL::seal)