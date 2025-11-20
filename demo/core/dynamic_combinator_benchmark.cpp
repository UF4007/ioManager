#include <ioManager/ioManager.h>
#include <ioManager/timer.h>
#include <iostream>
#include <vector>
#include <chrono>

constexpr size_t NUM_FUTURES = 100; // number of futures to combine
// Inflection point at ~50 futures: future::race is faster below, combinator is faster above
constexpr size_t NUM_ITERATIONS = 100000; // number of iterations

// Test dynamic_combinator vs io::future::race performance
io::fsm_func<void> benchmark_single_triggered()
{

    io::fsm<void> &fsm = co_await io::get_fsm;

    io::timer::up timer;
    io::promise trigger;
    std::array<io::future, NUM_FUTURES> futures;

    // Push futures
    for (size_t i = 0; i < NUM_FUTURES; i++)
    {
        trigger = fsm.make_future(futures[i]);
    }

    // Benchmark dynamic_combinator
    timer.start();
    {
        io::dynamic_combinator<int> combinator(io::combinator_t::race, fsm);
        for (size_t i = 0; i < NUM_FUTURES; i++)
        {
            combinator.push(std::move(futures[i]));
        }
        for (size_t iter = 0; iter < NUM_ITERATIONS; iter++)
        {
            trigger.resolve();

            // Get combined future (should be immediately resolved)
            auto combined = combinator.get_future();

            co_await combined;

            // Pop results
            size_t result_count = 0;
            while (auto result = combinator.finished_out())
            {
                result_count++;
            }

            io::future fut;
            trigger = fsm.make_future(fut);
            combinator.push(std::move(fut));
        }
    }
    auto combinator_time = std::chrono::duration_cast<std::chrono::microseconds>(timer.lap());

    // Push futures
    for (size_t i = 0; i < NUM_FUTURES; i++)
    {
        trigger = fsm.make_future(futures[i]);
    }

    // Benchmark io::future::race
    timer.start();
    for (size_t iter = 0; iter < NUM_ITERATIONS; iter++)
    {
        trigger.resolve();

        // Use io::future::race with std::apply to combine all futures
        // This generates the race call at compile time with all NUM_FUTURES futures
        co_await std::apply(
            [](auto &&...futs)
            {
                return io::future::race(std::forward<decltype(futs)>(futs)...);
            },
            futures);

        trigger = fsm.make_future(futures[NUM_FUTURES - 1]);
    }
    auto race_time = std::chrono::duration_cast<std::chrono::microseconds>(timer.lap());

    // Print results
    std::cout << "\n========== Test 1: Single Triggered (Race Mode) ==========\n"
              << "Number of futures per iteration: " << NUM_FUTURES << "\n"
              << "Number of iterations: " << NUM_ITERATIONS << "\n\n"
              << "dynamic_combinator (race mode, " << NUM_FUTURES << " futures):\n"
              << "  Total time: " << combinator_time.count() / 1000.0 << " ms\n"
              << "  Time per iteration: " << combinator_time.count() / double(NUM_ITERATIONS) << " us\n"
              << "  Time per future: " << combinator_time.count() / double(NUM_ITERATIONS * NUM_FUTURES) << " us\n\n"
              << "io::future::race (std::apply with " << NUM_FUTURES << " futures):\n"
              << "  Total time: " << race_time.count() / 1000.0 << " ms\n"
              << "  Time per iteration: " << race_time.count() / double(NUM_ITERATIONS) << " us\n"
              << "  Time per future: " << race_time.count() / double(NUM_ITERATIONS * NUM_FUTURES) << " us\n\n";

    if (combinator_time.count() > 0 && race_time.count() > 0)
    {
        double ratio = static_cast<double>(combinator_time.count()) / race_time.count();
        std::cout << "Performance Ratio (combinator / race): " << std::fixed << std::setprecision(2) << ratio << "x\n"
                  << "(dynamic_combinator is " << (ratio < 1 ? "faster" : "slower") << " than io::future::race)\n";
    }

    co_return;
}

// Test dynamic_combinator vs io::future::allSettle performance
io::fsm_func<void> benchmark_all_triggered()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    io::manager *manager = fsm.getManager();

    io::timer::up timer;

    // Benchmark dynamic_combinator
    timer.start();
    for (size_t iter = 0; iter < NUM_ITERATIONS; iter++)
    {
        io::dynamic_combinator<int> combinator(io::combinator_t::allSettle, manager);

        // Push futures
        for (size_t i = 0; i < NUM_FUTURES; i++)
        {
            io::future fut;
            io::promise<void> prom = manager->make_future(fut);
            prom.resolve();
            combinator.push(std::move(fut));
        }

        // Get combined future (should be immediately resolved)
        auto combined = combinator.get_future();

        // Pop results
        size_t result_count = 0;
        while (auto result = combinator.finished_out())
        {
            result_count++;
        }
    }
    auto combinator_time = std::chrono::duration_cast<std::chrono::microseconds>(timer.lap());

    // Benchmark io::future::allSettle
    timer.start();
    for (size_t iter = 0; iter < NUM_ITERATIONS; iter++)
    {
        std::array<io::future, NUM_FUTURES> futures;

        // Create and resolve futures
        for (size_t i = 0; i < NUM_FUTURES; i++)
        {
            io::future fut;
            io::promise<void> prom = manager->make_future(fut);
            prom.resolve();
            futures[i] = std::move(fut);
        }

        // Use io::future::allSettle with std::apply to combine all futures
        // This generates the allSettle call at compile time with all NUM_FUTURES futures
        auto race_result = std::apply(
            [](auto &&...futs)
            {
                return io::future::allSettle(std::forward<decltype(futs)>(futs)...);
            },
            futures);
    }
    auto race_time = std::chrono::duration_cast<std::chrono::microseconds>(timer.lap());

    // Print results
    std::cout << "\n========== Test 2: All Triggered (Already Resolved) ==========\n"
              << "Number of futures per iteration: " << NUM_FUTURES << "\n"
              << "Number of iterations: " << NUM_ITERATIONS << "\n\n"
              << "dynamic_combinator (allSettle mode, " << NUM_FUTURES << " futures):\n"
              << "  Total time: " << combinator_time.count() / 1000.0 << " ms\n"
              << "  Time per iteration: " << combinator_time.count() / double(NUM_ITERATIONS) << " us\n"
              << "  Time per future: " << combinator_time.count() / double(NUM_ITERATIONS * NUM_FUTURES) << " us\n\n"
              << "io::future::allSettle (std::apply with " << NUM_FUTURES << " futures):\n"
              << "  Total time: " << race_time.count() / 1000.0 << " ms\n"
              << "  Time per iteration: " << race_time.count() / double(NUM_ITERATIONS) << " us\n"
              << "  Time per future: " << race_time.count() / double(NUM_ITERATIONS * NUM_FUTURES) << " us\n\n";

    if (combinator_time.count() > 0 && race_time.count() > 0)
    {
        double ratio = static_cast<double>(combinator_time.count()) / race_time.count();
        std::cout << "Performance Ratio (combinator / race): " << std::fixed << std::setprecision(2) << ratio << "x\n"
                  << "(dynamic_combinator is " << (ratio < 1 ? "faster" : "slower") << " than io::future::allSettle)\n";
    }

    co_return;
}

int main()
{
    io::manager mngr;
    mngr.async_spawn(benchmark_single_triggered());
    mngr.async_spawn(benchmark_all_triggered());

    while (1)
    {
        mngr.drive();
    }

    return 0;
}
