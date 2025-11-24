



//awaiter
inline void io::lowlevel::awaiter::erase_this()
{
    if (this->bit_set & this->has_dynamic_error) {
        mngr->errc_pool.release(this->no_tm.err.value());
    }
    mngr->awaiter_hive.erase(this);
}
inline void io::lowlevel::awaiter::queue_in(await_queue* queue)
{
    while (queue->lock.test_and_set());
    this->no_tm.queue_next = this;
    std::swap(this->no_tm.queue_next, queue->queue);
    queue->lock.clear();
    mngr->suspend_release();
}
inline void io::lowlevel::awaiter::reset()
{
    if (this->bit_set & this->has_dynamic_error)
    {
        mngr->errc_pool.release(this->no_tm.err.value());
    }
    bit_set = initilaze;
    this->no_tm.err = std::error_code();
};



//fsm_base
template <typename T_spawn>
[[nodiscard]] inline io::fsm_handle<T_spawn> io::lowlevel::fsm_base::spawn_now(fsm_func<T_spawn> new_fsm)
{
    new_fsm._data->_fsm.mngr = this->mngr;
    std::coroutine_handle<fsm_promise<T_spawn>> h;
    h = h.from_promise(*new_fsm._data);
    h.resume();
    return {new_fsm._data};
}
template <typename T_Prom>
inline io::promise<T_Prom> io::lowlevel::fsm_base::make_future(io::future &fut, T_Prom *mem_bind)
{
    return this->mngr->make_future(fut, mem_bind);
}
inline io::promise<void> io::lowlevel::fsm_base::make_future(io::future &fut)
{
    return this->mngr->make_future(fut);
}
template <typename T_Duration>
inline void io::lowlevel::fsm_base::make_clock(clock &fut, T_Duration duration, bool isResolve)
{
    this->mngr->make_clock(fut, duration, isResolve);
}
template <typename T_Duration>
inline io::clock io::lowlevel::fsm_base::setTimeout(T_Duration duration, bool isResolve)
{
    clock fut;
    make_clock(fut, duration, isResolve);
    return fut;
}
inline void io::lowlevel::fsm_base::make_outdated_clock(clock& fut, bool isResolve)
{
    this->mngr->make_outdated_clock(fut, isResolve);
}
inline io::async_promise io::lowlevel::fsm_base::make_future(async_future &fut)
{
    return this->mngr->make_future(fut);
}



