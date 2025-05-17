#include <iostream>
#include <chrono>
#include <thread>
#include <random>
#include <ioManager/ioManager.h>
#include <ioManager/protocol/kcp/kcp.h>

constexpr bool debuging = false;

// Coroutine A: Simple KCP Echo Server
io::fsm_func<void> kcp_echo_server() {
    io::fsm<void>& fsm = co_await io::get_fsm;
    
    std::cout << "Starting KCP Echo Server on port 12345..." << std::endl;

    // Create KCP control block with session ID 0x11223344
    io::prot::kcp_t kcp(fsm, 0x11223344);
    
    // Configure KCP for low latency settings
    kcp.nodelay(1, 10, 2, 1);  // Fast mode
    kcp.wndsize(16, 16);     // Larger window size
    kcp.setmtu(1400);          // Standard MTU size
    
    // Create KCP protocol objects (receive and send)
    io::prot::kcp_recv_t kcp_recv = kcp.getRecv();
    io::prot::kcp_send_t kcp_send = kcp.getSend(true);
    
    // Create UDP socket for communication
    io::sock::udp socket(fsm);
    
    if (!socket.bind(asio::ip::udp::endpoint(asio::ip::address_v4::any(), 12345))) {
        std::cerr << "Failed to bind UDP socket to port 12345" << std::endl;
        co_return;
    }
    
    std::cout << "KCP Echo Server is running. Waiting for data..." << std::endl;
    
    // We need to store the client endpoint for replies
    asio::ip::udp::endpoint client_endpoint;
    
    // Create pipeline for echo: UDP >> kcp_recv >> kcp_send >> UDP
    auto pipeline = io::pipeline<>() >> socket >> 
        // Adapter to convert UDP data to KCP input format
        [&client_endpoint](std::pair<io::buf, asio::ip::udp::endpoint>& data) -> std::optional<std::span<const char>> {
            // Store the client endpoint for later use
            client_endpoint = data.second;
            
            if constexpr (debuging)
            std::cout << "    Server received " << data.first.size() << " bytes from " 
                    << data.second.address().to_string() << ":" << data.second.port() << std::endl;
            return std::span<const char>(data.first.data(), data.first.size());
        } >> kcp_recv >> kcp_send >>
        // Adapter to convert KCP output to UDP format
        [&client_endpoint](std::string& kcp_data) -> std::optional<std::pair<std::span<char>, asio::ip::udp::endpoint>> {
            try {
                if constexpr (debuging)
                std::cout << "    Server sending " << kcp_data.size() << " bytes back to client at " 
                         << client_endpoint.address().to_string() << ":" << client_endpoint.port() << std::endl;
                
                // Return data with the client endpoint
                return std::make_pair(std::span<char>(kcp_data.data(), kcp_data.size()), client_endpoint);
            } catch (const std::exception& e) {
                std::cerr << "    Error in server adapter: " << e.what() << std::endl;
                return std::nullopt;
            }
        } >> socket;
    
    // Start the pipeline with error handler
    auto started_pipeline = std::move(pipeline).start(
        [](int which, bool output_or_input, std::error_code ec) {
            std::cerr << "    KCP Echo Server pipeline error in segment " << which 
                      << (output_or_input ? " (output)" : " (input)")
                      << " - Error: " << ec.message()
                      << " [code: " << ec.value() << "]" << std::endl;
        }
    );
    
    // Keep driving the pipeline
    while (true) {
        auto i = co_await +started_pipeline;
        started_pipeline <= i;
    }
    
    co_return;
}

