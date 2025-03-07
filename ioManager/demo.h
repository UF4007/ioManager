#include <stdio.h>
#include "ioManager.h"

io::fsm_func<io::awaitable> coro_demo(int initial)
{
    io::fsm<io::awaitable> &fsm = co_await io::get_fsm;
    io::fsm_handle<io::awaitable> task_handle;
    if (initial)
    {
        task_handle = fsm.getManager()->spawn_later(coro_demo(initial - 1));
        co_await *task_handle;
        std::cout << initial << std::endl;
    }
    if (fsm->operator bool())
        fsm->resume();
}

io::fsm_func<void> coro_limit_test(int num)
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    for (int i = 0; i < num; i++)
    {
        fsm.spawn_now([]() -> io::fsm_func<void>
                      {
            io::fsm<void>& fsm = co_await io::get_fsm;
            io::clock cl1, cl2;
            io::future fut;
            while (1)
            {
                fsm.make_clock(cl1, std::chrono::seconds(1));
                fsm.make_clock(cl2, std::chrono::seconds(2));
                io::promise prom = fsm.make_future(fut);
                io_select(
                    cl1, {
                    }, 
                    cl2, {
                    },
                    fut, {
                    });
            } }())
            .detach();
    }
}

io::fsm_func<void> coro_multi_thread()
{
    auto& fsm = co_await io::get_fsm;

    co_return;
}

io::fsm_func<void> coro_chan(size_t *count)
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    io::chan<char> chan = fsm.make_chan<char>(1000);
    io::fsm_handle<void> handle;

    // is zero-copy ?
    if (true && count == nullptr)
    {
        handle = fsm.spawn_now([](io::chan_r<char> chan, size_t *count) -> io::fsm_func<void>
                               {
            io::fsm<void>& fsm = co_await io::get_fsm;
            while (1)
            {
                io::chan<char>::span_guard recv;        //zero copy span guard object.
                co_await(chan >> recv);
                if (count)
                    *count += recv.span.size();
                else
                    std::cout << recv.span.data() << std::endl;
            } }(chan, count));
    }
    else
    {
        handle = fsm.spawn_now([](io::chan_r<char> ch, size_t *count) -> io::fsm_func<void>
                               {
            std::string str;
            str.resize(1024 * 10);
            io::fsm<void>& fsm = co_await io::get_fsm;
            while (1)
            {
                io::future_with<std::span<char>> futur;
                ch.get_and_copy(std::span<char>(str), futur);
                co_await futur;
                if (count)
                    *count += str.size();
                else
                    std::cout << str << std::endl;
            } }(chan, count));
    }

    while (1)
    {
        char send[] = "this is a very long long string. ";
        std::span<char> send_span(send, sizeof(send));
        co_await (chan << send_span);
    }
}

io::fsm_func<void> coro_chan_benchmark()
{
    constexpr size_t TEST_SECONDS = 3;
    io::fsm<void> &fsm = co_await io::get_fsm;
    size_t byte_count = 0;
    io::fsm_handle<void> h = fsm.spawn_now(coro_chan(&byte_count));
    io::clock clock;
    fsm.make_clock(clock, std::chrono::seconds(TEST_SECONDS));
    co_await clock;

    double mb_total = byte_count / (1024.0 * 1024.0);
    double mb_per_second = mb_total / TEST_SECONDS;

    std::cout << "Total data transmitted in " << TEST_SECONDS << " seconds: " << std::fixed << std::setprecision(2)
              << mb_total << " MB" << std::endl;
    std::cout << "Average throughput: " << mb_per_second << " MB/s" << std::endl;

    fsm.getManager()->spawn_later(coro_chan_benchmark()).detach();
}

io::fsm_func<void> coro_benchmark()
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

    fsm.getManager()->spawn_later(coro_benchmark()).detach();
}

io::fsm_func<void> coro_tcp_echo_server()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    io::tcp::acceptor acceptor(12345);
    std::cout << "TCP Echo Server started, waiting for connections on port 12345..." << std::endl;

    while (1)
    {
        co_await acceptor.wait_accept(fsm);
        io::tcp::socket sock;
        if (acceptor.accept(sock))
        {
            std::cout << "New connection accepted!" << std::endl;
            fsm.spawn_now(
                   [](io::tcp::socket socket) -> io::fsm_func<void>
                   {
                       while (1)
                       {
                           io::fsm<void> &fsm = co_await io::get_fsm;
                           co_await socket.wait_read(fsm);
                           auto read_err = socket.readGetErr();
                           if (read_err)
                           {
                               std::cerr << "Error while reading from socket: " << read_err.message() << std::endl;
                               co_return;
                           }

                           auto read_sp = socket.read();
                           io::async_future afut = socket.wait_send(fsm, read_sp);
                           co_await afut;
                           if (afut.getErr())
                           {
                               std::cerr << "Error while sending data to socket: " << afut.getErr().message() << std::endl;
                               co_return;
                           }

                           std::cout << "Echoed data back to client!" << std::endl;
                       }
                   }(std::move(sock)))
                .detach();
        }
        else
        {
            std::cerr << "Failed to accept connection!" << std::endl;
        }
    }
    co_return;
    std::cout << "Server stopped." << std::endl;
}