//promise_base
inline bool io::lowlevel::promise_base::resolve_later() {
    bool ret = false;
    if (valid())
    {
        awaiter->no_tm.queue_next = awaiter;
        std::swap(awaiter->no_tm.queue_next, awaiter->mngr->resolve_queue_local);
        awaiter->mngr->suspend_release();
        ret = true;
    }
    awaiter = nullptr;
    return ret;
}
inline bool io::lowlevel::promise_base::reject_later(std::error_code ec) {
    bool ret = false;
    if (valid())
    {
        this->awaiter->no_tm.err = ec;
        awaiter->no_tm.queue_next = awaiter;
        std::swap(awaiter->no_tm.queue_next, awaiter->mngr->resolve_queue_local);
        awaiter->mngr->suspend_release();
        ret = true;
    }
    awaiter = nullptr;
    return ret;
}
inline bool io::lowlevel::promise_base::reject_later(std::string_view message)
{
    bool ret = false;
    if (valid())
    {
        std::error_code ec(this->awaiter->mngr->errc_pool.assign(message), this->awaiter->mngr->errc_pool);
        this->awaiter->bit_set |= this->awaiter->has_dynamic_error;
        this->awaiter->no_tm.err = ec;
        awaiter->no_tm.queue_next = awaiter;
        std::swap(awaiter->no_tm.queue_next, awaiter->mngr->resolve_queue_local);
        awaiter->mngr->suspend_release();
        ret = true;
    }
    awaiter = nullptr;
    return ret;
}
inline bool io::lowlevel::promise_base::reject_later(std::string &&message)
{
    bool ret = false;
    if (valid())
    {
        std::error_code ec(this->awaiter->mngr->errc_pool.assign(std::move(message)), this->awaiter->mngr->errc_pool);
        this->awaiter->bit_set |= this->awaiter->has_dynamic_error;
        this->awaiter->no_tm.err = ec;
        awaiter->no_tm.queue_next = awaiter;
        std::swap(awaiter->no_tm.queue_next, awaiter->mngr->resolve_queue_local);
        awaiter->mngr->suspend_release();
        ret = true;
    }
    awaiter = nullptr;
    return ret;
}
inline bool io::lowlevel::promise_base::reject(std::string_view message)
{
    promise_base moved;
    moved.awaiter = this->awaiter;
    this->awaiter = nullptr;
    if (moved.valid())
    {
        std::error_code ec(moved.awaiter->mngr->errc_pool.assign(message), moved.awaiter->mngr->errc_pool);
        moved.awaiter->no_tm.err = ec;
        moved.awaiter->bit_set |= moved.awaiter->has_dynamic_error;
        moved.awaiter->bit_set |= moved.awaiter->occupy_lock;
        moved.awaiter->set();
        return true;
    }
    return false;
}
inline bool io::lowlevel::promise_base::reject(std::string &&message)
{
    promise_base moved;
    moved.awaiter = this->awaiter;
    this->awaiter = nullptr;
    if (moved.valid())
    {
        std::error_code ec(moved.awaiter->mngr->errc_pool.assign(std::move(message)), moved.awaiter->mngr->errc_pool);
        moved.awaiter->no_tm.err = ec;
        moved.awaiter->bit_set |= moved.awaiter->has_dynamic_error;
        moved.awaiter->bit_set |= moved.awaiter->occupy_lock;
        moved.awaiter->set();
        return true;
    }
    return false;
}



//all any race allsettle
template <bool returnTypeAsIndex, typename ...Args>
inline void io::lowlevel::all<returnTypeAsIndex, Args...>::deploy(future& arg) {
    assert(arg.awaiter->coro == nullptr || !"await ERROR: future is not clean: being co_awaited by another coroutine, or not processed by make_future function.");
    il[ind] = arg.awaiter;
    ind++;
}
template <bool returnTypeAsIndex, typename ...Args>
inline void io::lowlevel::all<returnTypeAsIndex, Args...>::deploy(future&& arg) {
    assert(arg.awaiter->coro == nullptr || !"await ERROR: future is not clean: being co_awaited by another coroutine, or not processed by make_future function.");
    arg.awaiter->coro = (std::function<void(awaiter*)>*)1;    //hold the lifetime, prevent from deconstruct.
    il[ind] = arg.awaiter;
    ind++;
}
template <bool returnTypeAsIndex, typename ...Args>
inline void io::lowlevel::any<returnTypeAsIndex, Args...>::deploy(future& arg) {
    assert(arg.awaiter->coro == nullptr || !"await ERROR: future is not clean: being co_awaited by another coroutine, or not processed by make_future function.");
    il[ind] = arg.awaiter;
    ind++;
}
template <bool returnTypeAsIndex, typename ...Args>
inline void io::lowlevel::any<returnTypeAsIndex, Args...>::deploy(future&& arg) {
    assert(arg.awaiter->coro == nullptr || !"await ERROR: future is not clean: being co_awaited by another coroutine, or not processed by make_future function.");
    arg.awaiter->coro = (std::function<void(awaiter*)>*)1;    //hold the lifetime, prevent from deconstruct.
    il[ind] = arg.awaiter;
    ind++;
}
template <bool returnTypeAsIndex, typename ...Args>
inline void io::lowlevel::race<returnTypeAsIndex, Args...>::deploy(future& arg) {
    assert(arg.awaiter->coro == nullptr || !"await ERROR: future is not clean: being co_awaited by another coroutine, or not processed by make_future function.");
    il[ind] = arg.awaiter;
    ind++;
}
template <bool returnTypeAsIndex, typename ...Args>
inline void io::lowlevel::race<returnTypeAsIndex, Args...>::deploy(future&& arg) {
    assert(arg.awaiter->coro == nullptr || !"await ERROR: future is not clean: being co_awaited by another coroutine, or not processed by make_future function.");
    arg.awaiter->coro = (std::function<void(awaiter*)>*)1;    //hold the lifetime, prevent from deconstruct.
    il[ind] = arg.awaiter;
    ind++;
}
template <bool returnTypeAsIndex, typename ...Args>
inline void io::lowlevel::allSettle<returnTypeAsIndex, Args...>::deploy(future& arg) {
    assert(arg.awaiter->coro == nullptr || !"await ERROR: future is not clean: being co_awaited by another coroutine, or not processed by make_future function.");
    il[ind] = arg.awaiter;
    ind++;
}
template <bool returnTypeAsIndex, typename ...Args>
inline void io::lowlevel::allSettle<returnTypeAsIndex, Args...>::deploy(future&& arg) {
    assert(arg.awaiter->coro == nullptr || !"await ERROR: future is not clean: being co_awaited by another coroutine, or not processed by make_future function.");
    arg.awaiter->coro = (std::function<void(awaiter*)>*)1;    //hold the lifetime, prevent from deconstruct.
    il[ind] = arg.awaiter;
    ind++;
}



