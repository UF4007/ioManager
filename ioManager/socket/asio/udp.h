#pragma once
namespace io
{
    inline namespace IO_LIB_VERSION___ {
        namespace sock {
            struct udp {
                __IO_INTERNAL_HEADER_PERMISSION;
                using prot_output_type = std::pair<std::span<char>, asio::ip::udp::endpoint>;
                static constexpr size_t default_buffer_size = 1024 * 16;
                template <typename T_FSM>
                inline udp(fsm<T_FSM>& state_machine) : manager(state_machine.getManager()), asio_sock(manager->io_ctx) {
                    buffer.resize(default_buffer_size);
                }

                inline void setBufSize(size_t size) {
                    buffer.resize(size);
                }

                //receive function
                inline void operator >>(future_with<std::pair<std::span<char>, asio::ip::udp::endpoint>>& fut) {
                    auto prom = manager->make_future(fut, &fut.data);
                    if (!asio_sock.is_open()) {
                        prom.reject_later(std::make_error_code(std::errc::not_connected));
                        return;
                    }

                    asio_sock.async_receive_from(asio::buffer(buffer), remote_endpoint,
                        [this, prom = std::move(prom)](const std::error_code& ec, size_t bytes_read) mutable {
                            if (ec) {
                                prom.reject_later(ec);
                                return;
                            }

                            auto ptr = prom.resolve_later();
                            if (ptr) {
                                *ptr = std::make_pair(std::span<char>(buffer.data(), bytes_read), remote_endpoint);
                            }
                        });
                }

                //send function
                inline future operator <<(const std::pair<std::span<char>, asio::ip::udp::endpoint>& span) {
                    // Set non-blocking for newly created sockets
                    std::error_code ec;
                    asio_sock.non_blocking(true, ec);

                    future fut;
                    promise<> prom = manager->make_future(fut);
                    if (!asio_sock.is_open()) {
                        prom.reject_later(std::make_error_code(std::errc::not_connected));
                        return fut;
                    }

                    // Try immediate non-blocking send first
                    std::error_code write_ec;
                    size_t bytes_written = asio_sock.send_to(
                        asio::buffer(span.first.data(), span.first.size()),
                        span.second,
                        0, // flags
                        write_ec
                    );

                    // For UDP, if we get a would_block, we should use async send
                    // If we get any other error, fail immediately
                    if (!write_ec && bytes_written == span.first.size()) {
                        prom.resolve_later();
                        return fut;
                    }

                    if (write_ec && write_ec != asio::error::would_block) {
                        prom.reject_later(write_ec);
                        return fut;
                    }

                    // Create a copy of data using unique_ptr
                    auto data_copy = std::make_unique<std::string>(
                        span.first.data(), span.first.size()
                    );
                    auto endpoint_copy = span.second;

                    asio_sock.async_wait(asio::ip::udp::socket::wait_write,
                        [this, data = std::move(data_copy), endpoint = endpoint_copy, prom = std::move(prom)]
                        (const std::error_code& ec) mutable {
                            if (ec) {
                                prom.reject_later(ec);
                                return;
                            }

                            std::error_code write_ec;
                            asio_sock.send_to(
                                asio::buffer(data->data(), data->size()),
                                endpoint,
                                0, // flags
                                write_ec
                            );

                            if (write_ec) {
                                prom.reject_later(write_ec);
                            }
                            else {
                                prom.resolve_later();
                            }
                        });

                    return fut;
                }

                inline bool bind(const asio::ip::udp::endpoint& endpoint) {
                    std::error_code ec;
                    asio_sock.open(endpoint.protocol(), ec);
                    if (ec) {
                        return false;
                    }

                    asio_sock.bind(endpoint, ec);
                    if (ec) {
                        return false;
                    }
                    return !ec;
                }

                inline void close() {
                    if (asio_sock.is_open()) {
                        std::error_code ec;
                        asio_sock.close(ec);
                    }
                }

                IO_MANAGER_FORWARD_FUNC(asio_sock, native_handle);
                IO_MANAGER_FORWARD_FUNC(asio_sock, is_open);
                IO_MANAGER_FORWARD_FUNC(asio_sock, local_endpoint);

            private:
                io::manager* manager = nullptr;
                std::string buffer;
                asio::ip::udp::socket asio_sock;
                asio::ip::udp::endpoint remote_endpoint;
            };
        };
    };
};