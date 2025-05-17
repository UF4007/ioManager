class lowlevel {
    __IO_INTERNAL_HEADER_PERMISSION
    struct awaiter;
    struct await_queue {
        lowlevel::awaiter* queue = nullptr;
        std::atomic_flag lock = ATOMIC_FLAG_INIT;
    };
    struct awaiter {
        static constexpr int promise_handled = 1 << 0;
        static constexpr int future_handled = 1 << 1;
        static constexpr int occupy_lock = 1 << 2;
        static constexpr int set_lock = 1 << 3;
        static constexpr int is_clock = 1 << 4;
        static constexpr int clock_resolve = 1 << 5;    // clock treated as resolve, rather than default reject.
        static constexpr int initilaze = promise_handled | future_handled;

        int bit_set = initilaze;

        std::function<void(awaiter*)>* coro = nullptr;    //lambda type erasure. (virtual)

        union {
            std::multimap<std::chrono::steady_clock::time_point, awaiter*>::iterator tm;
            struct {
                std::error_code err;
                awaiter* queue_next;
            } no_tm;
        };
        manager* mngr;
        inline void erase_this();
        inline void queue_in(await_queue* queue);
        inline awaiter() {
            this->no_tm.err = std::error_code();
        }
        inline ~awaiter() {}
        inline void reset() {
            bit_set = initilaze;
            this->no_tm.err = std::error_code();
        };
        inline void set() {
            this->bit_set |= set_lock;
            if (coro)
                std::invoke(*coro, this);
        }
    };
    struct promise_base {
        __IO_INTERNAL_HEADER_PERMISSION
        promise_base(const promise_base&) = delete;
        promise_base& operator=(const promise_base&) = delete;
        inline promise_base(promise_base&& right) noexcept :awaiter(right.awaiter) {
            right.awaiter = nullptr;
        }
        inline promise_base& operator=(promise_base&& right) noexcept {
            decons();
            this->awaiter = right.awaiter;
            right.awaiter = nullptr;
            return *this;
        }
        inline ~promise_base() {
            decons();
        }
        inline promise_base() {}
        inline bool valid() {
            if (this->awaiter == nullptr)
                return false;
            return (awaiter->occupy_lock & awaiter->bit_set) == false && 
                (awaiter->future_handled & awaiter->bit_set ||
                    awaiter->coro != nullptr);
        }
        inline bool resolve() {
            bool ret = false;
            if (valid())
            {
                awaiter->bit_set |= awaiter->occupy_lock;
                this->awaiter->set();
                ret = true;
            }
            this->decons();
            awaiter = nullptr;
            return ret;
        }
        inline bool resolve_later();
        inline bool reject(std::error_code ec) {
            bool ret = false;
            if (valid())
            {
                this->awaiter->no_tm.err = ec;
                awaiter->bit_set |= awaiter->occupy_lock;
                this->awaiter->set();
                ret = true;
            }
            this->decons();
            awaiter = nullptr;
            return ret;
        }
        inline bool reject_later(std::error_code ec);
        inline bool reject(std::errc ec) { return reject(std::make_error_code(ec)); }
        inline bool reject_later(std::errc ec) { return reject_later(std::make_error_code(ec)); }
    private:
        inline promise_base(lowlevel::awaiter* a) noexcept :awaiter(a) {}
        lowlevel::awaiter* awaiter = nullptr;
        inline void decons() noexcept {
            if (this->awaiter) {
                if ((this->awaiter->bit_set & this->awaiter->future_handled) == false &&
                    this->awaiter->coro == nullptr)
                    this->awaiter->erase_this();
                else
                    this->awaiter->bit_set &= ~this->awaiter->promise_handled;
            }
        }
    };
    
    struct fsm_base
    {
        __IO_INTERNAL_HEADER_PERMISSION
        inline manager *getManager() { return mngr; }
        template <typename T_Prom>
        promise<T_Prom> make_future(future &fut, T_Prom *mem_bind);
        promise<void> make_future(future &fut);
        // make a clock
        template <typename T_Duration>
        void make_clock(io::clock&fut, T_Duration duration, bool isResolve = false);
        template <typename T_Duration>
        io::clock setTimeout(T_Duration duration, bool isResolve = false);
        inline void make_outdated_clock(io::clock& fut, bool isResolve = false);
        // make async future pair
        async_promise make_future(async_future &fut);
        // sync co_spawn, run coroutine immediately.
        template <typename T_spawn>
        [[nodiscard]] fsm_handle<T_spawn> spawn_now(fsm_func<T_spawn> new_fsm);
        
#if IO_USE_ASIO
        // Convert asio::awaitable to io::future
        template <typename T, typename Executor>
        void fromAsio(future& fut, asio::awaitable<T, Executor>&& awaitable_obj);
        
        // Convert asio::awaitable to io::future_with
        template <typename T, typename Executor>
        void fromAsio(future_with<T>& fut, asio::awaitable<T, Executor>&& awaitable_obj);
#endif

        inline bool detached() { return is_detached; }
        inline bool deconstructing() { return is_deconstructing; }
        fsm_base(const fsm_base &) = delete;
        fsm_base &operator=(const fsm_base &) = delete;
        fsm_base(fsm_base &&right) = delete;
        fsm_base &operator=(fsm_base &&right) = delete;
        fsm_base() = default;

    private:
        io::manager *mngr;
        bool is_awaiting = false;
        bool is_detached = false;
        bool is_deconstructing = false; // is in delay deconstruction list?
    };

    struct awa_awaitable
    {
        inline awa_awaitable(std::coroutine_handle<>* p, bool* ps) :ptr(p), simple_awaiter(ps) {}
        inline bool await_ready() noexcept { return false; }
        inline void await_suspend(std::coroutine_handle<> h) {
            *ptr = h;
        }
        inline void await_resume() noexcept {
            *ptr = nullptr;
            *simple_awaiter = false;
        }
        std::coroutine_handle<>* ptr;
        bool* simple_awaiter;
    };
    template <typename T_spawn>
    struct awa_initial_suspend {
        io::fsm<T_spawn>* _pthis;
        inline awa_initial_suspend(io::fsm<T_spawn>* pthis):_pthis(pthis){}
        constexpr bool await_ready() const noexcept {
            return false;
        }
        constexpr void await_suspend(std::coroutine_handle<>) const noexcept {}
        constexpr void await_resume() const noexcept {
            if constexpr (io::is_future_with<T_spawn>::value)
            {
                _pthis->mngr->make_future(_pthis->_data, &_pthis->_data.data);
            }
            else if constexpr (io::is_future<T_spawn>::value)
            {
                _pthis->mngr->make_future(_pthis->_data);
            }
        }
    };
    enum class selector_status {
        //not_await = 0,      //condition has been fulfilled. not await.
        all = 1,            //all resolve or any reject
        any = 2,            //any resolve or all reject
        race = 3,           //race the same as in a selector
        allsettle = 4       //all settle
    };
    //general future awaitable
    template <typename T_FSM, selector_status status, typename ...Args>
        requires (std::is_convertible_v<Args&, io::future&> && ...)
    struct awaitable_base {
        fsm_func<T_FSM>::promise_type& f_p;
        uint32_t when_all_count;
        int who;
        std::array<awaiter*, sizeof...(Args)> await_arr;

        std::function<void(awaiter*)> coro_set; // lambda type erasure

        inline bool coro_set_base(awaiter* awa) // returns true to fulfill
        {
            bool isReject = (awa->bit_set & awa->is_clock && (awa->bit_set & awa->clock_resolve) == false) || awa->no_tm.err.operator bool();
            auto findWhoEnd = [&] {
                for (int i = 0; i < sizeof...(Args); i++)
                {
                    if (std::memcmp(&await_arr[i], &awa, sizeof(this)) == 0)
                    {
                        who = i;
                        return;
                    }
                }
                IO_ASSERT(false, "awaiter mismatch!");
            };
            auto whenAll = [&]()->bool {
                when_all_count--;
                if (when_all_count == 0)
                {
                    who = -1;
                    return true;
                }
                return false;
            };
            if constexpr (status == selector_status::all)
            {
                if (isReject)
                {
                    findWhoEnd();
                    return true;
                }
                else
                {
                    if (whenAll())
                        return true;
                }
            }
            else if constexpr (status == selector_status::any)
            {
                bool isResolve = !isReject;
                if (isResolve)
                {
                    findWhoEnd();
                    return true;
                }
                else
                {
                    if (whenAll())
                        return true;
                }
            }
            else if constexpr (status == selector_status::race)
            {
                findWhoEnd();
                return true;
            }
            else if constexpr (status == selector_status::allsettle)
            {
                if (whenAll())
                    return true;
            }
            else
            {
                static_assert(!std::is_same_v<T_FSM, T_FSM>, "invalid args!");
            }
            return false;
        }
        awaitable_base(fsm_func<T_FSM>::promise_type& _fsm, std::array<awaiter*, sizeof...(Args)>&& il);
        inline bool await_ready() noexcept { return when_all_count == 0; }
        inline void await_suspend(std::coroutine_handle<> h) {}
        inline int await_resume() noexcept requires (sizeof...(Args) == 1 && !(std::is_convertible_v<Args&, io::clock&> && ...)) {
            return 0;
        }
        inline int await_resume() noexcept requires (sizeof...(Args) == 1 && (std::is_convertible_v<Args&, io::clock&> && ...)) {
            return 0;
        }
        inline int await_resume() noexcept requires (sizeof...(Args) >= 2 && status == selector_status::allsettle) {
            return -1;
        }
        inline int await_resume() noexcept requires (sizeof...(Args) >= 2 && status != selector_status::allsettle) {
            return who;
        }
        inline ~awaitable_base() {
            for (auto& awa : await_arr)
            {
                if ((awa->bit_set & awa->future_handled) == false)
                {
                    if ((awa->bit_set & awa->promise_handled) == false)
                    {
                        awa->erase_this();
                        continue;
                    }
                    else if ((awa->bit_set & awa->is_clock) && (awa->bit_set & awa->set_lock) == false)
                    {
                        awa->mngr->time_chain.erase(awa->tm);
                        awa->erase_this();
                        continue;
                    }
                }
                awa->coro = nullptr;
            }
            f_p._fsm.is_awaiting = false;
        }
    };

    //store rvalue reference
    template<typename... Args>
    class value_carrier {
        std::tuple<typename std::conditional_t<
            std::is_rvalue_reference_v<Args&&>,
            std::remove_reference_t<Args>,
            std::tuple<>
        >...> values; 
    public:
        template<typename... UArgs>
        value_carrier(UArgs&&... args)
            : values(store_if_rvalue<Args>(std::forward<UArgs>(args))...) {
        }
    private:
        template<typename T, typename U>
        static auto store_if_rvalue(U&& arg) {
            if constexpr (std::is_rvalue_reference_v<T&&>) {
                return std::forward<U>(arg);
            }
            else {
                return std::tuple<>();
            }
        }
    };
    template<typename... Args>
    value_carrier(Args&&...) -> value_carrier<Args&&...>;

    //all resolve or any reject
    template <typename ...Args>
    struct all {
        __IO_INTERNAL_HEADER_PERMISSION
            inline all(Args&&... args) {
            (deploy(std::forward<Args>(args)), ...);
        }
    private:
        all(all&) = default;
        all(all&&) = default;
        inline void deploy(future& arg);
        inline void deploy(future&& arg);
        size_t ind = 0;
        std::array<lowlevel::awaiter*, sizeof...(Args)> il;
    };
    //any resolve or all reject
    template <typename ...Args>
    struct any {
        __IO_INTERNAL_HEADER_PERMISSION
            inline any(Args&&... args) {
            (deploy((std::forward<Args>(args))), ...);
        }
    private:
        any(any&) = default;
        any(any&&) = default;
        inline void deploy(future& arg);
        inline void deploy(future&& arg);
        size_t ind = 0;
        std::array<lowlevel::awaiter*, sizeof...(Args)> il;
    };
    //race the same as in a selector
    template <typename ...Args>
    struct race {
        __IO_INTERNAL_HEADER_PERMISSION
            inline race(Args&&... args) {
            (deploy((std::forward<Args>(args))), ...);
        }
    private:
        race(race&) = default;
        race(race&&) = default;
        inline void deploy(future& arg);
        inline void deploy(future&& arg);
        size_t ind = 0;
        std::array<lowlevel::awaiter*, sizeof...(Args)> il;
    };
    //all settle
    template <typename ...Args>
    struct allSettle {
        __IO_INTERNAL_HEADER_PERMISSION
            inline allSettle(Args&&... args) {
            (deploy((std::forward<Args>(args))), ...);
        }
    private:
        allSettle(allSettle&) = default;
        allSettle(allSettle&&) = default;
        inline void deploy(future& arg);
        inline void deploy(future&& arg);
        size_t ind = 0;
        std::array<lowlevel::awaiter*, sizeof...(Args)> il;
    };
};