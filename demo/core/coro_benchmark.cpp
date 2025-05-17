#include <cstdio>
#include <iostream>
#include <vector>
#include <chrono>
#include "../../ioManager/all.h"

io::fsm_func<void> benchmark()
{
    // High speed low drag!
    constexpr size_t NUM_COROS = 30000;        // num of coroutines
    constexpr size_t TOTAL_SWITCHES = 3000000; // switch sum

    io::fsm<void> &fsm = co_await io::get_fsm;
    std::vector<io::fsm_handle<io::promise<>>> test_coros;

    io::timer::up timer;
    timer.start();

    // prepare for coroutine loop
    for (size_t i = 0; i < NUM_COROS; i++)
    {
        test_coros.push_back(
            fsm.spawn_now([]() -> io::fsm_func<io::promise<>>
                          {
                io::fsm<io::promise<>>& fsm = co_await io::get_fsm;
                io::future future;
                while(1)
                {
                    *fsm = fsm.make_future(future);
                    co_await future;
                } }()));
    }

    auto duration_coro = std::chrono::duration_cast<std::chrono::microseconds>(timer.lap());

    // start loop
    for (size_t i = 0; i < TOTAL_SWITCHES; i++)
    {
        size_t ind = i % NUM_COROS;
        test_coros[ind]->resolve();
    }

    auto duration_loop = std::chrono::duration_cast<std::chrono::microseconds>(timer.lap());
    double creates_per_sec = NUM_COROS * 1000000.0 / duration_coro.count();
    double switches_per_sec = TOTAL_SWITCHES * 1000000.0 / duration_loop.count();

    std::cout << "Benchmark Results:\n"
              << "Number of coroutines: " << NUM_COROS << "\n"
              << "Total time: " << duration_coro.count() / 1000.0 << " ms\n"
              << "Spawn per second: " << static_cast<size_t>(creates_per_sec) << "\n"
              << "Average spawn time: " << duration_coro.count() / double(NUM_COROS) * 1000.0 << " ns\n\n"
              << "Total switches: " << TOTAL_SWITCHES << "\n"
              << "Total time: " << duration_loop.count() / 1000.0 << " ms\n"
              << "Switches per second: " << static_cast<size_t>(switches_per_sec) << "\n"
              << "Average switch time: " << duration_loop.count() / double(TOTAL_SWITCHES) * 1000.0 << " ns\n\n\n\n";

    fsm.getManager()->spawn_later(benchmark()).detach();
}

int main()
{
    io::manager mngr;
    mngr.async_spawn(benchmark());

    while (1)
    {
        mngr.drive();
    }

    return 0;
} 