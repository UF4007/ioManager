#pragma once
namespace io
{
    inline namespace IO_LIB_VERSION___ {
        namespace sock {
            struct udp {
                __IO_INTERNAL_HEADER_PERMISSION;
                friend class std::optional<udp>;
                using prot_output_type = std::pair<io::buf, asio::ip::udp::endpoint>;
                
                template <typename T_FSM>
                inline udp(fsm<T_FSM>& state_machine) : manager(state_machine.getManager()), asio_sock(manager->io_ctx) {}
                
                inline udp(io::manager* _manager) : manager(_manager), asio_sock(manager->io_ctx) {}

                inline void setNextBuf(io::buf &&buffer_next) {
                    buffer = std::move(buffer_next);
                }

                //receive function
                inline void operator >>(future_with<std::pair<io::buf, asio::ip::udp::endpoint>>& fut) {
                    auto prom = manager->make_future(fut, &fut.data);
                    if (!asio_sock.is_open()) {
                        prom.reject_later(std::make_error_code(std::errc::not_connected));
                        return;
                    }

                    asio_sock.async_wait(asio::ip::tcp::socket::wait_read,
                        [this, prom = std::move(prom)](const std::error_code& ec) mutable {
                            if (ec) {
                                prom.reject_later(ec);
                                return;
                            }

                            auto ptr = prom.data();
                            if (ptr)
                            {
                                std::error_code read_ec;
                                if (asio_sock.available(read_ec) == 0) {
                                    prom.reject_later(std::make_error_code(std::errc::connection_aborted));
                                    return;
                                }

                                asio::ip::udp::endpoint end;
                                io::buf read_buf;
                                if (this->buffer)
                                {
                                    read_buf = std::move(this->buffer);
                                }
                                else
                                {
                                    read_buf = io::buf(asio_sock.available());
                                }
                                size_t bytes_read = asio_sock.receive_from(asio::buffer(read_buf.unused_span().data(), read_buf.unused_span().size()), end, 0, read_ec);
                                read_buf.size_increase(bytes_read);
                                if (read_ec) {
                                    prom.reject_later(read_ec);
                                    return;
                                }

                                prom.resolve_later();
                                *ptr = std::make_pair(std::move(read_buf), end);
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

                // Add support for io::buf send
                inline future operator <<(std::pair<io::buf, asio::ip::udp::endpoint>& data) {
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
                        asio::buffer(data.first.data(), data.first.size()),
                        data.second,
                        0, // flags
                        write_ec
                    );

                    // For UDP, if we get a would_block, we should use async send
                    // If we get any other error, fail immediately
                    if (!write_ec && bytes_written == data.first.size()) {
                        prom.resolve_later();
                        return fut;
                    }

                    if (write_ec && write_ec != asio::error::would_block) {
                        prom.reject_later(write_ec);
                        return fut;
                    }

                    asio_sock.async_wait(asio::ip::udp::socket::wait_write,
                        [this, buf = std::move(data.first), endpoint = data.second, prom = std::move(prom)]
                        (const std::error_code& ec) mutable {
                            if (ec) {
                                prom.reject_later(ec);
                                return;
                            }

                            std::error_code write_ec;
                            asio_sock.send_to(
                                asio::buffer(buf.data(), buf.size()),
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
                io::buf buffer;
                asio::ip::udp::socket asio_sock;
                asio::ip::udp::endpoint remote_endpoint;
            };
        };
    };
};