io::fsm_func<void> coro_tcp_echo_client()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    std::cout << "TCP Echo Client started." << std::endl;

    while (1)
    {
        io::tcp::socket client;
        io::future fut = client.connect(fsm, asio::ip::tcp::endpoint(asio::ip::address::from_string("192.168.142.1"), 12345));
        co_await fut;

        if (fut.getErr())
        {
            std::cerr << "Failed to connect to server: " << fut.getErr().message() << std::endl;
            io::clock clock;
            fsm.make_clock(clock, std::chrono::seconds(1));
            co_await clock;
            continue;
        }

        std::cout << "Connected to server at " << client.remote_endpoint() << std::endl;

        while (1)
        {
            std::cout << "Local endpoint: " << client.local_endpoint() << std::endl;

            io_select(client.wait_read(fsm), {});
            auto read_err = client.readGetErr();
            if (read_err)
            {
                std::cerr << "Error while waiting for data to read: " << read_err.message() << std::endl;
                break;
            }

            auto read_sp = client.read();
            io::future afut = client.wait_send(fsm, read_sp);
            co_await afut;

            if (afut.getErr())
            {
                std::cerr << "Error while sending data: " << afut.getErr().message() << std::endl;
                break;
            }

            std::cout << "Sent data to server." << std::endl;
        }
    }
}

io::fsm_func<void> coro_udp_echo()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    std::cout << "UDP Echo Server started on port 12345..." << std::endl;

    while (1)
    {
        io::udp::socket client;
        asio::error_code ec;
        client.open(asio::ip::udp::v4(), ec);

        if (ec)
        {
            std::cerr << "Failed to open UDP socket: " << ec.message() << std::endl;
            break;
        }

        client.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), 12345), ec);

        if (ec)
        {
            std::cerr << "Failed to bind UDP socket: " << ec.message() << std::endl;
            break;
        }

        co_await client.wait_read(fsm);
        auto read_err = client.readGetErr();
        if (read_err)
        {
            std::cerr << "Error while reading from UDP socket: " << read_err.message() << std::endl;
            break;
        }

        auto [read_sp, peer] = client.read();
        std::cout << "Received message from " << peer << std::endl;

        io::future afut = client.wait_send(fsm, read_sp, peer);
        co_await afut;

        if (afut.getErr())
        {
            std::cerr << "Error while sending message back to peer: " << afut.getErr().message() << std::endl;
            break;
        }

        std::cout << "Echoed message back to " << peer << std::endl;
    }
}

io::fsm_func<void> coro_down_timer() {
    auto& fsm = co_await io::get_fsm;
    io::timer::down timer(10);
    timer.start(std::chrono::seconds(1));
    while (timer.isReach() == false)
    {
        std::cout << "tick" << std::endl;
        co_await timer.await_tm(fsm);
    }
    std::cout << "end" << std::endl;
}

void io_testmain_v3()
{
    io::manager mngr;

    // test of basic coroutine method
    if (false)
        mngr.spawn_later(coro_demo(100)).detach();

    // limit test
    if (false)
        mngr.spawn_later(coro_limit_test(10000000)).detach();

    // test of multi-thread
    if (false)
        mngr.spawn_later(coro_multi_thread()).detach();

    // test of chan
    if (false)
    {
        if (true)
            mngr.spawn_later(coro_chan_benchmark()).detach();
        else
            mngr.spawn_later(coro_chan(nullptr)).detach();
    }

    // coroutine benchmark
    if (true)
        mngr.spawn_later(coro_benchmark()).detach();

    // tcp echo server
    if (false)
        mngr.spawn_later(coro_tcp_echo_server()).detach();

    // tcp echo client
    if (false)
        mngr.spawn_later(coro_tcp_echo_client()).detach();

    // udp echo
    if (false)
        mngr.spawn_later(coro_udp_echo()).detach();

    // compensated timer test
    if (false)
        mngr.spawn_later(coro_down_timer()).detach();

    while (1)
    {
        mngr.drive();
    }
}