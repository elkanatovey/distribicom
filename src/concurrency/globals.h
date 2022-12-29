#pragma once

#include <thread>

namespace concurrency {
    // States the number of CPUs the concurrency module will use.
    extern std::uint64_t num_cpus; // defined in threadpool.cpp
}