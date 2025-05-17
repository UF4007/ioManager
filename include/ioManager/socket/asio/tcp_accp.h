#pragma once
#include "../../ioManager.h"
#include "tcp.h"

namespace io
{
    inline namespace IO_LIB_VERSION___ {
        namespace sock {
            struct tcp_accp {
                __IO_INTERNAL_HEADER_PERMISSION;
                using prot_output_type = std::optional<tcp>;
                template <typename T_FSM>
                inline tcp_accp(fsm<T_FSM>& state_machine) : manager(state_machine.getManager()), sock(manager->io_ctx), acceptor(manager->io_ctx) {}

                inline tcp_accp(io::manager* _manager) : manager(_manager), sock(manager->io_ctx), acceptor(manager->io_ctx) {}

                inline bool bind_and_listen(const asio::ip::tcp::endpoint& endpoint, int backlog = 10) {
                    std::error_code ec;
                    acceptor.open(endpoint.protocol(), ec);
                    if (ec) {
                        return false;
                    }

                    acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true), ec);
                    if (ec) {
                        return false;
                    }

                    acceptor.bind(endpoint, ec);
                    if (ec) {
                        return false;
                    }

                    acceptor.listen(backlog, ec);
                    if (ec) {
                        return false;
                    }

                    return !ec;
                }

                inline void operator >>(future_with<std::optional<tcp>>& fut) {
                    promise<std::optional<tcp>> prom = manager->make_future(fut, &fut.data);
                    if (!acceptor.is_open()) {
                        fut.getPromise().reject_later(std::make_error_code(std::errc::not_connected));
                        return;
                    }

                    acceptor.async_accept(sock,
                        [this, prom = std::move(prom)](const std::error_code& ec) mutable {
                            if (ec) {
                                prom.reject_later(ec);
                                return;
                            }

                            auto ptr = prom.data();
                            if (ptr)
                            {
                                std::error_code nb_ec;
                                sock.non_blocking(true, nb_ec);
                                if (nb_ec) {
                                    prom.reject_later(nb_ec);
                                    return;
                                }
                                prom.resolve_later();
                                ptr->emplace(std::move(sock), manager);
                            }
                            else
                            {
                                std::error_code ec;
                                sock.close(ec);
                            }
                        });
                }

                inline void close() {
                    if (acceptor.is_open()) {
                        std::error_code ec;
                        acceptor.close(ec);
                    }
                }

                IO_MANAGER_FORWARD_FUNC(acceptor, is_open);
                IO_MANAGER_FORWARD_FUNC(acceptor, local_endpoint);

            private:
                io::manager* manager = nullptr;
                asio::ip::tcp::socket sock;
                asio::ip::tcp::acceptor acceptor;
            };
        };
    };
};