namespace io
{
    inline namespace IO_LIB_VERSION___
    {
        namespace minicoro_detail
        {
            template <typename Func, typename... Args>
            struct stackful_context
            {
                Func func_ptr;
                std::tuple<Args...> args_ptr;
                stackful_context(Func &&f, Args &&...args)
                    : func_ptr(std::forward<Func>(f)), args_ptr(std::forward<Args>(args)...) {}
            };

            template <typename Func, typename... Args>
            inline void stackful_coro_entry(minicoro_detail::mco_coro *co)
            {
                io::this_manager()->current_stackful = co;
                auto *ctx = reinterpret_cast<stackful_context<Func, Args...> *>(minicoro_detail::mco_get_user_data(co));
                if constexpr (sizeof...(Args) == 0)
                {
                    (ctx->func_ptr)();
                }
                else
                {
                    std::apply(std::forward<Func>(ctx->func_ptr), std::forward<std::tuple<Args...>>(ctx->args_ptr));
                }
                io::this_manager()->stackful_deconstruct.push(co);
            }

            inline void resume(minicoro_detail::mco_coro *co)
            {
                if (co && minicoro_detail::mco_status(co) == minicoro_detail::MCO_SUSPENDED)
                {
                    minicoro_detail::mco_coro *previous = io::this_manager()->current_stackful;
                    minicoro_detail::mco_resume(co);
                    io::this_manager()->current_stackful = previous;
                }
            }
        }

        namespace stackful
        {
            template <typename Func, typename... Args>
            inline bool spawn_stacksize(size_t stack_size, Func &&func, Args &&...args)
            {
                auto ctx = minicoro_detail::stackful_context{
                    std::forward<Func>(func),
                    std::forward<Args>(args)...};

                auto desc = minicoro_detail::mco_desc_init(
                    minicoro_detail::stackful_coro_entry<Func, Args...>,
                    stack_size
                );

                desc.user_data = &ctx;

                minicoro_detail::mco_coro *co = nullptr;
                if (minicoro_detail::mco_create(&co, &desc) != minicoro_detail::MCO_SUCCESS)
                {
                    return false;
                }

                minicoro_detail::mco_coro *previous = io::this_manager()->current_stackful;
                if (minicoro_detail::mco_resume(co) != minicoro_detail::MCO_SUCCESS)
                {
                    io::this_manager()->current_stackful = previous;
                    minicoro_detail::mco_destroy(co);
                    return false;
                }
                io::this_manager()->current_stackful = previous;
                return true;
            }

            template <typename Func, typename... Args>
            inline bool spawn(Func &&func, Args &&...args)
            {
                return spawn_stacksize(0, std::forward<Func>(func), std::forward<Args>(args)...);
            }

            template <typename T>
            inline io::future_tag await(T&& fut)
            {
                if (fut.awaiter->bit_set & fut.awaiter->set_lock)
                    return fut;

                minicoro_detail::mco_coro *previous = io::this_manager()->current_stackful;
                std::function<void(lowlevel::awaiter *)> coro_set = [previous](lowlevel::awaiter *awa)
                {
                    minicoro_detail::resume(previous);
                };
                fut.awaiter->coro = &coro_set;
                minicoro_detail::mco_yield(previous);
                io::this_manager()->current_stackful = previous;

                return fut;
            }
        }
    }
}