//warning: this memory pool allocates only and never free memories in the pool, don't use it on a big object.
#define IO_SSO_HEAP_POOLING_DECLARE(___typename)inline static std::atomic_flag lockPool = ATOMIC_FLAG_INIT;\
                                                inline static std::atomic_flag lockFreed = ATOMIC_FLAG_INIT;\
                                                static std::deque<___typename> memPool;\
                                                static std::stack<___typename*> memFreed;\
                                                static void* operator new(size_t size) noexcept;\
                                                static void operator delete(void* pthis) noexcept;

#define IO_SSO_HEAP_POOLING_DEFINE(___typename) inline std::deque<___typename> ___typename::memPool = {};\
                                                inline std::stack<___typename*> ___typename::memFreed = {};\
                                                inline void* ___typename::operator new(size_t size) noexcept\
                                                {\
                                                    while (lockFreed.test_and_set(std::memory_order_acquire));\
                                                    if (memFreed.size() == 0)\
                                                    {\
                                                        lockFreed.clear(std::memory_order_release);\
                                                        while (lockPool.test_and_set(std::memory_order_acquire));\
                                                        memPool.emplace_back();\
                                                        ___typename* ret = &*(memPool.end() - 1);\
                                                        lockPool.clear(std::memory_order_release);\
                                                        return ret;\
                                                    }\
                                                    else\
                                                    {\
                                                        ___typename* ret = memFreed.top();\
                                                        memFreed.pop();\
                                                        lockFreed.clear(std::memory_order_release);\
                                                        return ret;\
                                                    }\
                                                }\
                                                inline void ___typename::operator delete(void* pthis) noexcept\
                                                {\
                                                    while (lockFreed.test_and_set(std::memory_order_acquire));\
                                                    memFreed.push((___typename*)pthis);\
                                                    lockFreed.clear(std::memory_order_release);\
                                                }
/*
* sso heap pooling optimize simply test code:
* i5-9300HF

    constexpr auto num = 100000000;
    shared_aflag_comm*(*ptr)[] = (shared_aflag_comm*(*)[])malloc(8 * num);
    GetLastError();
    for (int i = 0; i < num; i++)
    {
        (*ptr)[i] = new shared_aflag_comm();
        delete (*ptr)[i];
    }

    MSVC -O2 costs 4953ms for un-overloaded new, 1687ms for optimized.

    also optimized the aes::BigInt object significantly
*/





class lowlevel {
    __IO_INTERNAL_HEADER_PERMISSION
    struct shared_aflag_comm {
        std::atomic_flag flag = ATOMIC_FLAG_INIT;
        std::atomic<uint32_t> ref_count = 1;
        IO_SSO_HEAP_POOLING_DECLARE(shared_aflag_comm)
    };
    struct shared_aflag {
        shared_aflag_comm* content;
        inline shared_aflag() {
            content = new shared_aflag_comm();
        }
        inline shared_aflag(const shared_aflag& other) : content(other.content) {
            content->ref_count.fetch_add(1, std::memory_order_relaxed);
        }
        inline ~shared_aflag() {
            if (content && content->ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                delete content;
            }
        }
        shared_aflag& operator=(const shared_aflag& other) = delete;
        shared_aflag& operator=(shared_aflag&& other) = delete;
        inline bool test_and_set() {
            return content->flag.test_and_set();
        }
        inline void clear() {
            content->flag.clear();
        }
        inline bool operator<(uint32_t i)
        {
            return content->ref_count < i;
        }
        inline bool operator>(uint32_t i)
        {
            return content->ref_count > i;
        }
    };

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
            complete = 0,
            aborted = 1,
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
IO_SSO_HEAP_POOLING_DEFINE(io::lowlevel::shared_aflag_comm)
