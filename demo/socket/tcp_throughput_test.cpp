#include <ioManager/ioManager.h>
#include <ioManager/socket/asio/tcp.h>
#include <ioManager/socket/asio/tcp_accp.h>
#include <ioManager/timer.h>

io::fsm_func<void> tcp_throughput_test()
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
    fsm.getManager()->spawn_later(tcp_throughput_test()).detach();
}

int main()
{
    io::manager mngr;
    mngr.async_spawn(tcp_throughput_test());

    while (1)
    {
        mngr.drive();
    }

    return 0;
} 