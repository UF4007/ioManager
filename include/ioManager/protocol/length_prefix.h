#pragma once
#include <limits>
#include "../ioManager.h"
#include "../pipeline.h"

namespace io {
    inline namespace IO_LIB_VERSION___ {
        namespace length_prefix {

            struct package {
                void clean() {
                    received_size = 0;
                    if (total_size <= 24 || total_size == std::numeric_limits<size_t>::max()) {
                        //use SSO
                        received_size = 0;
                    }
                    else {
                        //use heap
                        sso.large_bufs.clear();
                        received_size = 0;
                    }
                    total_size = std::numeric_limits<size_t>::max();
                }

                // clean first, then set total size.
                void set_total_size(size_t size) {
                    clean();
                    total_size = size;
                }

                // data_in must be owned memory.
                // if data_in can totally push into the package, data_in will be transferred.
                // if not, data pointer in data_in will be increased, size decreased.
                // returns the size actually pushed.
                size_t push(io::buf& data_in) {
                    size_t to_push = std::min(data_in.size(), total_size - received_size);
                    if (to_push == 0) {
                        return 0;
                    }

                    if (total_size <= 24) {
                        //use SSO
                        size_t old_received = received_size;
                        std::copy(
                            data_in.data(),
                            data_in.data() + to_push,
                            sso.small_buf.data() + received_size
                        );
                        received_size += to_push;
                        data_in.data_increase(to_push);
                        return to_push;
                    }
                    else {
                        //use heap
                        if (to_push == data_in.size()) {
                            sso.large_bufs.push_back(std::move(data_in));
                        }
                        else {
                            io::buf new_buf(
                                std::span<const char>(data_in.data(), to_push)
                            );
                            sso.large_bufs.push_back(std::move(new_buf));
                            data_in.data_increase(to_push);
                        }
                        return to_push;
                    }
                }

                // push the span data into the package.
                // returns the size actually pushed.
                size_t push_span(std::span<const char> span_in) {
                    size_t to_push = std::min(span_in.size(), total_size - received_size);
                    if (to_push == 0) {
                        return 0;
                    }

                    if (total_size <= 24) {
                        //use SSO
                        size_t old_received = received_size;
                        std::copy(
                            span_in.data(),
                            span_in.data() + to_push,
                            sso.small_buf.data() + received_size
                        );
                        received_size += to_push;
                    }
                    else {
                        //use heap
                        io::buf new_buf(span_in.data(), to_push, to_push);
                        sso.large_bufs.push_back(std::move(new_buf));
                    }
                    return to_push;
                }

                // try to get the specific buffer and transfer it.
                // will return empty buf if not enough.
                io::buf get(size_t num) {
                    if (total_size <= 24) {
                        // use SSO - return small buffer
                        return io::buf(sso.small_buf.data(), total_size, total_size);
                    }
                    else {
                        // use heap - concatenate large buffers
                        if (sso.large_bufs.size() <= num) {
                            return io::buf();
                        }
                        return std::move(sso.large_bufs[num]);
                    }
                }

            private:
                //bool useSSO = false;  // if total_size <= 24, use SSO.
                size_t total_size = std::numeric_limits<size_t>::max();
                size_t received_size = 0;
                union SSO{
                    std::array<char, 24> small_buf;
                    std::vector<io::buf> large_bufs;
                } sso;
            };

            struct receiver {
                using prot_output_type = io::buf;

                io::future operator <<(io::buf& out_buf) {
                    // to be implemented
                    io::future fut;
                    return fut;
                }
                
            };
            
            struct sender {
                
            };
        }
    };
};