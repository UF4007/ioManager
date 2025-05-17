#include <cstdio>
#include <iostream>
#include <string>
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

int main()
{
    io::manager mngr;
    mngr.async_spawn(chan(nullptr, nullptr));

    while (1)
    {
        mngr.drive();
    }

    return 0;
} 