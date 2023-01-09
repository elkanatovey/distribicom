#include "../test_utils.hpp"
#include "matrix_operations.hpp"


int threadpool_test(int a, char *b[]) {
    (void) a;
    (void) b;

    constexpr int num_tasks = 1000;

    // Number of times to repeat the benchmark
    constexpr int num_reps = 50;
    auto num_work_per_thead = 100000;

    // used for sleep in thread:
    // Minimum and maximum execution times for the tasks
    constexpr int min_exec_time = 1;
    constexpr int max_exec_time = 5;

    // Seed the random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min_exec_time, max_exec_time);

    // Create the thread pool
    concurrency::threadpool pool(std::thread::hardware_concurrency());

    // Vector to store the execution times
    std::vector<int> executionTimes;
    executionTimes.reserve(num_tasks * num_reps);

    // Benchmark loop
    for (int i = 0; i < num_reps; i++) {
        std::cout << "starting iteration " << i + 1 << " out of " << num_reps << std::endl;
        // Start the timer
        auto latch = std::make_shared<concurrency::safelatch>(num_tasks);
        auto startTime = std::chrono::high_resolution_clock::now();

        // Enqueue the tasks
        for (int j = 0; j < num_tasks; j++) {
            pool.submit(
                {
                    .f =[&] {
                        std::uint64_t x = 1;
                        // Sleep for a random amount of time
                        for (int k = 0; k < num_work_per_thead; ++k) {
                            x += dis(gen);
                        }
                    },
                    .wg = latch,
                }
            );
        }
        latch->wait();
    }

    return 0;
}


