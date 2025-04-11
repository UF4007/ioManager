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
                do {
                    switch (co_await io::future::race(cl1, cl2, fut)) {
                    case 0: {
                    } break; case 1: {
                    } break; case 2: {
                    } break;
                    }
                } while (0);;
            } }())
            .detach();
    }
}

io::fsm_func<void> coro_chan(size_t *count, size_t* throughput)
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

io::fsm_func<void> coro_chan_benchmark()
{
    constexpr size_t TEST_SECONDS = 3;
    io::fsm<void> &fsm = co_await io::get_fsm;
    size_t exchange_count = 0;
    size_t byte_count = 0;
    io::fsm_handle<void> h = fsm.spawn_now(coro_chan(&exchange_count, &byte_count));
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

    fsm.getManager()->spawn_later(coro_chan_benchmark()).detach();
}

io::fsm_func<void> coro_async_chan(std::atomic<size_t> *count, std::atomic<size_t>* throughput, size_t thread_pool_sum)
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

io::fsm_func<void> coro_async_chan_benchmark()
{
    constexpr size_t TEST_SECONDS = 3;
    io::fsm<void> &fsm = co_await io::get_fsm;
    std::atomic<size_t> exchange_count = 0;
    std::atomic<size_t> byte_count = 0;

    // Start the async channel test with the thread pool
    io::fsm_handle<void> h = fsm.spawn_now(coro_async_chan(&exchange_count, &byte_count, 8));
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

    fsm.getManager()->spawn_later(coro_async_chan_benchmark()).detach();
}

