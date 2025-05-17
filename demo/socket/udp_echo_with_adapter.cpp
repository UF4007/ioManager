#include <cstdio>
#include <iostream>
#include <string>
#include <utility>
#include "../../ioManager/all.h"

io::fsm_func<void> udp_echo_with_adapter()
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

int main()
{
    io::manager mngr;
    mngr.async_spawn(udp_echo_with_adapter());

    while (1)
    {
        mngr.drive();
    }

    return 0;
} 