//referenced from: https://github.com/skywind3000/kcp
//
//2025.04.28
#pragma once
#include "../../ioManager.h"

namespace io
{
    inline namespace IO_LIB_VERSION___
    {
        namespace prot
        {
            namespace kcp_detail {
#include "ikcp.c"
            }
            using ikcpcb = kcp_detail::ikcpcb;

            struct kcp_recv_t;
            struct kcp_send_t;

            //kcp protocol control block
            // Not thread safe at all.
            struct kcp_t {
                friend struct kcp_recv_t;
                friend struct kcp_send_t;
            private:
                struct fsm_builtin {
                    kcp_detail::ikcpcb _kcp;
                    bool has_recv_prot = false;
                    bool has_send_prot = false;
                    inline fsm_builtin() {}
                    inline ~fsm_builtin() {
                        kcp_detail::ikcp_release(&_kcp);
                    }
                };
                inline fsm_func<fsm_builtin> kcp_regular() {
                    auto& fsm_user = co_await io::get_fsm;
                    io::clock delayer;
                    while (1)
                    {
                        auto now = std::chrono::system_clock::now();
                        auto duration = now.time_since_epoch();
                        uint32_t timestamp = (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
                        kcp_detail::ikcp_update(&fsm_user.data()->_kcp, timestamp);
                        uint32_t delay_time = kcp_detail::ikcp_check(&fsm_user.data()->_kcp, timestamp) - timestamp;
                        fsm_user.make_clock(delayer, std::chrono::milliseconds(delay_time));
                        co_await delayer;
                    }
                }
                inline kcp_detail::ikcpcb* getkcp() const {
                    return &_fsm.get()->data()->_kcp;
                }
                std::shared_ptr<fsm_handle<fsm_builtin>> _fsm;
            public:
                template<typename T_FSM>
                inline kcp_t(fsm<T_FSM>& fsm_user, uint32_t conv) :kcp_t(fsm_user.getManager(), conv) {}
                inline kcp_t(io::manager* manager_, uint32_t conv) {
                    _fsm = std::make_shared<fsm_handle<fsm_builtin>>(manager_->spawn_later(kcp_regular()));
                    kcp_detail::ikcp_create(getkcp(), conv, nullptr);
                }
                inline ~kcp_t() {}

                inline ikcpcb* operator->() { return &_fsm.get()->data()->_kcp; }
                inline void flush() { kcp_detail::ikcp_flush(getkcp()); }
                inline int peeksize() const { return kcp_detail::ikcp_peeksize(getkcp()); }
                inline int setmtu(int mtu) { return kcp_detail::ikcp_setmtu(getkcp(), mtu); }
                inline int wndsize(int sndwnd, int rcvwnd) { return kcp_detail::ikcp_wndsize(getkcp(), sndwnd, rcvwnd); }
                inline int waitsnd() const { return kcp_detail::ikcp_waitsnd(getkcp()); }
                inline int nodelay(int nodelay_enable, int interval, int resend, int nc) { return kcp_detail::ikcp_nodelay(getkcp(), nodelay_enable, interval, resend, nc); }
                template<typename... Args>
                inline void log(int mask, const char* fmt, Args... args) { kcp_detail::ikcp_log(getkcp(), mask, fmt, args...); }
                kcp_recv_t getRecv();
                kcp_send_t getSend(bool auto_flush = true);
            };

            //lowlevel data -> kcp recv protocol -> kcp data
            // Not thread safe at all.
            struct kcp_recv_t {
                friend struct kcp_t;

                // Output protocol: produces KCP data segments for upper layer
                using prot_output_type = std::string;

                kcp_recv_t(const kcp_recv_t&) = delete;
                kcp_recv_t& operator=(const kcp_recv_t&) = delete;

                kcp_recv_t(kcp_recv_t&& other) noexcept : _kcp_parent(std::move(other._kcp_parent)), _pending_recv_promise(std::move(other._pending_recv_promise)) {}

                kcp_recv_t& operator=(kcp_recv_t&& other) noexcept {
                    if (this != &other) {
                        _kcp_parent = std::move(other._kcp_parent);
                        _pending_recv_promise = std::move(other._pending_recv_promise);
                    }
                    return *this;
                }

                // Input protocol - process incoming KCP packet (e.g. from UDP)
                inline void operator<<(const std::span<const char>& raw_packet) {
                    // Get ikcp instance
                    auto* kcp = _kcp_parent.getkcp();
                    
                    // Feed the packet to KCP
                    kcp_detail::ikcp_input(kcp, raw_packet.data(), (long)raw_packet.size());

                    // Notify pending receive if there's one
                    auto i = kcp_detail::ikcp_peeksize(kcp);
                    if (_pending_recv_promise.valid() && i > 0) {
                        _pending_recv_promise.data()->resize(i);
                        // Try to fulfill the pending receive
                        int recv_len = kcp_detail::ikcp_recv(kcp, _pending_recv_promise.data()->data(), i);
                        if (recv_len > 0) {
                            // Data received successfully
                            _pending_recv_promise.data()->resize(recv_len);
                            _pending_recv_promise.resolve_later();
                        }
                        else {
                            _pending_recv_promise.reject(std::errc::not_enough_memory);
                        }
                    }
                }

                // Output protocol - retrieve a KCP data segment after reassembly
                inline void operator>>(future_with<prot_output_type>& out_future) {
                    io::manager* iomanager = _kcp_parent._fsm->getManager();

                    auto* kcp = _kcp_parent.getkcp();
                    
                    // Create promise
                    _pending_recv_promise = iomanager->make_future(out_future, &out_future.data);
                    
                    // Check if data is already available
                    int peek_size = kcp_detail::ikcp_peeksize(kcp);
                    if (peek_size > 0) {
                        _pending_recv_promise.data()->resize(peek_size);
                        // Data is available, retrieve it
                        int recv_len = kcp_detail::ikcp_recv(kcp, _pending_recv_promise.data()->data(), peek_size);
                        if (recv_len > 0) {
                            // Data received successfully
                            _pending_recv_promise.data()->resize(recv_len);
                            _pending_recv_promise.resolve();
                        }
                        else {
                            _pending_recv_promise.reject(std::errc::not_enough_memory);
                        }
                    }
                    // Otherwise, the promise will be fulfilled later when data arrives
                }

                ~kcp_recv_t() {
                    if (_kcp_parent._fsm != nullptr)
                        _kcp_parent._fsm.get()->data()->has_recv_prot = false;
                }

            private:
                kcp_recv_t(kcp_t kcp_parent) : _kcp_parent(kcp_parent) {
                    if (_kcp_parent._fsm.get()->data()->has_recv_prot)
                        IO_THROW(std::runtime_error("A receive protocol object for this KCP block already exists."));
                    _kcp_parent._fsm.get()->data()->has_recv_prot = true;
                }
                kcp_t _kcp_parent;
                promise<prot_output_type> _pending_recv_promise;
            };

            //kcp data -> kcp send protocol -> lowlevel data
            // Not thread safe at all.
            struct kcp_send_t {
                static constexpr size_t OUTPUT_QUEUE_MAX = 128;

                friend struct kcp_t;

                // Output protocol: produces raw KCP packets (to be sent via UDP or the other socket)
                using prot_output_type = std::string;

                kcp_send_t(const kcp_send_t&) = delete;
                kcp_send_t& operator=(const kcp_send_t&) = delete;

                kcp_send_t(kcp_send_t&& other) noexcept : _kcp_parent(std::move(other._kcp_parent)), 
                                                        _pending_send_promise(std::move(other._pending_send_promise)),
                                                        _pending_input_promise(std::move(other._pending_input_promise)),
                                                        _unsent_data(std::move(other._unsent_data)) {
                    _kcp_parent.getkcp()->user = this;
                }

                kcp_send_t& operator=(kcp_send_t&& other) noexcept {
                    if (this != &other) {
                        _kcp_parent = std::move(other._kcp_parent);
                        _pending_send_promise = std::move(other._pending_send_promise);
                        _pending_input_promise = std::move(other._pending_input_promise);
                        _unsent_data = std::move(other._unsent_data);
                        _kcp_parent.getkcp()->user = this;
                    }
                    return *this;
                }

                // Input protocol - send data through KCP
                inline future operator<<(const std::span<const char>& user_data) {
                    io::manager* iomanager = _kcp_parent._fsm->getManager();

                    // Create a future to return
                    future result;
                    _pending_input_promise = iomanager->make_future(result);
                    
                    auto i = (size_t)kcp_detail::ikcp_waitsnd(_kcp_parent.getkcp());
                    if (i > _kcp_parent.getkcp()->snd_wnd * 2) {
                        _unsent_data.resize(user_data.size());
                        std::memcpy(_unsent_data.data(), user_data.data(), user_data.size());
                        return result;
                    }

                    int ret = kcp_detail::ikcp_send(_kcp_parent.getkcp(), user_data.data(), (int)user_data.size());

                    // Check if all data was sent
                    if (ret < (int)user_data.size()) {
                        // Some data remains unsent - store it
                        size_t unsent_size = user_data.size() - ret;
                        _unsent_data.resize(unsent_size);
                        std::memcpy(_unsent_data.data(), user_data.data() + ret, unsent_size);
                    }
                    else {
                        // All data was sent successfully
                        _pending_input_promise.resolve();
                    }

                    if (auto_flush)
                        _kcp_parent.flush();
                    
                    return result;
                }

                // Output protocol - get KCP packets to be sent
                inline void operator>>(future_with<prot_output_type>& out_future) {
                    io::manager* iomanager = _kcp_parent._fsm->getManager();
                    
                    // Create promise
                    _pending_send_promise = iomanager->make_future(out_future, &out_future.data);

                    if (_output_buf.size())
                    {
                        out_future.data = std::move(_output_buf.front());
                        _output_buf.pop();
                        _pending_send_promise.resolve();
                        return;
                    }
                    
                    // Try to send any unsent data first
                    auto i = (size_t)kcp_detail::ikcp_waitsnd(_kcp_parent.getkcp());
                    if (_unsent_data.size() > 0 && _pending_input_promise.valid() && i < _kcp_parent.getkcp()->snd_wnd) {
                        int ret = kcp_detail::ikcp_send(_kcp_parent.getkcp(), _unsent_data.data(), (int)_unsent_data.size());
                        if (ret > 0) {
                            if (ret < (int)_unsent_data.size()) {
                                // Still have unsent data - update the buffer
                                size_t remaining = _unsent_data.size() - ret;
                                std::memmove(_unsent_data.data(), _unsent_data.data() + ret, remaining);
                                _unsent_data.resize(remaining);
                            } else {
                                // All data was sent
                                _unsent_data.clear();
                                // Resolve the input promise
                                _pending_input_promise.resolve_later();
                            }
                        }
                    }
                    
                    // Flush the KCP control block to generate output
                    // The output callback will handle the data and resolve the promise
                    //_kcp_parent.flush();
                }

                ~kcp_send_t() {
                    if (_kcp_parent._fsm != nullptr)
                    {
                        _kcp_parent._fsm.get()->data()->has_send_prot = false;
                        _kcp_parent.getkcp()->user = nullptr;
                    }
                }

            private:
                kcp_send_t(kcp_t kcp_parent, bool auto_flush_) : _kcp_parent(kcp_parent), auto_flush(auto_flush_) {
                    kcp_detail::ikcp_setoutput(_kcp_parent.getkcp(), +[](const char* buf, int len, kcp_detail::ikcpcb* kcp, void* user) -> int {
                        // This function captures output from KCP
                        // We'll use this in the >> operator to provide the data
                        kcp_send_t* self = static_cast<kcp_send_t*>(kcp->user);
                        if (self) {
                            if (self->_pending_send_promise.valid())
                            {
                                // Copy the data to our pending buffer
                                auto* data_ptr = self->_pending_send_promise.data();
                                if (data_ptr) {
                                    data_ptr->resize(len);
                                    std::memcpy(data_ptr->data(), buf, len);

                                    // Resolve the promise
                                    self->_pending_send_promise.resolve_later();
                                }
                            }
                            else
                            {
                                if (self->_output_buf.size() < OUTPUT_QUEUE_MAX)
                                    self->_output_buf.push(std::string(buf, len));
                                else
                                    return -1;
                            }
                            return len;
                        }
                        return -1;
                        });

                    // Store this pointer for the callback
                    _kcp_parent.getkcp()->user = this;

                    if (_kcp_parent._fsm.get()->data()->has_send_prot)
                        IO_THROW(std::runtime_error("A send protocol object for this KCP block already exists."));
                    _kcp_parent._fsm.get()->data()->has_send_prot = true;
                }
                kcp_t _kcp_parent;
                promise<prot_output_type> _pending_send_promise;
                promise<void> _pending_input_promise;
                std::string _unsent_data;  // Buffer for unsent data
                std::queue<std::string> _output_buf;
                bool auto_flush;
            };

            inline kcp_recv_t kcp_t::getRecv() { return kcp_recv_t(*this); }
            inline kcp_send_t kcp_t::getSend(bool auto_flush) { return kcp_send_t(*this, auto_flush); }
        }
    }
}