io::fsm_func<void> coro_chan_construct_correct_test()
{
    // Class with construction/destruction tracking for testing
    static int constructed = 0;
    static int destroyed = 0;
    static int moved = 0;
    static int copied = 0;
    struct LifetimeTracker {
        int value;

        LifetimeTracker(int val = 0) : value(val) {
            constructed++;
            std::cout << "LifetimeTracker constructed: " << value << " (total: " << constructed << ")" << std::endl;
        }

        LifetimeTracker(const LifetimeTracker& other) : value(other.value) {
            copied++;
            std::cout << "LifetimeTracker copied: " << value << " (total copies: " << copied << ")" << std::endl;
        }

        LifetimeTracker(LifetimeTracker&& other) noexcept : value(other.value) {
            moved++;
            other.value = -1; // Mark as moved
            std::cout << "LifetimeTracker moved: " << value << " (total moves: " << moved << ")" << std::endl;
        }

        LifetimeTracker& operator=(const LifetimeTracker& other) {
            if (this != &other) {
                value = other.value;
                copied++;
                std::cout << "LifetimeTracker copy assigned: " << value << " (total copies: " << copied << ")" << std::endl;
            }
            return *this;
        }

        LifetimeTracker& operator=(LifetimeTracker&& other) noexcept {
            if (this != &other) {
                value = other.value;
                other.value = -1; // Mark as moved
                moved++;
                std::cout << "LifetimeTracker move assigned: " << value << " (total moves: " << moved << ")" << std::endl;
            }
            return *this;
        }

        ~LifetimeTracker() {
            destroyed++;
            std::cout << "LifetimeTracker destroyed: " << value << " (total: " << destroyed << ")" << std::endl;
        }

        static void resetCounters() {
            constructed = 0;
            destroyed = 0;
            moved = 0;
            copied = 0;
        }

        static void printStats() {
            std::cout << "\n--- LifetimeTracker Stats ---" << std::endl;
            std::cout << "Constructed: " << constructed << std::endl;
            std::cout << "Destroyed: " << destroyed << std::endl;
            std::cout << "Moved: " << moved << std::endl;
            std::cout << "Copied: " << copied << std::endl;
            std::cout << "Balance (constructed - destroyed): " << (constructed - destroyed) << std::endl;
            std::cout << "-------------------------\n" << std::endl;
        }
    };

    io::fsm<void>& fsm = co_await io::get_fsm;
    std::cout << "\n=== Starting chan construction/destruction tests ===\n" << std::endl;

    // Test 1: Basic construction and destruction
    std::cout << "Test 1: Basic construction and destruction" << std::endl;
    {
        LifetimeTracker::resetCounters();
        std::cout << "Creating chan with 5 capacity..." << std::endl;
        io::chan<LifetimeTracker> chan(fsm, 5);

        std::cout << "Creating and sending 3 elements..." << std::endl;
        std::array<LifetimeTracker, 3> items = { LifetimeTracker(1), LifetimeTracker(2), LifetimeTracker(3) };
        co_await(chan << std::span<LifetimeTracker>(items));

        std::cout << "Chan before destruction:" << std::endl;
        std::cout << "  Size: " << chan.size() << std::endl;
        std::cout << "  Capacity: " << chan.capacity() << std::endl;
        std::cout << "Channel will be destroyed now..." << std::endl;
    }
    std::cout << "After chan destruction:" << std::endl;
    LifetimeTracker::printStats();

    // Test 2: Testing copy and move construction of chan
    std::cout << "Test 2: Copy and move construction of chan" << std::endl;
    {
        LifetimeTracker::resetCounters();

        std::cout << "Creating original chan..." << std::endl;
        io::chan<LifetimeTracker> original_chan(fsm, 5);

        // Add some elements to the channel
        std::array<LifetimeTracker, 2> items = { LifetimeTracker(10), LifetimeTracker(20) };
        co_await(original_chan << std::span<LifetimeTracker>(items));

        std::cout << "Creating copy constructed chan..." << std::endl;
        io::chan<LifetimeTracker> copy_chan(original_chan);

        std::cout << "Creating move constructed chan..." << std::endl;
        io::chan<LifetimeTracker> move_chan(std::move(copy_chan));

        std::cout << "Stats after construction:" << std::endl;
        std::cout << "  Original size: " << original_chan.size() << std::endl;
        std::cout << "  Moved size: " << move_chan.size() << std::endl;

        // Receive from move_chan to verify data was preserved
        std::array<LifetimeTracker, 2> received;
        co_await move_chan.get_and_copy(std::span<LifetimeTracker>(received));

        std::cout << "Received values from moved chan: ";
        for (const auto& item : received) {
            std::cout << item.value << " ";
        }
        std::cout << std::endl;
    }
    LifetimeTracker::printStats();

    // Test 3: Testing chan_r and chan_s conversion
    std::cout << "Test 3: Testing chan_r and chan_s conversion" << std::endl;
    {
        LifetimeTracker::resetCounters();

        std::cout << "Creating original chan..." << std::endl;
        io::chan<LifetimeTracker> original_chan(fsm, 5);

        // Test conversion to chan_r
        std::cout << "Converting to chan_r..." << std::endl;
        io::chan_r<LifetimeTracker> read_chan = original_chan;

        // Test conversion to chan_s
        std::cout << "Converting to chan_s..." << std::endl;
        io::chan_s<LifetimeTracker> write_chan = original_chan;

        // Add some elements through write_chan
        std::array<LifetimeTracker, 2> items = { LifetimeTracker(30), LifetimeTracker(40) };
        co_await(write_chan << std::span<LifetimeTracker>(items));

        // Read them through read_chan
        std::array<LifetimeTracker, 2> received;
        co_await read_chan.get_and_copy(std::span<LifetimeTracker>(received));

        std::cout << "Received values from chan_r: ";
        for (const auto& item : received) {
            std::cout << item.value << " ";
        }
        std::cout << std::endl;
    }
    LifetimeTracker::printStats();

    // Test 4: Testing ring buffer behavior
    std::cout << "Test 4: Testing ring buffer behavior" << std::endl;
    {
        LifetimeTracker::resetCounters();

        std::cout << "Creating chan with capacity 4..." << std::endl;
        io::chan<LifetimeTracker> chan(fsm, 4);

        // First, fill the buffer
        std::cout << "Filling buffer with 4 elements..." << std::endl;
        std::array<LifetimeTracker, 4> items1 = {
            LifetimeTracker(100), LifetimeTracker(101),
            LifetimeTracker(102), LifetimeTracker(103)
        };
        co_await(chan << std::span<LifetimeTracker>(items1));

        // Read 2 elements
        std::cout << "Reading 2 elements..." << std::endl;
        std::array<LifetimeTracker, 2> received1;
        co_await chan.get_and_copy(std::span<LifetimeTracker>(received1));

        // Add 2 more elements (should wrap around)
        std::cout << "Adding 2 more elements (should wrap around)..." << std::endl;
        std::array<LifetimeTracker, 2> items2 = { LifetimeTracker(104), LifetimeTracker(105) };
        co_await(chan << std::span<LifetimeTracker>(items2));

        // Read all remaining elements
        std::cout << "Reading all remaining elements..." << std::endl;
        std::array<LifetimeTracker, 4> received2;
        co_await chan.get_and_copy(std::span<LifetimeTracker>(received2));

        std::cout << "Read values from first batch: ";
        for (const auto& item : received1) {
            std::cout << item.value << " ";
        }
        std::cout << std::endl;

        std::cout << "Read values from second batch: ";
        for (const auto& item : received2) {
            std::cout << item.value << " ";
        }
        std::cout << std::endl;
    }
    LifetimeTracker::printStats();

    // Test 5: Testing close() behavior
    std::cout << "Test 5: Testing close() behavior" << std::endl;
    {
        LifetimeTracker::resetCounters();

        std::cout << "Creating chan..." << std::endl;
        io::chan<LifetimeTracker> chan(fsm, 3);

        // Add some elements
        std::cout << "Adding elements..." << std::endl;
        std::array<LifetimeTracker, 2> items = { LifetimeTracker(200), LifetimeTracker(201) };
        co_await(chan << std::span<LifetimeTracker>(items));

        std::cout << "Closing channel..." << std::endl;
        chan.close();

        std::cout << "Channel closed, checking counters..." << std::endl;
    }
    LifetimeTracker::printStats();

    std::cout << "\n=== chan construction/destruction tests completed ===\n" << std::endl;
    co_return;
}

