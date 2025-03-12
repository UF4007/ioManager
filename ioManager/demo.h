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

io::fsm_func<void> coro_tcp_echo_server()
{
    io::fsm<void>& fsm = co_await io::get_fsm;
    io::sock::tcp_accp acceptor(fsm);
    std::cout << "TCP Echo Server started, waiting for connections on port 12345..." << std::endl;

    if (!acceptor.bind_and_listen(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 12345))) {
        std::cerr << "Failed to bind and listen on port 12345!" << std::endl;
        co_return;
    }

    while (1)
    {
        io::future_with<std::optional<io::sock::tcp>> accept_future;
        acceptor >> accept_future;
        co_await accept_future;
        
        if (accept_future.getErr()) {
            std::cerr << "Error while accepting connection: " << accept_future.getErr().message() << std::endl;
            continue;
        }

        if (accept_future.data.has_value()) {
            std::cout << "New connection accepted!" << std::endl;
            io::sock::tcp socket = std::move(accept_future.data.value());
            
            fsm.spawn_now(
                [](io::sock::tcp socket) -> io::fsm_func<void>
                {
                    io::fsm<void>& fsm = co_await io::get_fsm;
                    
                    while (1)
                    {
                        io::future_with<std::span<char>> read_future;
                        socket >> read_future;
                        co_await read_future;
                        
                        if (read_future.getErr()) {
                            std::cerr << "Error while reading from socket: " << read_future.getErr().message() << std::endl;
                            co_return;
                        }

                        io::future send_future = socket << read_future.data;
                        co_await send_future;
                        
                        if (send_future.getErr()) {
                            std::cerr << "Error while sending data to socket: " << send_future.getErr().message() << std::endl;
                            co_return;
                        }

                        std::cout << "Echoed data back to client!" << std::endl;
                    }
                }(std::move(socket)))
                .detach();
        }
        else {
            std::cerr << "Failed to accept connection!" << std::endl;
        }
    }
    co_return;
    std::cout << "Server stopped." << std::endl;
}

io::fsm_func<void> coro_tcp_echo_client()
{
    io::fsm<void>& fsm = co_await io::get_fsm;
    std::cout << "TCP Echo Client started." << std::endl;

    while (1)
    {
        io::sock::tcp client(fsm);
        io::future connect_future = client.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 12345));
        co_await connect_future;

        if (connect_future.getErr())
        {
            std::cerr << "Failed to connect to server: " << connect_future.getErr().message() << std::endl;
            io::clock clock;
            fsm.make_clock(clock, std::chrono::seconds(1));
            co_await clock;
            continue;
        }

        std::cout << "Connected to server at " << client.remote_endpoint() << std::endl;

        std::cout << "Local endpoint: " << client.local_endpoint() << std::endl;

        std::string data = "Hello from TCP client! " + std::to_string(std::time(nullptr));
        std::span<char> data_span(data.data(), data.size());

        io::future send_future = client << data_span;
        co_await send_future;

        if (send_future.getErr())
        {
            std::cerr << "Error while sending data: " << send_future.getErr().message() << std::endl;
            break;
        }

        while (1)
        {
            io::future_with<std::span<char>> read_future;
            client >> read_future;
            co_await read_future;

            if (read_future.getErr()) {
                std::cerr << "Error while reading from socket: " << read_future.getErr().message() << std::endl;
                co_return;
            }

            io::future send_future = client << read_future.data;
            co_await send_future;

            if (send_future.getErr()) {
                std::cerr << "Error while sending data to socket: " << send_future.getErr().message() << std::endl;
                co_return;
            }

            std::cout << "Echoed data back to client!" << std::endl;
        }
    }
}

io::fsm_func<void> coro_udp_echo()
{
    io::fsm<void>& fsm = co_await io::get_fsm;
    std::cout << "UDP Echo Server started on port 12345..." << std::endl;

    io::sock::udp socket(fsm);
    
    if (!socket.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), 12345))) {
        std::cerr << "Failed to bind UDP socket to port 12345" << std::endl;
        co_return;
    }

    while (1)
    {
        io::future_with<std::pair<std::span<char>, asio::ip::udp::endpoint>> recv_future;
        socket >> recv_future;
        co_await recv_future;
        
        if (recv_future.getErr()) {
            std::cerr << "Error while reading from UDP socket: " << recv_future.getErr().message() << std::endl;
            continue;
        }

        auto& [data, peer] = recv_future.data;
        std::string message(data.data(), data.size());
        std::cout << "Received message from " << peer << ": " << message << std::endl;

        io::future send_future = socket << std::make_pair(data, peer);
        co_await send_future;

        if (send_future.getErr()) {
            std::cerr << "Error while sending message back to peer: " << send_future.getErr().message() << std::endl;
            continue;
        }

        std::cout << "Echoed message back to " << peer << std::endl;
    }
}

io::fsm_func<void> coro_pipeline_test() {
    io::fsm<void>& fsm = co_await io::get_fsm;
    io::sock::tcp x(fsm);
    io::sock::udp y(fsm);
    auto pipeline = io::pipeline<>() >> x >> [](const std::span<char>& str) ->std::optional<std::pair<std::span<char>, asio::ip::udp::endpoint>> {

        } >> y >> [](const std::pair<std::span<char>, asio::ip::udp::endpoint>& rec) -> std::optional<std::span<char>> {
            } >> x;
        while (1)
            pipeline <= co_await +pipeline;
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
    if (false)
        mngr.spawn_later(coro_benchmark()).detach();

    // compensated timer test
    if (false)
        mngr.spawn_later(coro_down_timer()).detach();

    // tcp echo server
    if (false)
        mngr.spawn_later(coro_tcp_echo_server()).detach();

    // tcp echo client
    if (false)
        mngr.spawn_later(coro_tcp_echo_client()).detach();

    // udp echo
    if (false)
        mngr.spawn_later(coro_udp_echo()).detach();

    // pipeline
    if (true)
        mngr.spawn_later(coro_pipeline_test()).detach();

    while (1)
    {
        mngr.drive();
    }
}