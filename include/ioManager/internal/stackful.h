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
            inline void stackful_coro_entry(mco_coro *co)
            {
                io::this_manager()->current_stackful = co;
                auto *ctx = reinterpret_cast<stackful_context<Func, Args...> *>(mco_get_user_data(co));
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

            inline void resume(mco_coro *co)
            {
                if (co && mco_status(co) == MCO_SUSPENDED)
                {
                    mco_coro *previous = io::this_manager()->current_stackful;
                    mco_resume(co);
                    io::this_manager()->current_stackful = previous;
                }
            }
        }

        namespace stackful
        {
            template <typename Func, typename... Args>
            inline bool spawn_stacksize(size_t stack_size, Func &&func, Args &&...args)
            {
                auto ctx = minicoro_detail::stackful_context<Func, Args...>{
                    std::forward<Func>(func),
                    std::forward<Args>(args)...};

                auto desc = mco_desc_init(
                    minicoro_detail::stackful_coro_entry<Func, Args...>,
                    stack_size
                );

                desc.user_data = &ctx;

                mco_coro *co = nullptr;
                if (mco_create(&co, &desc) != MCO_SUCCESS)
                {
                    return false;
                }

                mco_coro *previous = io::this_manager()->current_stackful;
                if (mco_resume(co) != MCO_SUCCESS)
                {
                    io::this_manager()->current_stackful = previous;
                    mco_destroy(co);
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

                mco_coro *previous = io::this_manager()->current_stackful;
                std::function<void(lowlevel::awaiter *)> coro_set = [previous](lowlevel::awaiter *awa)
                {
                    minicoro_detail::resume(previous);
                };
                fut.awaiter->coro = &coro_set;
                mco_yield(previous);
                fut.awaiter->coro = nullptr;
                io::this_manager()->current_stackful = previous;

                return fut;
            }
        }
    }
}