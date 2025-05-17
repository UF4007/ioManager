#pragma once
#include "ioManager.h"
#include <unordered_map>

namespace io {
    inline namespace IO_LIB_VERSION___ {

        // helper struct of rpc<>::def.
        template <> struct rpc<void, void, void> {
            rpc() = delete;

            // default struct of rpc
            template <typename Req, typename Resp> struct def {
                using request_type = Req;
                using response_type = Resp;
                def(std::function<Resp(Req)> h) : handler(h) {}
                def() = default;

                std::function<Resp(Req)> handler;

                operator bool() const { return static_cast<bool>(handler); }
            };

            template <typename F>
            def(F) -> def<typename trait::function_traits<F>::template arg<0>::type,
                typename trait::function_traits<F>::result_type>;
        };

        template <typename key = void, typename req = void, typename rsp = void>
        struct rpc {
            __IO_INTERNAL_HEADER_PERMISSION;
            using key_type = key;
            using request_type = req;
            using response_type = rsp;
            using handler_type = std::function<response_type(request_type)>;

            template <typename... Args> inline rpc(Args &&...args) {
                process_args(std::forward<Args>(args)...);
            }

            // Call operator to invoke the appropriate handler
            template <typename K, typename R>
            inline response_type operator()(K&& req_, R&& rsp_) {
                auto it = handlers_.find(std::forward<K>(req_));
                if (it != handlers_.end()) {
                    return it->second(std::forward<R>(rsp_));
                }
                else if (default_handler_) {
                    return default_handler_(std::forward<R>(rsp_));
                }
                else {
                    IO_ASSERT(false, "rpc ERROR: No handler found for key and no default "
                        "handler provided");
                    return it->second(std::forward<R>(rsp_));
                }
            }

        private:
            std::unordered_map<key_type, handler_type> handlers_;
            handler_type default_handler_;

            template <typename... Args>
            inline void process_args(const std::pair<key_type, handler_type>& pair,
                Args &&...args) {
                handlers_[pair.first] = pair.second;
                process_args(std::forward<Args>(args)...);
            }

            template <typename Def>
                requires(
            std::is_same_v<Def, typename rpc<>::def<typename Def::request_type,
                typename Def::response_type>>&&
                std::is_convertible_v<request_type, typename Def::request_type>&&
                std::is_convertible_v<typename Def::response_type, response_type>)
                inline void process_args(const Def& default_handler) {
                if (default_handler) {
                    default_handler_ = default_handler.handler;
                }
            }

            inline void process_args() {}
        };
    } // namespace IO_LIB_VERSION___
} // namespace io