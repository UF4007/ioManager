#pragma once
//deprecated
namespace io {
    inline namespace IO_LIB_VERSION___ {
        namespace prot {
            // RPC protocol wrapper class
            // This class wraps io::rpc with protocol support
            template <typename key = void, typename req = void, typename rsp = void>
            struct rpc {
                __IO_INTERNAL_HEADER_PERMISSION;

                // Define protocol types
                //using prot_input_type = std::pair<key, req>;
                using prot_output_type = rsp;

                // Constructor that takes an FSM and a list of RPC handlers
                template <typename T_FSM, typename... Args>
                inline rpc(fsm<T_FSM>& state_machine, Args&&... args)
                    : executor(state_machine.getManager()), rpc_impl(std::forward<Args>(args)...) {
                }

                // Input protocol implementation
                inline future operator<<(const prot_input_type& request) {
                    future fut;
                    input_prom = executor->make_future(fut);
                    auto data_ptr = output_prom.data();
                    if (data_ptr)
                    {
                        *data_ptr = rpc_impl(request);
                        output_prom.resolve();
                        input_prom.resolve();
                    }
                    else
                    {
                        input_temp = rpc_impl(request);
                    }
                    return fut;
                }

                // Output protocol implementation
                inline void operator>>(future_with<prot_output_type>& fut) {
                    output_prom = executor->make_future(fut, &fut.data);

                    if (input_prom.valid())
                    {
                        fut.data = std::move(input_temp.value());
                        input_prom.resolve();
                        output_prom.resolve();
                    }
                }

                // Get the underlying RPC implementation
                inline io::rpc<key, req, rsp>& get_rpc() {
                    return rpc_impl;
                }

            private:
                io::manager* executor;
                io::rpc<key, req, rsp> rpc_impl;
                std::optional<rsp> input_temp;
                io::promise<> input_prom;
                io::promise<prot_output_type> output_prom;
            };
        }
    }
} 