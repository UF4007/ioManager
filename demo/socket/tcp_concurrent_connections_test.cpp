#include <cstdio>
#include <iostream>
#include <vector>
#include <atomic>
#include <chrono>
#include "../../ioManager/all.h"

io::fsm_func<void> tcp_concurrent_connections_test()
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
    fsm.getManager()->spawn_later(tcp_concurrent_connections_test()).detach();
}

int main()
{
    io::manager mngr;
    mngr.async_spawn(tcp_concurrent_connections_test());

    while (1)
    {
        mngr.drive();
    }

    return 0;
} 