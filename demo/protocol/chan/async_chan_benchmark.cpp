#include <cstdio>
#include <iostream>
#include <iomanip>
#include <string>
#include <atomic>
#include "../../../ioManager/all.h"

io::fsm_func<void> async_chan(std::atomic<size_t> *count, std::atomic<size_t>* throughput, size_t thread_pool_sum)
{
    constexpr size_t PRODUCERS = 16;
    constexpr size_t CONSUMERS = 16;
    io::pool thread_pool(thread_pool_sum); // Create a thread pool with 4 threads
    io::fsm<void> &fsm = co_await io::get_fsm;
    io::async::chan<char> chan(fsm.getManager(), 1024 * 16);
    io::async::semaphore stop_singal(fsm.getManager(), 0);
    IO_DEFER = [&]() {
        chan.close();
        while (stop_singal.try_acquire(PRODUCERS + CONSUMERS) == false);    // Stackless coroutines make things complicated, spinning locks make things easy.
        };

    // Launch multiple producers
    for (int i = 0; i < PRODUCERS; ++i) {
        thread_pool.async_spawn([](io::async::chan<char> ch, io::async::semaphore stop_singal) -> io::fsm_func<void> {
            io::fsm<void>& fsm = co_await io::get_fsm;
            ch.setManager(fsm.getManager());
            char send[] = "this is a very long long string for async channel test.";
            std::span<char> send_span(send, sizeof(send));
            while (1) {
                co_await (ch << send_span);
                if (ch.isClosed())
                    break;
            }
            stop_singal.release();
            co_return;
        }(chan, stop_singal));
    }

    // Launch multiple consumers
    for (int i = 0; i < CONSUMERS; ++i) {
        thread_pool.async_spawn([](io::async::chan<char> ch, io::async::semaphore stop_singal, std::atomic<size_t>* count, std::atomic<size_t>* throughput) -> io::fsm_func<void> {
            io::fsm<void>& fsm = co_await io::get_fsm;
            ch.setManager(fsm.getManager());
            std::string str;
            str.resize(1024 * 16);
            while (1) {
                co_await ch.listen();
                std::span<char> str_span(str.data(), str.size());
                size_t read_size = ch.accept(str_span);
                if (throughput && count)
                {
                    *throughput += read_size;
                    (*count)++;
                }
                else
                {
                    std::cout << str.substr(0, read_size) << std::endl;
                }
                if (ch.isClosed())
                    break;
            }
            stop_singal.release();
            co_return;
        }(chan, stop_singal, count, throughput));
    }

    io::future block_forever;
    fsm.make_future(block_forever);
    co_await block_forever;

    co_return;
}

io::fsm_func<void> async_chan_benchmark()
{
    constexpr size_t TEST_SECONDS = 3;
    io::fsm<void> &fsm = co_await io::get_fsm;
    std::atomic<size_t> exchange_count = 0;
    std::atomic<size_t> byte_count = 0;

    // Start the async channel test with the thread pool
    io::fsm_handle<void> h = fsm.spawn_now(async_chan(&exchange_count, &byte_count, 8));
    io::clock clock;
    fsm.make_clock(clock, std::chrono::seconds(TEST_SECONDS));
    co_await clock;

    double mb_total = byte_count / (1024.0 * 1024.0);
    double mb_per_second = mb_total / TEST_SECONDS;
    double exchange_per_second = (double)exchange_count / TEST_SECONDS;
    const double NANOSECONDS_PER_SECOND = 1e9;
    double nanoseconds_per_operation = NANOSECONDS_PER_SECOND / exchange_per_second / 2;

    std::cout << "Async Channel Test Results:" << std::endl;
    std::cout << "Total data transmitted in " << TEST_SECONDS << " seconds: " << std::fixed << std::setprecision(2)
        << mb_total << " MB" << std::endl;
    std::cout << "Average throughput: " << mb_per_second << " MB/s" << std::endl;
    std::cout << "Total data exchange: " << exchange_count << std::endl;
    std::cout << "Average latency: " << nanoseconds_per_operation << " ns/op" << std::endl;

    fsm.getManager()->spawn_later(async_chan_benchmark()).detach();
}

int main()
{
    io::manager mngr;
    mngr.async_spawn(async_chan_benchmark());

    while (1)
    {
        mngr.drive();
    }

    return 0;
} 