io::fsm_func<void> coro_chan_peak_shaving()
{
    io::fsm<void> &fsm = co_await io::get_fsm;

    struct ProducerProtocol {
        using prot_output_type = int;
        
        io::manager* mngr;
        int counter = 0;
        
        ProducerProtocol(io::fsm<void>& fsm) : mngr(fsm.getManager()) {
            fsm_handle = fsm.spawn_now(produce(this));
        }
        
        void operator>>(io::future_with<int>& fut) {
            promise = mngr->make_future(fut, &fut.data);
            fsm_handle->resolve_later();
        }
        
        io::fsm_func<io::promise<>> produce(ProducerProtocol* pthis) {
            io::fsm<io::promise<>> &fsm = co_await io::get_fsm;

            io::future fut;
            
            while (true) {
                *fsm = fsm.make_future(fut);
                co_await fut;

                // idle producer speed
                co_await fsm.setTimeout(std::chrono::milliseconds(200));
                pthis->promise.resolve_later(pthis->counter);

                if (pthis->counter % 30 == 0)
                {
                    for (int i = 0; i < 20; i++)
                    {
                        *fsm = fsm.make_future(fut);
                        co_await fut;

                        // producer boost speed
                        co_await fsm.setTimeout(std::chrono::milliseconds(5));
                        pthis->promise.resolve_later(1);
                    }
                }

                pthis->counter++;
                
                std::cout << "Produced: " << pthis->counter << std::endl;
            }
            
            co_return;
        }
        
        io::promise<int> promise;
        io::fsm_handle<io::promise<>> fsm_handle;
    };

    struct ConsumerProtocol {
        io::manager* mngr;
        io::timer::up timer;
        int value;
        
        ConsumerProtocol(io::fsm<void>& fsm) : mngr(fsm.getManager()) {
            timer.start();
            fsm_handle = fsm.spawn_now(consume(this));
        }
        
        // Input protocol implementation
        io::future operator<<(const int& input) {
            value = input;
            
            io::future fut;
            prom = mngr->make_future(fut);

            fsm_handle->resolve_later();
            return fut;
        }
        
        io::fsm_func<io::promise<>> consume(ConsumerProtocol* pthis) {
            io::fsm<io::promise<>> &fsm = co_await io::get_fsm;

            io::future fut;
            
            while (true) {
                *fsm = fsm.make_future(fut);
                co_await fut;

                co_await fsm.setTimeout(std::chrono::milliseconds(50));

                auto duration = pthis->timer.lap();
                std::cout << "Consumed: " << pthis->value
                    << " (took: "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                    << "ms)" << std::endl;

                pthis->prom.resolve_later();
            }
            
            std::cout << "Consumer finished" << std::endl;
            co_return;
        }
        
        io::promise<> prom;
        io::fsm_handle<io::promise<>> fsm_handle;
    };

    ProducerProtocol producer(fsm);
    ConsumerProtocol consumer(fsm);

    //io::chan<int> ch = io::chan<int>(fsm, 10);
    io::async::chan<int> ch = io::async::chan<int>(fsm, 10);
    
    // With a chan
    auto pipeline = io::pipeline<>() >> producer >> [ch](const int& a)mutable->std::optional<int> {
        std::cout << "Channel capacity: (" << ch.size() << "/" << ch.capacity() << ")" << std::endl;
        return a;
        } >> io::prot::chan(ch) >> consumer;

    // Without chan, the producer will be block during the consumer being await.
    //auto pipeline = io::pipeline<>() >> producer >> consumer;
    
    auto started_pipeline = std::move(pipeline).start();

    while(1)
    {
        //drive the pipeline
        started_pipeline <= co_await +started_pipeline;
    }
    
    co_return;
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
                    io::future end;
                    
                    // Create a simple pipeline: socket >> socket
                    // This pipeline reads data from the socket and writes it back to the socket
                    auto pipeline = io::pipeline<>() >> socket >> socket;
                    
                    // Start the pipeline and set up error handling callback
                    auto started_pipeline = std::move(pipeline).spawn(fsm, 
                        [prom = fsm.make_future(end)](int which, bool output_or_input, std::error_code ec) mutable{
                            std::cerr << "Pipeline error in segment " << which 
                                      << (output_or_input ? " (output)" : " (input)")
                                      << " - Error: " << ec.message()
                                      << " [code: " << ec.value() << "]" << std::endl;
                            prom.resolve_later();
                        }
                    );
                    co_await end;
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
            [&loop](int which, bool output_or_input, std::error_code ec) {
                std::cerr << "Pipeline error in segment " << which 
                          << (output_or_input ? " (output)" : " (input)")
                          << " - Error: " << ec.message()
                          << " [code: " << ec.value() << "]" << std::endl;
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
        [](int which, bool output_or_input, std::error_code ec) {
            std::cerr << "Pipeline error in segment " << which 
                      << (output_or_input ? " (output)" : " (input)")
                      << " - Error: " << ec.message()
                      << " [code: " << ec.value() << "]" << std::endl;
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
        [](std::pair<io::buf, asio::ip::udp::endpoint>& recv_data) -> std::optional<std::pair<io::buf, asio::ip::udp::endpoint>> {
            const auto& [data, peer] = recv_data;
            
            std::string message(data.data(), data.size());
            std::cout << "Received message from " << peer << ": " << message << std::endl;
            
            return std::move(recv_data);
        } >> socket;
    
    // Start the pipeline and set up error handling callback
    auto started_pipeline = std::move(pipeline).start(
        [](int which, bool output_or_input, std::error_code ec) {
            std::cerr << "Pipeline error in segment " << which 
                      << (output_or_input ? " (output)" : " (input)")
                      << " - Error: " << ec.message()
                      << " [code: " << ec.value() << "]" << std::endl;
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
        void operator<<(std::string& input) {
            // Process input directly
            std::cout << "DirectInputProtocol received: " << input << std::endl;
        }
    };

    // 4. Future Input Protocol - accepts data and returns a future
    struct FutureInputProtocol {
        io::manager* mngr;
        FutureInputProtocol(io::manager* mngr) :mngr(mngr) {}
        
        io::future operator<<(int& input) {
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
        using prot_output_type = std::string;

        std::vector<std::string> data = { "Processed", "Data", "From", "Bidirectional" };
        size_t current_index = 0;

        void operator>>(std::string& output) {
            // Direct output
            output = data[current_index];
            current_index = (current_index + 1) % data.size();
            std::cout << "DirectBidirectionalProtocol produced: " << output << std::endl;
        }

        void operator<<(std::string& input) {
            // Direct input
            std::cout << "DirectBidirectionalProtocol received: " << input << std::endl;
        }
    };

    // 6. Bidirectional Protocol with Future I/O - implements both future input and output
    struct FutureBidirectionalProtocol {
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
        
        io::future operator<<(int& input) {
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
        
        io::future operator<<(int& input) {
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
        
        void operator<<(std::string& input) {
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
            >> [](int& value) -> std::optional<std::string> {
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
            [](std::string& value) -> std::optional<int> {
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
            [](std::string& value) -> std::optional<int> {
                return static_cast<int>(value.length());
            } >> middle_prot1 >> 
            [](std::string& value) -> std::optional<std::string> {
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

io::fsm_func<void> coro_tcp_throughput_test()
{
    // Test configuration
    constexpr size_t TEST_DURATION_SECONDS = 5;
    constexpr uint16_t SERVER_PORT = 12350;
    constexpr size_t PACKET_SIZE = 1024 * 64; // 64KB per packet
    
    io::fsm<void>& fsm = co_await io::get_fsm;
    std::cout << "TCP Throughput Test started..." << std::endl;
    
    // Start server
    io::fsm_handle<void> server_handle = fsm.spawn_now(
        [](uint16_t port, size_t packet_size) -> io::fsm_func<void> {
            io::fsm<void>& fsm = co_await io::get_fsm;
            std::cout << "TCP Throughput Server started on port " << port << std::endl;
            
            io::sock::tcp_accp acceptor(fsm);
            if (!acceptor.bind_and_listen(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))) {
                std::cerr << "Failed to bind to port " << port << std::endl;
                co_return;
            }
            
            // Accept client connection
            io::future_with<std::optional<io::sock::tcp>> accept_future;
            acceptor >> accept_future;
            co_await accept_future;
            
            if (!accept_future.data.has_value()) {
                std::cerr << "Failed to accept connection!" << std::endl;
                co_return;
            }
            
            io::sock::tcp socket = std::move(accept_future.data.value());
            std::cout << "Client connected from " << socket.remote_endpoint() << std::endl;
            
            // Prepare receive buffer
            io::buf recv_buffer(packet_size);
            size_t bytes_received = 0;
            io::timer::up timer;
            timer.start();
            
            // Process data in a loop
            while (1) {
                io::future_with<io::buf> recv_future;
                socket >> recv_future;
                co_await recv_future;
                
                if (recv_future.getErr()) {
                    std::cerr << "Error receiving data: " << recv_future.getErr().message() << std::endl;
                    break;
                }
                
                bytes_received += recv_future.data.size();
                
                // Send data back
                io::future send_future = socket << recv_future.data;
                co_await send_future;
                
                if (send_future.getErr()) {
                    std::cerr << "Error sending data: " << send_future.getErr().message() << std::endl;
                    break;
                }
            }
            
            std::cout << "Server completed." << std::endl;
            co_return;
        }(SERVER_PORT, PACKET_SIZE));
    
    // Wait a bit for server to start
    co_await fsm.setTimeout(std::chrono::milliseconds(500));
    
    // Start client
    io::fsm_handle<io::future> client_handle = fsm.spawn_now(
        [](uint16_t port, size_t packet_size, size_t duration_seconds) -> io::fsm_func<io::future> {
            auto& fsm = co_await io::get_fsm;
            std::cout << "TCP Throughput Client connecting to port " << port << std::endl;
            
            io::sock::tcp client(fsm);
            io::future connect_future = client.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), port));
            co_await connect_future;
            
            if (connect_future.getErr()) {
                std::cerr << "Failed to connect: " << connect_future.getErr().message() << std::endl;
                co_return;
            }
            
            std::cout << "Connected to server" << std::endl;
            
            // Prepare test data
            io::buf send_buffer(packet_size);
            send_buffer.size_increase(packet_size);
            
            size_t total_bytes = 0;
            size_t total_packets = 0;
            io::timer::up timer;
            timer.start();
            
            std::cout << "Starting throughput test for " << duration_seconds << " seconds..." << std::endl;
            
            // Test loop
            while (timer.elapsed() < std::chrono::seconds(duration_seconds)) {
                // Send data
                io::future send_future = client << send_buffer;
                co_await send_future;
                
                if (send_future.getErr()) {
                    std::cerr << "Error sending data: " << send_future.getErr().message() << std::endl;
                    break;
                }
                
                // Receive echo
                io::future_with<io::buf> recv_future;
                client >> recv_future;
                co_await recv_future;
                
                if (recv_future.getErr()) {
                    std::cerr << "Error receiving data: " << recv_future.getErr().message() << std::endl;
                    break;
                }
                
                total_bytes += recv_future.data.size();
                total_packets++;
            }
            
            auto duration = timer.elapsed();
            double seconds = std::chrono::duration<double>(duration).count();
            double mbps = (total_bytes * 8.0) / (1000000.0 * seconds);
            
            std::cout << "Throughput Test Results:" << std::endl;
            std::cout << "  Duration: " << seconds << " seconds" << std::endl;
            std::cout << "  Total data: " << (total_bytes / (1024.0 * 1024.0)) << " MB" << std::endl;
            std::cout << "  Packets: " << total_packets << std::endl;
            std::cout << "  Throughput: " << mbps << " Mbps" << std::endl;
            
            // Close connection
            client.close();
            co_return;
        }(SERVER_PORT, PACKET_SIZE, TEST_DURATION_SECONDS));
    
    // Wait for client to finish
    co_await *client_handle;
    
    // Start another test after a delay
    co_await fsm.setTimeout(std::chrono::seconds(5));
    fsm.getManager()->spawn_later(coro_tcp_throughput_test()).detach();
}

io::fsm_func<void> coro_tcp_concurrent_connections_test()
{
    // Test configuration
    constexpr uint16_t SERVER_PORT = 12351;
    constexpr size_t MAX_CONNECTIONS = 100000;
    constexpr size_t CONNECTION_BATCH = 100; // Connect this many at a time
    
    io::fsm<void>& fsm = co_await io::get_fsm;
    std::cout << "TCP Concurrent Connections Test started..." << std::endl;
    
    // Start server
    io::fsm_handle<void> server_handle = fsm.spawn_now(
        [](uint16_t port) -> io::fsm_func<void> {
            io::fsm<void>& fsm = co_await io::get_fsm;
            std::cout << "TCP Concurrent Connections Server started on port " << port << std::endl;
            
            // Counter for active connections
            std::atomic<size_t>* active_connections = new std::atomic<size_t>(0);
            std::atomic<size_t>* max_connections = new std::atomic<size_t>(0);
            
            io::sock::tcp_accp acceptor(fsm);
            if (!acceptor.bind_and_listen(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port), 2000)) { // Large backlog
                std::cerr << "Failed to bind to port " << port << std::endl;
                co_return;
            }
            
            // Accept connections in a loop
            while (1) {
                io::future_with<std::optional<io::sock::tcp>> accept_future;
                acceptor >> accept_future;
                co_await accept_future;
                
                if (!accept_future.data.has_value()) {
                    std::cerr << "Failed to accept connection!" << std::endl;
                    continue;
                }
                
                // Spawn a handler for each connection
                fsm.spawn_now(
                    [socket = std::move(accept_future.data.value()), active_connections, max_connections]() mutable -> io::fsm_func<void> {
                        io::fsm<void>& fsm = co_await io::get_fsm;
                        
                        // Increment connection counter
                        size_t current = ++(*active_connections);
                        size_t max = max_connections->load();
                        if (current > max) {
                            max_connections->store(current);
                            //std::cout << "New connection record: " << current << " concurrent connections" << std::endl;
                        }
                        
                        // Echo any data received
                        io::future_with<io::buf> recv_future;
                        socket >> recv_future;
                        co_await recv_future;
                        
                        if (!recv_future.getErr()) {
                            // Echo back
                            io::future send_future = socket << recv_future.data;
                            co_await send_future;
                        }
                        
                        // Keep connection open for the test
                        co_await fsm.setTimeout(std::chrono::seconds(30));
                        
                        // Decrement counter when connection closes
                        --(*active_connections);
                        socket.close();
                        co_return;
                    }())
                    .detach();
            }
            
            co_return;
        }(SERVER_PORT));
    
    // Wait a bit for server to start
    co_await fsm.setTimeout(std::chrono::milliseconds(500));
    
    // Client part: establish many connections
    std::cout << "Starting connection test - will establish up to " << MAX_CONNECTIONS << " concurrent connections" << std::endl;
    
    std::vector<io::sock::tcp> connections;
    connections.reserve(MAX_CONNECTIONS);
    
    size_t successful_connections = 0;
    io::timer::up timer;
    timer.start();
    
    // Connect in batches to avoid overloading
    for (size_t batch = 0; batch < MAX_CONNECTIONS / CONNECTION_BATCH; batch++) {
        std::vector<io::future> connect_futures;
        std::vector<io::sock::tcp> batch_connections;
        
        // Start a batch of connections
        for (size_t i = 0; i < CONNECTION_BATCH; i++) {
            io::sock::tcp client(fsm);
            io::future connect_future = client.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), SERVER_PORT));
            connect_futures.push_back(std::move(connect_future));
            batch_connections.push_back(std::move(client));
        }
        
        // Wait for all connections in this batch
        for (size_t i = 0; i < connect_futures.size(); i++) {
            co_await connect_futures[i];
            if (!connect_futures[i].getErr()) {
                // Send a small message on successful connections
                std::string message = "Hello";
                std::span<char> span(message.data(), message.size());
                io::future send_future = batch_connections[i] << span;
                co_await send_future;
                
                // Successful connection
                connections.push_back(std::move(batch_connections[i]));
                successful_connections++;
                
                if (successful_connections % 100 == 0) {
                    //std::cout << "Established " << successful_connections << " connections..." << std::endl;
                }
            }
        }
        
        // Small delay between batches
        co_await fsm.setTimeout(std::chrono::milliseconds(10));
    }
    
    auto duration = timer.elapsed();
    double seconds = std::chrono::duration<double>(duration).count();
    
    std::cout << "Connection Test Results:" << std::endl;
    std::cout << "  Successfully established: " << successful_connections << " connections" << std::endl;
    std::cout << "  Time taken: " << seconds << " seconds" << std::endl;
    std::cout << "  Connections per second: " << (successful_connections / seconds) << std::endl;
    
    // Keep connections open for a while
    std::cout << "Maintaining connections for 10 seconds..." << std::endl;
    co_await fsm.setTimeout(std::chrono::seconds(10));
    
    // Close all connections
    std::cout << "Closing all connections..." << std::endl;
    for (auto& conn : connections) {
        conn.close();
    }
    connections.clear();
    
    // Start another test after a delay
    co_await fsm.setTimeout(std::chrono::seconds(5));
    fsm.getManager()->spawn_later(coro_tcp_concurrent_connections_test()).detach();
}

io::fsm_func<void> coro_http_rpc_demo() {
    io::fsm<void>& fsm = co_await io::get_fsm;
    io::sock::tcp_accp acceptor(fsm);
    if (!acceptor.bind_and_listen(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 11111))) co_return;
    while (1)
    {
        io::future_with<std::optional<io::sock::tcp>> accept_future;
        acceptor >> accept_future;
        co_await accept_future;
        if (accept_future.data.has_value()) {
            fsm.spawn_now(
                [](io::sock::tcp socket) -> io::fsm_func<void>
                {
                    io::fsm<void>& fsm = co_await io::get_fsm;

                    io::rpc<std::string_view, io::prot::http::req_insitu&, io::prot::http::rsp> rpc(
                        std::pair{
                            "/test",[](io::prot::http::req_insitu& req)->io::prot::http::rsp {
                        io::prot::http::rsp rsp;
                        rsp.body = "Hello io::manager!";
                        return rsp;
                        }
                        },
                        io::rpc<>::def{ [](io::prot::http::req_insitu& req)->io::prot::http::rsp {
                        io::prot::http::rsp rsp;
                        rsp.body = "Unknown request.";
                        return rsp;
                        }
                        }
                    );
                    io::future end;

                    auto pipeline = io::pipeline<>() >> socket >> io::prot::http::req_parser(fsm) >> [&rpc](io::prot::http::req_insitu& req)->std::optional<io::prot::http::rsp> {
						//std::cout << "Received request: " << req.method_name() << " " << req.url << std::endl;
                        io::prot::http::rsp rsp = rpc(req.url, req);
						
                        rsp.status_code = 200;
                        rsp.status_message = "OK";
                        rsp.major_version = 1;
                        rsp.minor_version = 1;
                        
                        rsp.headers["Server"] = "ioManager/3.0";
                        rsp.headers["Content-Type"] = "text/html; charset=UTF-8";
                        rsp.headers["Connection"] = "keep-alive";
                        rsp.headers["X-Powered-By"] = "ioManager";
                        rsp.headers["Content-Length"] = std::to_string(rsp.body.size());
                        
						return rsp;
                        } >> io::prot::http::serializer(fsm) >> socket;

                    auto started_pipeline = std::move(pipeline).spawn(fsm,
                        [prom = fsm.make_future(end)](int which, bool output_or_input, std::error_code ec) mutable {
                            prom.resolve_later();
                            //std::cout << "eof" << std::endl;
                        }
                    );
                    co_await end;
                }(std::move(accept_future.data.value())))
                .detach();
        }
        else
            co_return;
    }
    co_return;
}

io::fsm_func<void> coro_thread_pool_test()
{
    io::fsm<void>& fsm = co_await io::get_fsm;
    std::cout << "\n=== Thread Pool Async Post Test ===\n" << std::endl;
    
    constexpr size_t NUM_THREADS = 4;
    io::pool thread_pool(NUM_THREADS);
    
    std::cout << "Created thread pool with " << NUM_THREADS << " threads" << std::endl;
    
    std::cout << "\n--- Test 1: Basic post and wait ---\n" << std::endl;
    {
        io::timer::up timer;
        timer.start();
        
        io::async_future future = thread_pool.post(fsm.getManager(), 
            []() {
                std::cout << "Task executing in thread " << std::this_thread::get_id() << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                std::cout << "Task completed" << std::endl;
            });
        
        std::cout << "Waiting for task to complete..." << std::endl;
        co_await future;
        
        auto duration = timer.lap();
        std::cout << "Task completed in " 
                  << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() 
                  << " ms" << std::endl;
    }
    
    std::cout << "\n--- Test 2: Multiple concurrent tasks ---\n" << std::endl;
    {
        constexpr size_t NUM_TASKS = 10;
        std::vector<io::async_future> futures;
        
        io::timer::up timer;
        timer.start();
        
        std::cout << "Posting " << NUM_TASKS << " tasks..." << std::endl;
        
        for (size_t i = 0; i < NUM_TASKS; i++) {
            futures.push_back(thread_pool.post(fsm.getManager(), 
                [i]() {
                    std::cout << "Task " << i << " executing in thread " 
                              << std::this_thread::get_id() << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    std::cout << "Task " << i << " completed" << std::endl;
                }));
        }
        
        for (auto& future : futures) {
            co_await future;
        }
        
        auto duration = timer.lap();
        std::cout << "All tasks completed in " 
                  << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() 
                  << " ms" << std::endl;
                  
        std::cout << "Tasks ran concurrently (total time < sum of individual task times)" << std::endl;

        fsm.getManager()->spawn_later(coro_thread_pool_test()).detach();
        co_return;
    }
}

void io_testmain_v3()
{
    io::manager mngr;

    // test of basic coroutine method
    if (false)
        mngr.async_spawn(coro_demo(100));

    // limit test
    if (false)
        mngr.async_spawn(coro_limit_test(10000000));

    // test of chan
    if (false)
    {
        if (true)
            mngr.async_spawn(coro_chan_benchmark());
        else
            mngr.async_spawn(coro_chan(nullptr, nullptr));
    }

    // test of async_chan
    if (false)
    {
        if (true)
            mngr.async_spawn(coro_async_chan_benchmark());
        else
            mngr.async_spawn(coro_async_chan(nullptr, nullptr, 4));
    }

    // construction correctness test for chan
    if (false)
        mngr.async_spawn(coro_chan_construct_correct_test());
    
    // peak shaving demo with chan
    if (false)
        mngr.async_spawn(coro_chan_peak_shaving());
        
    // coroutine benchmark
    if (true)
        mngr.async_spawn(coro_benchmark());

    // compensated timer test
    if (false)
        mngr.async_spawn(coro_down_timer());

    // tcp echo server
    if (false)
        mngr.async_spawn(coro_tcp_echo_server());

    // tcp echo client
    if (false)
        mngr.async_spawn(coro_tcp_echo_client());

    // tcp throughput test
    if (false)
        mngr.async_spawn(coro_tcp_throughput_test());

    // tcp concurrent connections test
    if (false)
        mngr.async_spawn(coro_tcp_concurrent_connections_test());

    // udp echo
    if (false)
        mngr.async_spawn(coro_udp_echo());

    // udp echo with adapter
    if (false)
        mngr.async_spawn(coro_udp_echo_with_adapter());

    // pipeline
    if (false)
        mngr.async_spawn(coro_pipeline_test());

    // http rpc demo
    if (false)
        mngr.async_spawn(coro_http_rpc_demo());

    // thread pool test
    if (false)
        mngr.async_spawn(coro_thread_pool_test());

    while (1)
    {
        mngr.drive();
    }
}