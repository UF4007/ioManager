#include <ioManager/ioManager.h>
#include <ioManager/protocol/chan.h>

io::fsm_func<void> chan(size_t *count, size_t* throughput)
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    io::chan<char> chan(fsm, 1000);
    io::fsm_handle<void> handle;

    handle = fsm.spawn_now([](io::chan_r<char> ch, size_t* count, size_t* throughput) -> io::fsm_func<void>
        {
            std::string str;
            str.resize(64);
            io::fsm<void>& fsm = co_await io::get_fsm;
            while (1)
            {
                co_await ch.get_and_copy(std::span<char>(str));
                if (count)
                {
                    (*count)++;
                    *throughput += str.size();
                }
                else
                    std::cout << str << std::endl;
            } }(chan, count, throughput));

    while (1)
    {
        char send[] = "this is a very long long string. ";
        std::span<char> send_span(send, sizeof(send));
        co_await (chan << send_span);
    }
    co_return;
}

io::fsm_func<void> chan_benchmark()
{
    constexpr size_t TEST_SECONDS = 3;
    io::fsm<void> &fsm = co_await io::get_fsm;
    size_t exchange_count = 0;
    size_t byte_count = 0;
    io::fsm_handle<void> h = fsm.spawn_now(chan(&exchange_count, &byte_count));
    io::clock clock;
    fsm.make_clock(clock, std::chrono::seconds(TEST_SECONDS));
    co_await clock;

    double mb_total = byte_count / (1024.0 * 1024.0);
    double mb_per_second = mb_total / TEST_SECONDS;
    double exchange_per_second = (double)exchange_count / TEST_SECONDS;
    const double NANOSECONDS_PER_SECOND = 1e9;
    double nanoseconds_per_operation = NANOSECONDS_PER_SECOND / exchange_per_second / 2;

    std::cout << "Sync Channel Test Results:" << std::endl;
    std::cout << "Total data transmitted in " << TEST_SECONDS << " seconds: " << std::fixed << std::setprecision(2)
        << mb_total << " MB" << std::endl;
    std::cout << "Average throughput: " << mb_per_second << " MB/s" << std::endl;
    std::cout << "Total data exchange: " << exchange_count << std::endl;
    std::cout << "Average latency: " << nanoseconds_per_operation << " ns/op" << std::endl;

    fsm.getManager()->spawn_later(chan_benchmark()).detach();
}

int main()
{
    io::manager mngr;
    mngr.async_spawn(chan_benchmark());

    while (1)
    {
        mngr.drive();
    }

    return 0;
} 