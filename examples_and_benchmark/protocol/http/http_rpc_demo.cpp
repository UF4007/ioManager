#include <cstdio>
#include <iostream>
#include <string>
#include <ioManager/ioManager.h>
#include <ioManager/protocol/http/http.h>

io::fsm_func<void> http_rpc_demo() {
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

int main()
{
    io::manager mngr;
    mngr.async_spawn(http_rpc_demo());

    std::cout << "HTTP RPC server started on port 11111" << std::endl;
    std::cout << "Try connecting to http://localhost:11111/test with your browser" << std::endl;
    
    while (1)
    {
        mngr.drive();
    }

    return 0;
} 