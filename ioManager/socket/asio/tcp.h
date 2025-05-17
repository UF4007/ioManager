#pragma once
namespace io
{
    inline namespace IO_LIB_VERSION___ {
        namespace sock {
            struct tcp {
                __IO_INTERNAL_HEADER_PERMISSION;
                friend class std::optional<tcp>;
                using prot_output_type = io::buf;

                template <typename T_FSM>
                inline tcp(fsm<T_FSM>& state_machine) : manager(state_machine.getManager()), asio_sock(manager->io_ctx) {}

                inline tcp(io::manager* _manager) : manager(_manager), asio_sock(manager->io_ctx) {}

                inline void setNextBuf(io::buf &&buffer_next) {
                    buffer = std::move(buffer_next);
                }

                //receive function
                inline void operator >>(future_with<io::buf>& fut) {
                    promise<io::buf> prom = manager->make_future(fut, &fut.data);
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

                                io::buf read_buf;
                                if (this->buffer)
                                {
                                    read_buf = std::move(this->buffer);
                                }
                                else
                                {
                                    read_buf = io::buf(asio_sock.available());
                                }
                                size_t bytes_read = asio_sock.read_some(asio::buffer(read_buf.unused_span().data(), read_buf.unused_span().size()), read_ec);
                                read_buf.size_increase(bytes_read);
                                if (read_ec) {
                                    prom.reject_later(read_ec);
                                    return;
                                }

                                prom.resolve_later();
                                *ptr = std::move(read_buf);
                            }
                        });
                }

                //send function
                inline future operator <<(const std::span<char>& span) {
                    future fut;
                    promise<> prom = manager->make_future(fut);
                    if (!asio_sock.is_open()) {
                        prom.reject_later(std::make_error_code(std::errc::not_connected));
                        return fut;
                    }

                    // Try immediate non-blocking send first
                    std::error_code write_ec;
                    size_t bytes_written = asio_sock.write_some(asio::buffer(span.data(), span.size()), write_ec);

                    // Check if we wrote everything successfully
                    if (!write_ec && bytes_written == span.size()) {
                        prom.resolve_later();
                        return fut;
                    }

                    // If we got an error other than would_block, reject
                    if (write_ec && write_ec != asio::error::would_block) {
                        prom.reject_later(write_ec);
                        return fut;
                    }

                    // Create a copy of remaining data using unique_ptr
                    auto remaining_data = std::make_unique<std::string>(
                        span.data() + bytes_written,
                        span.size() - bytes_written
                    );

                    // Use helper function to handle recursive sending
                    async_send_remaining_str(std::move(remaining_data), std::move(prom));

                    return fut;
                }

                //send function
                inline future operator <<(io::buf& send_buf) {
                    future fut;
                    promise<> prom = manager->make_future(fut);
                    if (!asio_sock.is_open()) {
                        prom.reject_later(std::make_error_code(std::errc::not_connected));
                        return fut;
                    }

                    // Try immediate non-blocking send first
                    std::error_code write_ec;
                    size_t bytes_written = asio_sock.write_some(asio::buffer(send_buf.data(), send_buf.size()), write_ec);

                    // Check if we wrote everything successfully
                    if (!write_ec && bytes_written == send_buf.size()) {
                        prom.resolve_later();
                        return fut;
                    }

                    // If we got an error other than would_block, reject
                    if (write_ec && write_ec != asio::error::would_block) {
                        prom.reject_later(write_ec);
                        return fut;
                    }

                    send_buf.size_decrease(bytes_written);

                    // Use helper function to handle recursive sending
                    async_send_remaining_buf(std::move(send_buf), std::move(prom));

                    return fut;
                }

                inline future connect(const asio::ip::tcp::endpoint& endpoint) {
                    future fut;
                    promise<> prom = manager->make_future(fut);

                    asio_sock.async_connect(endpoint,
                        [this, prom = std::move(prom)](const std::error_code& ec) mutable {
                            if (ec) {
                                prom.reject_later(ec);
                            }
                            else {
                                // Set non-blocking mode after successful connection
                                std::error_code nb_ec;
                                asio_sock.non_blocking(true, nb_ec);
                                prom.resolve_later();
                            }
                        });

                    return fut;
                }

                inline void close() {
                    if (asio_sock.is_open()) {
                        std::error_code ec;
                        asio_sock.close(ec);
                    }
                }

                IO_MANAGER_FORWARD_FUNC(asio_sock, native_handle);
                IO_MANAGER_FORWARD_FUNC(asio_sock, is_open);
                IO_MANAGER_FORWARD_FUNC(asio_sock, remote_endpoint);
                IO_MANAGER_FORWARD_FUNC(asio_sock, local_endpoint);
                IO_MANAGER_FORWARD_FUNC(asio_sock, available);
                inline tcp(asio::ip::tcp::socket&& sock, io::manager* mngr) :manager(mngr), asio_sock(std::move(sock)) {}
            private:
                // Helper function to recursively send remaining data
                void async_send_remaining_str(std::unique_ptr<std::string> data, promise<> prom) {
                    asio_sock.async_write_some(
                        asio::buffer(data->data(), data->size()),
                        [this, data = std::move(data), prom = std::move(prom)]
                        (const std::error_code& ec, size_t bytes_sent) mutable {
                            if (ec) {
                                prom.reject_later(ec);
                                return;
                            }

                            if (bytes_sent < data->size()) {
                                // Still data left to send - update the buffer and recurse
                                data->erase(0, bytes_sent);
                                async_send_remaining_str(std::move(data), std::move(prom));
                            }
                            else {
                                // All data sent successfully
                                prom.resolve_later();
                            }
                        });
                }
                void async_send_remaining_buf(io::buf send_buf, promise<> prom) {
                    asio_sock.async_write_some(
                        asio::buffer(send_buf.data(), send_buf.size()),
                        [this, send_buf = std::move(send_buf), prom = std::move(prom)]
                        (const std::error_code& ec, size_t bytes_sent) mutable {
                            if (ec) {
                                prom.reject_later(ec);
                                return;
                            }

                            if (bytes_sent < send_buf.size()) {
								send_buf.size_decrease(bytes_sent);
                                async_send_remaining_buf(std::move(send_buf), std::move(prom));
                            }
                            else {
                                // All data sent successfully
                                prom.resolve_later();
                            }
                        });
                }
                io::manager* manager = nullptr;
                io::buf buffer;
                asio::ip::tcp::socket asio_sock;
            };
        };
    };
};