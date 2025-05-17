#pragma once
#include "../ioManager.h"

namespace io
{
    inline namespace IO_LIB_VERSION___
    {
        namespace prot
        {
            struct packet_sentinel {
            private:
                struct packet_t{
                    std::string content;
                    std::chrono::steady_clock::time_point tp;
                };
                struct built_in {
                    io::promise<std::span<char>> promise;
                    std::queue<packet_t> queue;
                    bool activated = true;
                    std::mt19937 generator;
                    std::uniform_int_distribution<size_t> size_distribution;
                    size_t package_sum_limit;
                    built_in() :generator(std::random_device{}()) {}
                };
            public:
                // Output protocol type definition
                using prot_output_type = std::span<char>;

                // Callback function type definitions
                using good_packet_callback = std::function<void(std::span<char>, std::chrono::steady_clock::duration delay)>;
                using bad_packet_callback = std::function<void(std::span<char>, std::span<char>, std::chrono::steady_clock::duration delay)>;// params: received, correct

                // Constructor, sets packet size range, time interval and callback functions
                packet_sentinel(
                    manager* manager_,
                    size_t min_packet_size,
                    size_t max_packet_size,
                    std::chrono::milliseconds interval,
                    good_packet_callback good_cb = nullptr,
                    bad_packet_callback bad_cb = nullptr,
                    size_t package_sum_limit = 100
                ) : good_callback(good_cb),
                    bad_callback(bad_cb)
                {
                    // Create internal coroutine
                    fsm_handle = manager_->spawn_later(packet_generator(interval));
                    fsm_handle->size_distribution = std::uniform_int_distribution<size_t>(min_packet_size, max_packet_size);
                    fsm_handle->package_sum_limit = package_sum_limit;
                }

                // Destructor, cleans up resources
                ~packet_sentinel() {}

                // Output protocol implementation - passes data through future to coroutine
                void operator>>(io::future_with<std::span<char>>& fut) {
                    fsm_handle->promise = fsm_handle.getManager()->make_future(fut, &fut.data);
                }

                // Input protocol implementation - directly receives data and compares with queued packets
                void operator<<(std::span<char> data) {
                    if (fsm_handle->queue.empty()) {
                        // No expected packets, trigger bad packet callback
                        if (bad_callback) {
                            bad_callback(data, {}, std::chrono::seconds(0));
                        }
                        return;
                    }

                    // Get the first packet from queue
                    auto expected_packet = std::move(fsm_handle->queue.front());
                    fsm_handle->queue.pop();

                    auto now = std::chrono::steady_clock::now();
                    // Compare data
                    if (compare_packets(expected_packet.content, data)) {
                        // Match, trigger good packet callback
                        if (good_callback) {
                            good_callback(data, now - expected_packet.tp);
                        }
                    }
                    else {
                        // No match, trigger bad packet callback
                        if (bad_callback) {
                            bad_callback(data, expected_packet.content, now - expected_packet.tp);
                        }
                    }
                }

                // Set callback functions
                void set_good_callback(good_packet_callback cb) {
                    good_callback = cb;
                }

                void set_bad_callback(bad_packet_callback cb) {
                    bad_callback = cb;
                }

                void pause()
                {
                    fsm_handle->activated = false;
                }

                void resume()
                {
                    fsm_handle->activated = true;
                }

            private:
                // Internal coroutine function, generates random data packets
                static fsm_func<built_in> packet_generator(auto interval) {
                    fsm<built_in>& fsm = co_await io::get_fsm;
                    io::clock delayer;

                    while (true) {
                        fsm.make_clock(delayer, interval);
                        co_await delayer;

                        if (!fsm->activated || !fsm->promise.valid() || fsm->package_sum_limit < fsm->queue.size()) {
                            continue;
                        }

                        // Generate random packet size
                        size_t packet_size = fsm->size_distribution(fsm->generator);

                        std::string buffer;
                        buffer.resize(packet_size);

                        // Generate random data
                        for (size_t i = 0; i < packet_size; i += sizeof(uint64_t)) {
                            uint64_t random_value = std::uniform_int_distribution<uint64_t>(0, UINT64_MAX)(fsm->generator);

                            if (i + sizeof(uint64_t) <= packet_size) {
                                *reinterpret_cast<uint64_t*>(buffer.data() + i) = random_value;
                            }
                            else {
                                size_t remaining = packet_size - i;
                                std::memcpy(buffer.data() + i, &random_value, remaining);
                            }
                        }
                        fsm->promise.resolve_later(buffer);
                        fsm->queue.push({ std::move(buffer) ,std::chrono::steady_clock::now() });
                    }

                    co_return;
                }

                // Compare two data packets for equality
                bool compare_packets(std::span<char> expected, std::span<char> actual) {
                    if (expected.size() != actual.size()) {
                        return false;
                    }

                    return std::equal(expected.begin(), expected.end(), actual.begin());
                }

                good_packet_callback good_callback;
                bad_packet_callback bad_callback;

                io::fsm_handle<built_in> fsm_handle;
            };
        }
    }
}