//awaitable_base
template <typename T_FSM, bool returnTypeAsIndex, io::lowlevel::selector_status status, typename ...Args>
    requires (std::is_convertible_v<Args&, io::future&> && ...)
inline io::lowlevel::awaitable_base<T_FSM, returnTypeAsIndex, status, Args...>::awaitable_base(io::fsm_promise<T_FSM>& _fsm, std::array<awaiter*, sizeof...(Args)>&& il)
    : f_p(_fsm), await_arr(std::move(il)) {

    when_all_count = sizeof...(Args);

    for (auto& i : await_arr)
    {
        if (i->bit_set & i->set_lock)
        {
            if (coro_set_base(i) == true)
            {
                when_all_count = 0;
                return;
            }
        }
    }

    f_p._fsm.is_awaiting = true;
    coro_set = [this](awaiter* awa) {
        if (this->coro_set_base(awa) == true)
        {
            std::coroutine_handle<io::fsm_promise<T_FSM>> h;
            h = h.from_promise(this->f_p);
            h.resume();
        }
        };
    for (auto& i : await_arr)
    {
        i->coro = &coro_set;
    }
}
template <typename T_FSM, bool returnTypeAsIndex, io::lowlevel::selector_status status, typename ...Args>
    requires (std::is_convertible_v<Args&, io::future&> && ...)
inline io::future_tag io::lowlevel::awaitable_base<T_FSM, returnTypeAsIndex, status, Args...>::await_resume() noexcept requires (!returnTypeAsIndex && sizeof...(Args) == 1 && !(std::is_convertible_v<Args&, io::clock&> && ...)) {
    return { who };
}
template <typename T_FSM, bool returnTypeAsIndex, io::lowlevel::selector_status status, typename ...Args>
    requires (std::is_convertible_v<Args&, io::future&> && ...)
inline io::future_tag io::lowlevel::awaitable_base<T_FSM, returnTypeAsIndex, status, Args...>::await_resume() noexcept requires (!returnTypeAsIndex && sizeof...(Args) == 1 && (std::is_convertible_v<Args&, io::clock&> && ...)) {
    return { who };
}
template <typename T_FSM, bool returnTypeAsIndex, io::lowlevel::selector_status status, typename ...Args>
    requires (std::is_convertible_v<Args&, io::future&> && ...)
inline io::future_tag io::lowlevel::awaitable_base<T_FSM, returnTypeAsIndex, status, Args...>::await_resume() noexcept requires (!returnTypeAsIndex && sizeof...(Args) >= 2 && status == selector_status::allsettle) {
    return {};
}
template <typename T_FSM, bool returnTypeAsIndex, io::lowlevel::selector_status status, typename ...Args>
    requires (std::is_convertible_v<Args&, io::future&> && ...)
