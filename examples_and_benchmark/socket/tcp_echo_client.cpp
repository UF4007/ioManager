#include <cstdio>
#include <iostream>
#include <string>
#include <ctime>
#include <ioManager/ioManager.h>
#include <ioManager/socket/asio/tcp.h>
#include <ioManager/pipeline.h>
#include <ioManager/timer.h>

io::fsm_func<void> tcp_echo_client()
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

int main()
{
    io::manager mngr;
    mngr.async_spawn(tcp_echo_client());

    while (1)
    {
        mngr.drive();
    }

    return 0;
} 