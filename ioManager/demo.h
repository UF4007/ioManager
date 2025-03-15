#pragma once
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
                    bool loop = true;
                    io::fsm<void>& fsm = co_await io::get_fsm;
                    
                    // Create a simple pipeline: socket >> socket
                    // This pipeline reads data from the socket and writes it back to the socket
                    auto pipeline = io::pipeline<>() >> socket >> socket;
                    
                    // Start the pipeline and set up error handling callback
                    auto started_pipeline = std::move(pipeline).start(
                        [&loop](int which, bool output_or_input) {
                            std::cerr << "Pipeline error in segment " << which 
                                      << (output_or_input ? " (output)" : " (input)") << std::endl;
                            loop = false;
                        }
                    );
                    
                    // Drive the pipeline in a loop
                    while (loop) {
                        started_pipeline <= co_await +started_pipeline;
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
        bool loop = true;
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

        // Send initial message
        std::string data = "Hello from TCP client! " + std::to_string(std::time(nullptr));
        std::span<char> data_span(data.data(), data.size());

        io::future send_future = client << data_span;
        co_await send_future;

        if (send_future.getErr())
        {
            std::cerr << "Error while sending data: " << send_future.getErr().message() << std::endl;
            break;
        }

        // Create a pipeline: client >> client
        // This pipeline reads data from the client and writes it back to the client
        auto pipeline = io::pipeline<>() >> client >> client;
        
        // Start the pipeline and set up error handling callback
        auto started_pipeline = std::move(pipeline).start(
            [&loop](int which, bool output_or_input) {
                std::cerr << "Pipeline error in segment " << which 
                          << (output_or_input ? " (output)" : " (input)") << std::endl;
                loop = false;
            }
        );
        
        // Drive the pipeline in a loop
        while (loop) {
            started_pipeline <= co_await +started_pipeline;
        }
        
        std::cout << "Client session completed." << std::endl;
    }
    
    co_return;
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

    // Create a pipeline: socket >> socket
    // This pipeline reads data from the socket and writes it back to the socket
    auto pipeline = io::pipeline<>() >> socket >> socket;
    
    // Start the pipeline and set up error handling callback
    auto started_pipeline = std::move(pipeline).start(
        [](int which, bool output_or_input) {
            std::cerr << "Pipeline error in segment " << which 
                      << (output_or_input ? " (output)" : " (input)") << std::endl;
        }
    );
    
    // Drive the pipeline in a loop
    while (1) {
        started_pipeline <= co_await +started_pipeline;
    }
    
    co_return;
}

io::fsm_func<void> coro_udp_echo_with_adapter()
{
    io::fsm<void>& fsm = co_await io::get_fsm;
    std::cout << "UDP Echo Server with Adapter started on port 12346..." << std::endl;

    io::sock::udp socket(fsm);
    
    if (!socket.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), 12346))) {
        std::cerr << "Failed to bind UDP socket to port 12346" << std::endl;
        co_return;
    }

    // Create a pipeline with adapter: socket >> [adapter] >> socket
    // The adapter is used to log received data and peer address
    auto pipeline = io::pipeline<>() >> socket >> 
        // Adapter from socket output to socket input
        [](const std::pair<std::span<char>, asio::ip::udp::endpoint>& recv_data) -> std::optional<std::pair<std::span<char>, asio::ip::udp::endpoint>> {
            // Destructure received data and peer address
            const auto& [data, peer] = recv_data;
            
            // Log received data
            std::string message(data.data(), data.size());
            std::cout << "Received message from " << peer << ": " << message << std::endl;
            
            // Return original data and peer address without modification
            return recv_data;
        } >> socket;
    
    // Start the pipeline and set up error handling callback
    auto started_pipeline = std::move(pipeline).start(
        [](int which, bool output_or_input) {
            std::cerr << "Pipeline error in segment " << which 
                      << (output_or_input ? " (output)" : " (input)") << std::endl;
        }
    );
    
    // Drive the pipeline in a loop
    while (1) {
        started_pipeline <= co_await +started_pipeline;
    }
    
    co_return;
}