inline io::future_tag io::lowlevel::awaitable_base<T_FSM, returnTypeAsIndex, status, Args...>::await_resume() noexcept requires (!returnTypeAsIndex && sizeof...(Args) >= 2 && status != selector_status::allsettle) {
    return { who };
}
template <typename T_FSM, bool returnTypeAsIndex, io::lowlevel::selector_status status, typename ...Args>
    requires (std::is_convertible_v<Args&, io::future&> && ...)
inline int io::lowlevel::awaitable_base<T_FSM, returnTypeAsIndex, status, Args...>::await_resume() noexcept requires (returnTypeAsIndex && sizeof...(Args) == 1 && !(std::is_convertible_v<Args&, io::clock&> && ...)) {
    return which;
}
template <typename T_FSM, bool returnTypeAsIndex, io::lowlevel::selector_status status, typename ...Args>
    requires (std::is_convertible_v<Args&, io::future&> && ...)
inline int io::lowlevel::awaitable_base<T_FSM, returnTypeAsIndex, status, Args...>::await_resume() noexcept requires (returnTypeAsIndex && sizeof...(Args) == 1 && (std::is_convertible_v<Args&, io::clock&> && ...)) {
    return which;
}
template <typename T_FSM, bool returnTypeAsIndex, io::lowlevel::selector_status status, typename ...Args>
    requires (std::is_convertible_v<Args&, io::future&> && ...)
inline int io::lowlevel::awaitable_base<T_FSM, returnTypeAsIndex, status, Args...>::await_resume() noexcept requires (returnTypeAsIndex && sizeof...(Args) >= 2 && status == selector_status::allsettle) {
    return -1;
}
template <typename T_FSM, bool returnTypeAsIndex, io::lowlevel::selector_status status, typename ...Args>
    requires (std::is_convertible_v<Args&, io::future&> && ...)
inline int io::lowlevel::awaitable_base<T_FSM, returnTypeAsIndex, status, Args...>::await_resume() noexcept requires (returnTypeAsIndex && sizeof...(Args) >= 2 && status != selector_status::allsettle) {
    return which;
}



//future
inline void io::future::decons() noexcept {
    if (this->awaiter) {
        if ((this->awaiter->bit_set & this->awaiter->promise_handled) == false &&
            this->awaiter->coro == nullptr)
            this->awaiter->erase_this();
        else
            this->awaiter->bit_set &= ~this->awaiter->future_handled;
    }
}
inline io::promise<void> io::future::getPromise() {
    if (awaiter)
    {
        if ((awaiter->promise_handled & awaiter->bit_set) == false &&
            (awaiter->occupy_lock & awaiter->bit_set) == false)
        {
            awaiter->bit_set |= awaiter->promise_handled;
            return io::promise<>(awaiter);
        }
    }
    return {};
}
//future with
template<typename T>
    requires (!std::is_same_v<T, void>)
inline io::promise<T> io::future_with<T>::getPromise() {
    if (awaiter)
    {
        if ((awaiter->promise_handled & awaiter->bit_set) == false &&
            (awaiter->occupy_lock & awaiter->bit_set) == false)
        {
            awaiter->bit_set |= awaiter->promise_handled;
            return io::promise<T>(awaiter, &this->data);
        }
    }
    return {};
}



//clock
inline void io::clock::decons() noexcept {
    if (this->awaiter) {
        if ((this->awaiter->bit_set & this->awaiter->set_lock) == false &&
            this->awaiter->coro == nullptr)
        {
            this->awaiter->bit_set &= ~this->awaiter->promise_handled;
            this->awaiter->mngr->time_chain.erase(this->awaiter->tm);
        }
    }
}
inline bool io::clock::set() {
    if (this->awaiter)
    {
        if ((this->awaiter->bit_set & this->awaiter->set_lock) == false)
        {
            this->awaiter->mngr->time_chain.erase(this->awaiter->tm);
            promise<void> prom(this->awaiter);
            prom.resolve();
            return true;
        }
        return false;
    }
    return false;
}
inline bool io::clock::set_later() {
    if (this->awaiter)
    {
        if ((this->awaiter->bit_set & this->awaiter->set_lock) == false)
        {
            this->awaiter->mngr->time_chain.erase(this->awaiter->tm);
            promise<void> prom(this->awaiter);
            prom.resolve_later();
            return true;
        }
        return false;
    }
    return false;
}