// Coroutine B: KCP Client with Sentinel
io::fsm_func<void> kcp_sentinel_client() {
    io::fsm<void>& fsm = co_await io::get_fsm;
    
    std::cout << "Starting KCP Client with Sentinel..." << std::endl;
    
    // Wait a moment for the server to start
    co_await fsm.setTimeout(std::chrono::seconds(1));
    
    // Create KCP control block with session ID 0x11223344 (same as server)
    io::prot::kcp_t kcp(fsm, 0x11223344);
    
    // Configure KCP for low latency settings
    kcp.nodelay(1, 10, 2, 1);  // Fast mode
    kcp.wndsize(16, 16);     // Larger window size
    kcp.setmtu(1400);          // Standard MTU size
    
    // Create KCP protocol objects (receive and send)
    io::prot::kcp_recv_t kcp_recv = kcp.getRecv();
    io::prot::kcp_send_t kcp_send = kcp.getSend(true);
    
    // Create UDP socket for communication
    io::sock::udp socket(fsm);
    
    // No need to bind to a specific port for client
    if (!socket.bind(asio::ip::udp::endpoint(asio::ip::address_v4::any(), 0))) {
        std::cerr << "Failed to bind UDP socket" << std::endl;
        co_return;
    }
    
    // Create a server endpoint
    asio::ip::udp::endpoint server_endpoint(
        asio::ip::address::from_string("127.0.0.1"), 
        12345
    );
    
    // Create packet sentinel to generate random data and validate responses
    io::prot::packet_sentinel sentinel(
        fsm.getManager(),
        64,     // Min packet size
        4096,   // Max packet size
        std::chrono::milliseconds(50),   // Generate a packet every second
        // Good packet callback
        [](std::span<char> data, std::chrono::steady_clock::duration delay) {
            static size_t total_bytes = 0;
            static size_t packet_count = 0;
            static auto last_report_time = std::chrono::steady_clock::now();

            total_bytes += data.size();
            packet_count++;

            std::cout << "--------Client received valid response packet of size " << data.size() << ",  delay " << std::chrono::duration_cast<std::chrono::milliseconds>(delay) << std::endl;

            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - last_report_time);

            if (elapsed.count() >= 5) {
                double throughput_bytes = static_cast<double>(total_bytes) / elapsed.count();
                double throughput_mbps = (throughput_bytes * 8) / (1024 * 1024);

                std::cout << "===== THROUGHPUT REPORT =====" << std::endl;
                std::cout << "Period: " << elapsed.count() << " seconds" << std::endl;
                std::cout << "Packets: " << packet_count << std::endl;
                std::cout << "Total data: " << total_bytes << " bytes" << std::endl;
                std::cout << "Average throughput: " << throughput_mbps << " Mbps" << std::endl;
                std::cout << "===========================" << std::endl;

                total_bytes = 0;
                packet_count = 0;
                last_report_time = current_time;
            }
        },
        // Bad packet callback
        [](std::span<char> received, std::span<char> expected, std::chrono::steady_clock::duration delay) {
            std::cerr << "---!!!---Client received invalid packet! Received size: " << received.size() 
                      << ", Expected size: " << expected.size() << std::endl;
        },
        10000
    );
    
    // Create pipeline for client: UDP >> kcp_recv >> sentinel >> kcp_send >> UDP
    auto pipeline = io::pipeline<>() >> socket >> 
        // Adapter to convert UDP data to KCP input format
        [](std::pair<io::buf, asio::ip::udp::endpoint>& data) -> std::optional<std::span<const char>> {
        if constexpr (debuging)
            std::cout << "Client received " << data.first.size() << " bytes from " 
                     << data.second.address().to_string() << ":" << data.second.port() << std::endl;
        return std::span<const char>(data.first.data(), data.first.size());
        } >> kcp_recv >> 
        // Adapter to convert KCP string output to char span for sentinel
        [](std::string& kcp_data) -> std::optional<std::span<char>> {
            return std::span<char>(kcp_data.data(), kcp_data.size());
        } >> sentinel >> 
        // Pass sentinel output to kcp_send
        [](std::span<char>& data) -> std::optional<std::span<const char>> {
            if constexpr (debuging)
            std::cout << "Client generate packet of size " << data.size() << std::endl;
            return std::span<const char>(data.data(), data.size());
        } >> kcp_send >>
        // Adapter to convert KCP output to UDP format with server endpoint
        [server_endpoint](std::string& kcp_data) -> std::optional<std::pair<std::span<char>, asio::ip::udp::endpoint>> {
            if constexpr (debuging)
            std::cout << "Client sending " << kcp_data.size() << " bytes to server at " 
                     << server_endpoint.address().to_string() << ":" << server_endpoint.port() << std::endl;
            // Always send to the server endpoint
            return std::make_pair(std::span<char>(kcp_data.data(), kcp_data.size()), server_endpoint);
        } >> socket;
    
    // Start the pipeline with error handler
    auto started_pipeline = std::move(pipeline).start(
        [](int which, bool output_or_input, std::error_code ec) {
            std::cerr << "KCP Client pipeline error in segment " << which 
                      << (output_or_input ? " (output)" : " (input)")
                      << " - Error: " << ec.message()
                      << " [code: " << ec.value() << "]" << std::endl;
        }
    );
    
    // Keep driving the pipeline
    while (true) {
        auto i = co_await +started_pipeline;
        started_pipeline <= i;
    }
    
    co_return;
}

int main() {
    // clumsy setting: (udp.DstPort == 12345  or udp.DstPort == 12345 )and loopback

    //std::this_thread::sleep_for(std::chrono::seconds(10));    //attach debuger

    io::manager mgr;
    
    mgr.async_spawn(kcp_echo_server());
    mgr.async_spawn(kcp_sentinel_client());
    
    std::cout << "Main thread: running io::manager loop..." << std::endl;
    while (true) {
        mgr.drive();
    }
    
    return 0;
} 