io::fsm_func<void> coro_pipeline_test() {

    // Protocol implementations for testing

    // 1. Direct Output Protocol - outputs data directly without future
    struct DirectOutputProtocol {
        using prot_output_type = std::string;

        std::vector<std::string> data = { "Hello", "World", "Testing", "Pipeline" };
        size_t current_index = 0;

        void operator>>(std::string& output) {
            // Direct output to the reference
            output = data[current_index];
            current_index = (current_index + 1) % data.size();
            std::cout << "DirectOutputProtocol produced: " << output << std::endl;
        }
    };

    // 2. Future Output Protocol - outputs data through a future
    struct FutureOutputProtocol {
        using prot_output_type = int;

        int counter = 0;

        io::manager* mngr;
        FutureOutputProtocol(io::manager* mngr) :mngr(mngr) {}

        void operator>>(io::future_with<int>& fut) {
            // Output through future
            fut.data = counter++;
            std::cout << "FutureOutputProtocol produced: " << fut.data << std::endl;
            auto promise = mngr->make_future(fut, &counter);
            promise.resolve();
        }
    };

    // 3. Direct Input Protocol - accepts data directly without returning a future
    struct DirectInputProtocol {
        using prot_input_type = std::string;

        void operator<<(const std::string& input) {
            // Process input directly
            std::cout << "DirectInputProtocol received: " << input << std::endl;
        }
    };

    // 4. Future Input Protocol - accepts data and returns a future
    struct FutureInputProtocol {
        using prot_input_type = int;
        
        io::manager* mngr;
        FutureInputProtocol(io::manager* mngr) :mngr(mngr) {}
        
        io::future operator<<(const int& input) {
            // Process input and return a future
            std::cout << "FutureInputProtocol received: " << input << std::endl;
            
            // Create a future to return
            io::future fut;
            io::promise<void> prom = mngr->make_future(fut);
            
            // Resolve the promise immediately for this example
            prom.resolve();
            return fut;
        }
    };

    // 5. Bidirectional Protocol with Direct I/O - implements both direct input and output
    struct DirectBidirectionalProtocol {
        using prot_input_type = std::string;
        using prot_output_type = std::string;

        std::vector<std::string> data = { "Processed", "Data", "From", "Bidirectional" };
        size_t current_index = 0;

        void operator>>(std::string& output) {
            // Direct output
            output = data[current_index];
            current_index = (current_index + 1) % data.size();
            std::cout << "DirectBidirectionalProtocol produced: " << output << std::endl;
        }

        void operator<<(const std::string& input) {
            // Direct input
            std::cout << "DirectBidirectionalProtocol received: " << input << std::endl;
        }
    };

    // 6. Bidirectional Protocol with Future I/O - implements both future input and output
    struct FutureBidirectionalProtocol {
        using prot_input_type = int;
        using prot_output_type = int;
        
        int counter = 100;
        io::manager* mngr;
        
        FutureBidirectionalProtocol(io::manager* mngr) :mngr(mngr) {}
        
        void operator>>(io::future_with<int>& fut) {
            // Output through future
            fut.data = counter++;
            std::cout << "FutureBidirectionalProtocol produced: " << fut.data << std::endl;
            io::promise<void> prom = mngr->make_future(fut);
            prom.resolve();
        }
        
        io::future operator<<(const int& input) {
            // Process input and return a future
            std::cout << "FutureBidirectionalProtocol received: " << input << std::endl;
            
            // Create a future to return
            io::future fut;
            io::promise<void> prom = mngr->make_future(fut);
            
            // Resolve the promise immediately for this example
            prom.resolve();
            return fut;
        }
    };

    // 7. Mixed Bidirectional Protocol - direct output and future input
    struct MixedBidirectionalProtocol1 {
        using prot_input_type = int;
        using prot_output_type = std::string;
        
        std::vector<std::string> data = { "Mixed", "Protocol", "Direct", "Output" };
        size_t current_index = 0;
        io::manager* mngr;
        
        MixedBidirectionalProtocol1(io::manager* mngr) :mngr(mngr) {}

        void operator>>(io::future_with<std::string>& fut) {
            // Future output
            fut.data = std::to_string(current_index++);
            std::cout << "MixedBidirectionalProtocol1 produced: " << fut.data << std::endl;
            io::promise<void> prom = mngr->make_future(fut);
            prom.resolve();
        }
        
        io::future operator<<(const int& input) {
            // Future input
            std::cout << "MixedBidirectionalProtocol1 received: " << input << std::endl;
            
            // Create a future to return
            io::future fut;
            io::promise<void> prom = mngr->make_future(fut);
            
            // Resolve the promise immediately for this example
            prom.resolve();
            return fut;
        }
    };

    // 8. Mixed Bidirectional Protocol - future output and direct input
    struct MixedBidirectionalProtocol2 {
        using prot_input_type = std::string;
        using prot_output_type = int;
        
        int counter = 200;
        io::manager* mngr;
        
        MixedBidirectionalProtocol2(io::manager* mngr) :mngr(mngr) {}
        
        void operator>>(io::future_with<int>& fut) {
            // Future output
            fut.data = counter++;
            std::cout << "MixedBidirectionalProtocol2 produced: " << fut.data << std::endl;
            io::promise<void> prom = mngr->make_future(fut);
            prom.resolve();
        }
        
        void operator<<(const std::string& input) {
            // Direct input
            std::cout << "MixedBidirectionalProtocol2 received: " << input << std::endl;
        }
    };

    io::fsm<void>& fsm = co_await io::get_fsm;
    
    std::cout << "\n=== Testing Pipeline Connections ===\n" << std::endl;
    
    // Test 1: DirectOutput -> DirectInput is not allowed in the pipeline
    // According to the README, two direct protocols cannot be directly connected
    // We need to use at least one future-based protocol
    
    // Test 2: FutureOutput -> DirectInput
    std::cout << "\n--- Test 2: FutureOutput -> DirectInput ---\n" << std::endl;
    {
        FutureOutputProtocol output_prot(fsm.getManager());
        DirectInputProtocol input_prot;
        
        // Need an adapter to convert int to string
        auto pipeline = io::pipeline<>() >> output_prot
            >> [](const int& value) -> std::optional<std::string> {
            return "Number: " + std::to_string(value);
            } 
        >> input_prot;
        auto started_pipeline = std::move(pipeline).start();
        
        for (int i = 0; i < 3; i++) {
            std::cout << "Cycle " << i+1 << ":" << std::endl;
            started_pipeline <= co_await +started_pipeline;
        }
    }
    
    // Test 3: DirectOutput -> FutureInput
    std::cout << "\n--- Test 3: DirectOutput -> FutureInput ---\n" << std::endl;
    {
        DirectOutputProtocol output_prot;
        FutureInputProtocol input_prot(fsm.getManager());
        
        // Need an adapter to convert string to int
        auto pipeline = io::pipeline<>() >> output_prot >> 
            [](const std::string& value) -> std::optional<int> {
                return static_cast<int>(value.length());
            } >> input_prot;
        auto started_pipeline = std::move(pipeline).start();
        
        for (int i = 0; i < 3; i++) {
            std::cout << "Cycle " << i+1 << ":" << std::endl;
            started_pipeline <= co_await +started_pipeline;
        }
    }
    
    // Test 4: FutureOutput -> FutureInput
    std::cout << "\n--- Test 4: FutureOutput -> FutureInput ---\n" << std::endl;
    {
        FutureOutputProtocol output_prot(fsm.getManager());
        FutureInputProtocol input_prot(fsm.getManager());
        
        auto pipeline = io::pipeline<>() >> output_prot >> input_prot;
        auto started_pipeline = std::move(pipeline).start();
        
        for (int i = 0; i < 10; i++) {
            std::cout << "Cycle " << i+1 << ":" << std::endl;
            started_pipeline <= co_await +started_pipeline;
        }
    }
    
    // Test 5: FutureOutput -> FutureBidirectional -> FutureInput
    std::cout << "\n--- Test 5: FutureOutput -> FutureBidirectional -> FutureInput ---\n" << std::endl;
    {
        FutureOutputProtocol output_prot(fsm.getManager());
        FutureBidirectionalProtocol middle_prot(fsm.getManager());
        FutureBidirectionalProtocol middle_prot2(fsm.getManager());
        FutureInputProtocol input_prot(fsm.getManager());
        
        auto pipeline = io::pipeline<>() >> output_prot >> middle_prot >> middle_prot2 >> input_prot;
        auto started_pipeline = std::move(pipeline).start();
        
        for (int i = 0; i < 10; i++) {
            std::cout << "Cycle " << i+1 << ":" << std::endl;
            started_pipeline <= co_await +started_pipeline;
        }
    }
    
    // Test 6: Mixed protocols with adapters
    std::cout << "\n--- Test 6: Mixed protocols with adapters ---\n" << std::endl;
    {
        DirectOutputProtocol output_prot;
        MixedBidirectionalProtocol1 middle_prot1(fsm.getManager());
        MixedBidirectionalProtocol2 middle_prot2(fsm.getManager());
        FutureInputProtocol input_prot(fsm.getManager());
        
        auto pipeline = io::pipeline<>() >> output_prot >> 
            [](const std::string& value) -> std::optional<int> {
                return static_cast<int>(value.length());
            } >> middle_prot1 >> 
            [](const std::string& value) -> std::optional<std::string> {
                return "Transformed: " + value;
            } >> middle_prot2 >> input_prot;
            auto started_pipeline = std::move(pipeline).start();
        
        for (int i = 0; i < 10; i++) {
            std::cout << "Cycle " << i+1 << ":" << std::endl;
            started_pipeline <= co_await +started_pipeline;
        }
    }
    
    std::cout << "\n=== Pipeline Testing Complete ===\n" << std::endl;
    
    co_return;
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

    // udp echo with adapter
    if (false)
        mngr.spawn_later(coro_udp_echo_with_adapter()).detach();

    // pipeline
    if (false)
        mngr.spawn_later(coro_pipeline_test()).detach();

    while (1)
    {
        mngr.drive();
    }
}