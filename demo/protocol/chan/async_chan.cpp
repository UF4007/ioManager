#include <cstdio>
#include <iostream>
#include <string>
#include <atomic>
#include "../../../ioManager/all.h"

io::fsm_func<void> async_chan(std::atomic<size_t> *count, std::atomic<size_t>* throughput, size_t thread_pool_sum)
{
    constexpr size_t PRODUCERS = 16;
    constexpr size_t CONSUMERS = 16;
    io::pool thread_pool(thread_pool_sum); // Create a thread pool with thread_pool_sum threads
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

int main()
{
    io::manager mngr;
    mngr.async_spawn(async_chan(nullptr, nullptr, 4));

    while (1)
    {
        mngr.drive();
    }

    return 0;
} 