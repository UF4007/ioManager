#include <cstdio>
#include <iostream>
#include <ioManager/ioManager.h>
#include <ioManager/socket/asio/udp.h>

io::fsm_func<void> udp_echo()
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

int main()
{
    io::manager mngr;
    mngr.async_spawn(udp_echo());

    while (1)
    {
        mngr.drive();
    }

    return 0;
} 