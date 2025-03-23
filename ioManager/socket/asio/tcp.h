#pragma once
namespace io
{
    inline namespace IO_LIB_VERSION___ {
        namespace sock {
            struct tcp {
                __IO_INTERNAL_HEADER_PERMISSION;
                friend class std::optional<tcp>;
                using prot_output_type = std::span<char>;
                using prot_input_type = std::span<char>;
                static constexpr size_t default_buffer_size = 1024 * 16;
                template <typename T_FSM>
                inline tcp(fsm<T_FSM>& state_machine) : manager(state_machine.getManager()), asio_sock(manager->io_ctx) {
                    buffer.resize(default_buffer_size);
                }

                inline void setBufSize(size_t size) {
                    buffer.resize(size);
                }

                //receive function
                inline void operator >>(future_with<std::span<char>>& fut) {
                    promise<std::span<char>> prom = manager->make_future(fut, &fut.data);
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

                            std::error_code read_ec;
                            size_t bytes_read = asio_sock.read_some(asio::buffer(buffer), read_ec);
                            if (read_ec) {
                                prom.reject_later(read_ec);
                                return;
                            }

                            auto ptr = prom.resolve_later();
                            if (ptr)
                            {
                                *ptr = std::span<char>(buffer.data(), bytes_read);
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
                    async_send_remaining(std::move(remaining_data), std::move(prom));

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
                inline tcp(asio::ip::tcp::socket&& sock, size_t size, io::manager* mngr) :manager(mngr), asio_sock(std::move(sock)) {
                    buffer.resize(size);
                }
            private:
                // Helper function to recursively send remaining data
                void async_send_remaining(std::unique_ptr<std::string> data, promise<> prom) {
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
                                async_send_remaining(std::move(data), std::move(prom));
                            }
                            else {
                                // All data sent successfully
                                prom.resolve_later();
                            }
                        });
                }
                io::manager* manager = nullptr;
                std::string buffer;
                asio::ip::tcp::socket asio_sock;
            };
        };
    };
};