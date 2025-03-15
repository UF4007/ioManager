
//        namespace prot {
//            namespace dns {
//#include "protocol/dns.h"
//            };
//
//            namespace kcp {
//                namespace detail {
//#include "protocol/ikcp.c"
//                };
//                //protocol control block
//                struct pcb {
//                    using lowlevel_output = std::function<int(std::span<const char>)>;
//                    template<typename T_FSM>
//                    inline pcb(fsm<T_FSM>& fsm_user) {
//                        _kcp = detail::ikcp_create(0, nullptr);
//                        _fsm = fsm_user.spawn_now([](detail::ikcpcb* _kcp)->fsm_func<fsm_builtin> {
//                            auto& fsm_user = co_await io::get_fsm;
//                            io::clock delayer;
//                            while (1)
//                            {
//                                auto now = std::chrono::system_clock::now();
//                                auto duration = now.time_since_epoch();
//                                uint32_t timestamp = (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
//                                detail::ikcp_update(_kcp, timestamp);
//                                uint32_t delay_time = detail::ikcp_check(_kcp, timestamp) - timestamp;
//                                fsm_user.make_clock(delayer, std::chrono::milliseconds(delay_time));
//                                co_await delayer;
//                            }
//                            }(_kcp));
//                    }
//                    inline pcb(pcb&& right) noexcept :_kcp(right._kcp), _fsm(std::move(right._fsm)) {}
//                    pcb& operator=(pcb&& right) noexcept {
//                        if (this != &right) {
//                            if (_kcp) {
//                                detail::ikcp_release(_kcp);
//                            }
//                            _kcp = right._kcp;
//                            right._kcp = nullptr;
//                            _fsm = std::move(right._fsm);
//                        }
//                        return *this;
//                    }
//                    inline ~pcb() {
//                        if (_kcp)
//                            detail::ikcp_release(_kcp);
//                    }
//                    inline int send(const char* buffer, int len) { return detail::ikcp_send(_kcp, buffer, len); }
//                    template <typename T_FSM>
//                    inline future& future_recv(fsm<T_FSM>& state_machine) {
//                        auto status = recv_future.status();
//                        if (status == io::future::status_t::null ||
//                            status == io::future::status_t::fullfilled ||
//                            status == io::future::status_t::rejected)
//                        {
//                            state_machine.make_future(recv_future);
//                        }
//                        return recv_future;
//                    }
//                    inline auto readGetErr() { return recv_future.getErr(); }
//                    inline int recv(char* buffer, int len) { return detail::ikcp_recv(_kcp, buffer, len); }
//                    inline void setoutput(lowlevel_output output) { return detail::ikcp_setoutput(_kcp, output); }
//                    inline int input(const char* buffer, long len) {
//                        int ret = detail::ikcp_input(_kcp, buffer, len);
//                        if (detail::ikcp_peeksize(_kcp) > 0)
//                        {
//                            recv_future.getPromise().resolve_later();
//                        }
//                        return ret;
//                    }
//                    inline void input_err(std::error_code ec) { recv_future.getPromise().reject_later(ec); }
//                    inline void flush() { detail::ikcp_flush(_kcp); }
//                    inline int peeksize() const { return detail::ikcp_peeksize(_kcp); }
//                    inline int setmtu(int mtu) { return detail::ikcp_setmtu(_kcp, mtu); }
//                    inline int wndsize(int sndwnd, int rcvwnd) { return detail::ikcp_wndsize(_kcp, sndwnd, rcvwnd); }
//                    inline int waitsnd() const { return detail::ikcp_waitsnd(_kcp); }
//                    inline int nodelay(int nodelay_enable, int interval, int resend, int nc) { return detail::ikcp_nodelay(_kcp, nodelay_enable, interval, resend, nc); }
//                    template<typename... Args>
//                    inline void log(int mask, const char* fmt, Args... args) { detail::ikcp_log(_kcp, mask, fmt, args...); }
//                    IO_MANAGER_BAN_COPY(pcb);
//                private:
//                    struct fsm_builtin {};
//                    detail::ikcpcb* _kcp = nullptr;
//                    fsm_handle<fsm_builtin> _fsm;
//                    future recv_future;
//                };
//            };
//
//            namespace http {
//#include "protocol/http.h"
//            };
//
//            // software simulated protocol, the same as below
//            namespace uart {};
//
//            namespace i2c {};
//
//            namespace spi {};
//
//            namespace can {};
//
//            namespace usb {};
//        };