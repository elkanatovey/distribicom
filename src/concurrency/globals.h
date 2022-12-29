#pragma once

#include <thread>

namespace concurrency {
    // States the number of CPUs the concurrency module will use.
    std::uint64_t num_cpus = std::thread::hardware_concurrency();
}