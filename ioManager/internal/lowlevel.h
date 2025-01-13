class lowlevel {
    __IO_INTERNAL_HEADER_PERMISSION
    struct awaiter;
    struct awaiter {
        struct awaiterListNode {
            awaiter* prev = nullptr;
            awaiter* next = nullptr;    //"next" also acts as a singly linked list in ioManager::finishedAwaiter
        } node;                             //this node is protected by ioManager::spinLock_tm or ioManager::spinLock_fn.
        std::chrono::steady_clock::time_point timeout;
        //awaiter** skipArrPtr = nullptr;

        std::atomic<uint32_t> count = 1;

        std::atomic_flag occupy_lock = ATOMIC_FLAG_INIT;
        std::atomic_flag set_lock = ATOMIC_FLAG_INIT;

        enum _status :char {
            idle,
            timing,
            queueing
        };
        std::atomic<_status> status;
        enum _set_status :char {
            resolve = 0,
            reject = 1,
            timeouted = 2
        }set_status;
        std::coroutine_handle<> coro = nullptr;

        io::ioManager* mngr;

        inline awaiter(io::ioManager *m) : mngr(m) {}
        awaiter(const awaiter&) = delete;
        awaiter(awaiter&&) = delete;
        awaiter& operator=(const awaiter&) = delete;
        awaiter& operator=(awaiter&&) = delete;
        ~awaiter();
    };
};

inline static asio::io_context asioManager = asio::io_context(1);
inline static auto asioManagerStart = [] {
    std::thread([] {
        asio::steady_timer jammer(asioManager, std::chrono::years(100));
        jammer.async_wait([](const asio::error_code& ec) {});
        while (1)
        {
            asioManager.run();
        }
        }).detach();
    return true;
    }();
