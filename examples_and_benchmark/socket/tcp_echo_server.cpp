#include <ioManager/ioManager.h>
#include <ioManager/socket/asio/tcp.h>
#include <ioManager/socket/asio/tcp_accp.h>
#include <ioManager/pipeline.h>

io::fsm_func<void> tcp_echo_server()
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

int main()
{
    io::manager mngr;
    mngr.async_spawn(tcp_echo_server());

    while (1)
    {
        mngr.drive();
    }

    return 0;
} 