//timer
//inline void io::timer::reset(
//    mode_t mode,
//    std::chrono::steady_clock::duration interval,
//    size_t _stop_count,
//    size_t _count)
//{
//    if (this->_mode == counter ||
//        this->_mode == up_timer)
//        when_count_future2.getPromise().reject(std::make_error_code(std::errc::connection_reset));
//    when_stop_future.getPromise().reject(std::make_error_code(std::errc::connection_reset));
//    if (interval != std::chrono::seconds(0))
//    {
//        this->_interval = interval;
//    }
//    this->count_num = _count;
//    if (mode != still_mode)
//    {
//        this->_mode = mode;
//    }
//    if (_stop_count != 0)
//    {
//        this->stop_count = _stop_count;
//    }
//}



//async_promise
inline void io::async_promise::decons(lowlevel::awaiter* exchange_ptr) noexcept {
    lowlevel::awaiter* old_value = awaiter.exchange(exchange_ptr);
    if (old_value) {
        old_value->queue_in(&old_value->mngr->prom_decons_queue);
    }
}
inline bool io::async_promise::resolve() {
    lowlevel::awaiter* temp = this->awaiter.exchange(nullptr);
    if (temp) {
        temp->queue_in(&temp->mngr->resolve_queue);
        return true;
    }
    return false;
}
inline bool io::async_promise::reject(std::error_code ec) {
    lowlevel::awaiter* temp = this->awaiter.exchange(nullptr);
    if (temp) {
        temp->no_tm.err = ec;
        temp->queue_in(&temp->mngr->resolve_queue);
        return true;
    }
    return false;
}

#if IO_USE_ASIO
// fromAsio implementation for void result type
template <typename T, typename Executor>
inline void io::lowlevel::fsm_base::fromAsio(future& fut, asio::awaitable<T, Executor>&& awaitable_obj) {
    mngr->fromAsio(fut, std::move(awaitable_obj));
}

// fromAsio implementation for non-void result type
template <typename T, typename Executor>
inline void io::lowlevel::fsm_base::fromAsio(future_with<T>& fut, asio::awaitable<T, Executor>&& awaitable_obj) {
    mngr->fromAsio(fut, std::move(awaitable_obj));
}
#endif


//future
inline bool io::future::rethrow(io::lowlevel::promise_base& prom_base) {
    // Check if this future has an error
    if (!this->awaiter || !(this->awaiter->no_tm.err)) {
        return false;  // No error
    }
    
    // If the error has dynamic error message (custom string)
    if (this->awaiter->bit_set & this->awaiter->has_dynamic_error) {
        // Transfer the error code and dynamic error flag directly
        // without creating a new string (reuse the error pool reference)
        if (prom_base.valid()) {
            auto tmp = this->awaiter->no_tm.err;
            this->awaiter->no_tm.err = std::error_code();
            prom_base.awaiter->bit_set |= prom_base.awaiter->has_dynamic_error;
            this->awaiter->bit_set &= ~(this->awaiter->has_dynamic_error);
            prom_base.reject(tmp);
        }
    } else {
        // Regular error code without custom message - just copy the error code
        if (this->awaiter) {
            prom_base.reject(this->awaiter->no_tm.err);
        }
    }
    
    return true;  // Had an error
}


//manager
template <typename Func, typename ...Args>
inline io::async_future io::manager::post(pool& thread_pool, Func func, Args&&... args)
{
	return thread_pool.post(this, func, std::forward<Args>(args)...);
}