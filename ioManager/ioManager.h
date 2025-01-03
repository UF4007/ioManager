/* driver library for cross-platform IoT dev.
 * ------Head-Only------
 * 
 * for unifying various interfaces and their multiplexing provided by controllers, systems even computers.
 * 
 * ---EXPERIMENTAL LIBRARY---
 * 
 * Considering: use LVGL for display support
 * 
 * Licensed under the MIT License.
 * Looking forward to visiting https://github.com/UF4007/ to propose issues, pull your device driver, and make ioManager stronger and more universal.
*/
#pragma once
#include "internal/config.h"
#include "internal/includes.h"
namespace io
{
    //headonly version distinguish, prevent the linker from mixing differental versions when multi-reference.
    inline namespace v247a {

#include "internal/forwardDeclarations.h"

        // -----------------------------------basis---------------------------------

        //error type of this lib
        enum class err : uint8_t {
            ok = 0,
            failed = 1,
            repeat = 2,         //try again please
            notfound = 3,
            less = 4,           //more data are required
            more = 5,           //too much data was received
            bufoverf = 6,       //buffer overflow
            formaterr = 7,      //format error
        };

        using duration_ms = std::chrono::duration<unsigned long long, std::milli>;

        template <size_t _capacity>
        struct byteBuffer {
            static constexpr size_t capacity = _capacity;

            size_t size() { return depleted; }
            size_t remain() { return capacity - depleted; }
            char* data() { return _data; }
            char* push_ptr() { return _data + depleted; }
            bool push_size(size_t size) { depleted += size; return (depleted > capacity ? depleted = capacity : false); }
            void clear() { depleted = 0; }
        private:
            char _data[_capacity];
            size_t depleted = 0;
        };

        template <typename _Struc>
        class dualBuffer {                        //dual buffer, multi-thread outbound, multi-thread inbound definitely safety. Reentrancy correct.
            _Struc buffer[2];
            std::atomic_flag bufferLock[2] = { ATOMIC_FLAG_INIT, ATOMIC_FLAG_INIT };
            std::atomic<bool> rotateLock{ false };

            dualBuffer(const dualBuffer&) = delete;
            const dualBuffer& operator=(const dualBuffer&) = delete;

        public:
            inline dualBuffer() {};
            [[nodiscard]] inline _Struc* outbound_get()     //if outbound gets nullptr don't repeat otherwise deadlock.
            {
                bool expected = true;
                if (rotateLock.compare_exchange_strong(expected, true, std::memory_order_acquire, std::memory_order_relaxed))
                {
                    if (!bufferLock[0].test_and_set(std::memory_order_acquire))
                    {
                        return &buffer[0];
                    }
                }
                else
                {
                    if (!bufferLock[1].test_and_set(std::memory_order_acquire))
                    {
                        return &buffer[1];
                    }
                }
                return nullptr;
            }
            inline void outbound_unlock(_Struc* unlock)
            {
                if (unlock == &buffer[0])
                {
                    bufferLock[0].clear(std::memory_order_release);
                }
                else if (unlock == &buffer[1])
                {
                    bufferLock[1].clear(std::memory_order_release);
                }
                else
                    return;
            }
            inline void outbound_rotate()
            {
                bool expected = true;
                if (rotateLock.compare_exchange_strong(expected, false) == false)
                    rotateLock.store(true, std::memory_order_relaxed);
            }

            [[nodiscard]] inline _Struc* inbound_get()    //if inbound gets nullptr, repeat then (spinning lock) or give in then (reentrancy)
            {
                bool expected = false;
                if (rotateLock.compare_exchange_strong(expected, false, std::memory_order_acquire, std::memory_order_relaxed))
                {
                    if (!bufferLock[0].test_and_set(std::memory_order_acquire))
                        return &buffer[0];
                }
                else
                {
                    if (!bufferLock[1].test_and_set(std::memory_order_acquire))
                        return &buffer[1];
                }
                return nullptr;
            }
            inline void inbound_unlock(_Struc* unlock)
            {
                if (unlock == &buffer[0])
                {
                    bufferLock[0].clear(std::memory_order_release);
                }
                else if (unlock == &buffer[1])
                {
                    bufferLock[1].clear(std::memory_order_release);
                }
            }
        };

        struct ringBuffer;






        // ----------------------------------coroutine---------------------------------

#include "internal/lowlevel.h"

        struct coTask
        {
            __IO_INTERNAL_HEADER_PERMISSION
            struct promise_type {
                inline coTask get_return_object() { return coTask{}; }

                inline std::suspend_never initial_suspend() { return {}; }
                inline std::suspend_never final_suspend() noexcept { return {}; }

                std::suspend_always yield_value(coTask& task) = delete;

                inline void return_void() {}
                inline void unhandled_exception() { std::terminate(); }
            };

            using processPtr = coTask(*)(ioManager*);
        };

        /*
        * productor:(many threads assumed. in single productor thread and never reentry scenraio, tryOccupy() is not necessary actually.)
        * 
        *                                                                                                            |abort();             <-+
        *                       tryOccupy();----------------------------------------------------------------------->{ complete();------------+
        *                             |              Occupying                                                       |timeout();             |
        * -----------------------------------------------------------------------------------------------------------+---------------------------
        * consumer: (coroutine thread, ioManager)                                                                 ---\
        *                        |                             |                             |                        \                    <-+
        *                        |             idle            |            timing           |           owned         \__>      settled     |
        *                        |                             |                             |                                             __|
        *               coPromise(ioManager);           setTimeout(duration)            task_await()                                       reset
        *                   construct
        */
        template <typename _T = void>
        class coPromise
        {
            __IO_INTERNAL_HEADER_PERMISSION
            void cdd();
            void complete_base_local();
            void complete_base();
            lowlevel::awaiter* _base = nullptr;
        public:
            struct awaiterIntermediate      //msvc can co_await a reference of awaitable object, but gcc/clang can't
            {
                lowlevel::awaiter* _a;
                inline awaiterIntermediate(lowlevel::awaiter* awt) { _a = awt; }
                inline bool await_ready() const noexcept { return false; }
                inline void await_suspend(std::coroutine_handle<> h) { _a->coro = h; }
                inline bool await_resume() noexcept { _a->coro = nullptr; return true; }
            };

            coPromise() = default;
            coPromise(coPromiseStack<_T> &stack) noexcept;
            coPromise(coAsync<_T>&& asnyc) noexcept;
            template <typename ...Args>
            coPromise(ioManager* m, Args&&... consArgs);
            coPromise(const coPromise<_T>& right) noexcept;
            void operator=(const coPromise<_T> &right) noexcept;
            coPromise(coPromise<_T> &&right) noexcept;
            void operator=(coPromise<_T> &&right) noexcept;
            operator bool();
            inline bool operator==(void* opr) { return _base == opr; };
            ~coPromise();           //asynchronously safe

            inline ioManager* getManager() const { if (_base) return _base->mngr; else return nullptr; };

            io::err canOccupy();    //asynchronously safe
            io::err tryOccupy();    //asynchronously safe
            void unlockOccupy();    //only use it when the complete event does not actually happen.
            void reject();          //asynchronously safe, set promise to reject
            void resolve();         //asynchronously safe, set promise to resolve
            void timeout();         //asynchronously safe, set promise to timeout


            //functions beneath are only allowed used in ioManager thread.
            template <typename _Duration>
            io::err setTimeout(_Duration time);

            inline bool countCheck() {                       //if true, this count == 1, which means only this coroutine handle the promise.
                uint32_t expected = 1;
                return !_base->count.compare_exchange_strong(expected, 1);
            }
            inline std::atomic_flag* getLock() { return &_base->set_lock; }
            inline lowlevel::awaiter* getAwaiter() { return _base; };
            _T* data();

            void rejectLocal();      //local functions conserve once call of coroutine::resume(), once ready queueing and lock cost
            void resolveLocal();
            void timeoutLocal();

            bool isTiming() const;
            bool isOwned() const;
            bool isSettled() const;
            bool isResolve() const;
            bool isTimeout() const;
            bool isReject() const;

            bool reset();       //return value meaningless.
        };
        
        // coPromise on controllable memory, no need to use dynamic malloc
        template <typename _T = void>
        struct coPromiseStack
        {
            __IO_INTERNAL_HEADER_PERMISSION
            lowlevel::awaiter _awa;
            _T _content;
            template <typename... Args>
            coPromiseStack(ioManager *m, Args &&...consArgs);
        };

        template <>
        struct coPromiseStack<void>
        {
            __IO_INTERNAL_HEADER_PERMISSION
            lowlevel::awaiter _awa;
            inline coPromiseStack(ioManager *m) : _awa(m){}
        };

//thread safe task await (multi-producer, one consumer). A coPromise can be only awaited by one coroutine.
#define task_await(___cofuture___) (((___cofuture___).getLock()->test_and_set(std::memory_order_acq_rel) == false) ? (co_await io::coPromise<>::awaiterIntermediate((___cofuture___).getAwaiter())) : true) ?  (___cofuture___).data() : nullptr

//delay for certain duration
#define task_delay(___delayer___, ___chrono____) \
    do                                           \
    {                                            \
        ___delayer___.reset();                   \
        ___delayer___.setTimeout(___chrono____);              \
        task_await(___delayer___);               \
    } while (0)

        template <typename _T = void>
        struct coAsync: public coPromise<_T>
        {
            __IO_INTERNAL_HEADER_PERMISSION
            struct promise_type {
                coPromise<_T> prom;
                template<typename ...Args>
                promise_type(ioManager* m, Args&&... args);
                inline coAsync<_T> get_return_object() { return coAsync<_T>(prom); }

                inline std::suspend_never initial_suspend() { return {}; }
                inline std::suspend_never final_suspend() noexcept { return {}; }

                inline std::suspend_never yield_value(coPromise<_T>& returnvl) { returnvl = prom; return {}; }      //for visit the promise_type obj of this coroutine.

                inline void return_value(bool isResolve) {
                    if (isResolve)
                        prom.resolveLocal();
                    else
                        prom.rejectLocal();
                }
                inline void unhandled_exception() { std::terminate(); }
            };

            coAsync<_T>(const coAsync<_T>&) = delete;
            coAsync<_T>(coAsync<_T>&&) = delete;
            coAsync<_T>& operator=(const coAsync<_T>&) = delete;
            void operator=(coAsync<_T>&&) = delete;
        private:
            template<typename T>
            inline coAsync<_T>(T&& t) :coPromise<_T>(std::forward<T>(t)) {}
        };

        // multiplex awaiter, an awaiter group. When ANY awaiter in coSelector is set, the coSelector will be resumed.
        //  not thread safe. All the coPromise in it must be in the same ioManager.
        class coSelector{
            struct subTask {
                struct promise_type {
                    template <typename T>
                    inline promise_type(coSelector* mul, promise_type* next, coPromise<T> promise) { _pmul = mul; _next = next; _awa = promise._base; }
                    inline subTask get_return_object() { return subTask{ this }; }

                    inline std::suspend_never initial_suspend() { return {}; }
                    inline std::suspend_never final_suspend() noexcept { return {}; }

                    inline std::suspend_never yield_value(promise_type*& returnvl) noexcept { returnvl = this; return {}; }     //for visit the promise_type obj of this coroutine.

                    inline void return_void() {}
                    inline void unhandled_exception() { std::terminate(); }
                    coSelector* _pmul;         //if this multiplex object about to be deconstructed, this pointer will be nullptr.
                    promise_type* _next;
                    promise_type* _pending_next = nullptr;
                    lowlevel::awaiter* _awa;
                };
                inline subTask(promise_type* prom) { _prom = prom; }
                promise_type* _prom;
            };
            template<typename T>
            static subTask subCoro(coSelector* mul, subTask::promise_type* next, coPromise<T> promise);   //auxiliary coroutine delegates each coPromise. this coroutine's lifetime follows the coSelector object.
            std::coroutine_handle<> coro = nullptr;
            subTask::promise_type* first = nullptr;
            subTask::promise_type* pending = nullptr;
        public:
            struct awaiterIntermediate {
                coSelector* _a;
                inline awaiterIntermediate(coSelector* awt) { _a = awt; }
                inline bool await_ready() const noexcept { return false; }
                inline void await_suspend(std::coroutine_handle<> h) { _a->coro = h; }
                inline void await_resume() noexcept { _a->coro = nullptr; }
            };
            inline coSelector() {}
            template <typename ...Args>
            coSelector(coPromise<Args>&... arg);
            coSelector(const coSelector&) = delete;
            void operator=(const coSelector&) = delete;
            coSelector(coSelector&&) = delete;            //all of those promise_type s record this coSelector pointer.
            void operator=(coSelector&&) = delete;
            ~coSelector();

            bool processAllPending();		                //make sure that all the subCoros are not awaiting in await tag 3.

            template <typename Last>
            void add(coPromise<Last>& last);
            template <typename First, typename ...Remain>
            void add(coPromise<First>& first, coPromise<Remain>&... remain);
            template <typename T>
            err remove(coPromise<T>& right);
        };

//if ANY coPromise is be in complete(set) status, this continues.
#define task_multi_await(___coSelector___) if((___coSelector___).processAllPending())co_await io::coSelector::awaiterIntermediate(&___coSelector___)\

        // manager of coroutine tasks
        //  a ioManager == a thread
        struct ioManager final
        {
            __IO_INTERNAL_HEADER_PERMISSION

            lowlevel::awaiter::awaiterListNode timeAwaiterCentral;
            std::atomic_flag spinLock_tm = ATOMIC_FLAG_INIT;    //time based awaiter list

            std::queue<coTask::processPtr> pendingTask;
            std::atomic_flag spinLock_pd = ATOMIC_FLAG_INIT;    //pending task

            lowlevel::awaiter* readyAwaiter = nullptr;
            std::atomic_flag spinLock_rd = ATOMIC_FLAG_INIT;    //queueing to continue (includes completed or aborted awaiter)

            std::atomic_flag isEnd = ATOMIC_FLAG_INIT;
            std::atomic<bool> going = false;

            std::chrono::nanoseconds suspend_max = std::chrono::nanoseconds(100000000);   //defalut 100ms
            std::binary_semaphore suspend_sem = std::binary_semaphore(1);
        public:
            ioManager();
            ioManager(ioManager&) = delete;
            ioManager(ioManager&&) = delete;
            void operator=(ioManager& right) = delete;
            void operator=(ioManager&& right) = delete;
            ~ioManager();
            void once(coTask::processPtr ptr);
            void drive();
            void go();
            void stop(bool sync = false);
            inline bool isGoing() { return going; }

            //global automatic thread pool of ioManager
        private:
            static std::deque<ioManager> all;
            inline static std::atomic<int> getIndex = 0;
            inline static bool auto_going = false;
            static int getPendingFromAll();

        public:
            inline static bool isAutoGoing() { return auto_going; }
            static void auto_go(uint32_t threadSum = 1);
            static void auto_stop();
            static void auto_once(coTask::processPtr ptr);
        };
        inline std::deque<ioManager> ioManager::all = {};

        // channel, state machine, protocol
        // a reference count of promise_type was handled by ioChannel.
            // !!! co_yield to get promise_value at first time !!!
            //  this coroutine cannot exit before "promise_value::destroy" turns to true, else UB.
            //  when ioChannel reference count turns to 0, coroutine will be deconstructing, the parameter "promise_value::destroy" will turn true, and prom_in will emit a reject event. The coroutine will be destroyed during the next wait.
            //  use ioChannel for multi-thread usage (out of current ioManager) is UB.
        template<typename OutT, typename InT>
        struct ioChannel {
            struct promise_value {
                std::vector<io::coPromise<OutT>> out_slot;  //not owned by this ioChannel
                io::coPromise<InT> in_handle;               //lifetime owned by this ioChannel(coroutine)
                bool destroy = false;
            };
            struct promise_type {
                inline ioChannel<OutT, InT> get_return_object() { return ioChannel<OutT, InT>(this); }

                inline std::suspend_never initial_suspend() { return {}; }
                inline std::suspend_always final_suspend() noexcept { return {}; }

                template<typename ...Args>
                promise_type(ioManager* mngr, Args&&... args);

                inline std::suspend_never yield_value(promise_value** returnvl) { *returnvl = &prom_vl; return {}; }      //for visit the promise_type obj of this coroutine.

                inline void return_void() noexcept {}
                inline void unhandled_exception() { std::terminate(); }

                promise_value prom_vl;
                size_t count = 1;
            };

            coPromise<InT>* operator->();
            friend inline io::ioChannel<OutT, InT>& operator<<(io::coPromise<OutT>& bind, io::ioChannel<OutT, InT>& channel)
            {
                channel.get_slot().push_back(bind);
                return channel;
            }
            template<typename _T>
            friend inline io::ioChannel<OutT, InT>& operator<<(io::ioChannel<_T, OutT>& bind, io::ioChannel<OutT, InT>& channel)
            {
                channel.get_slot().push_back(bind.get_in_promise());
                return channel;
            }

            std::vector<io::coPromise<OutT>>& get_slot();
            io::coPromise<InT>& get_in_promise();

            ioChannel(const ioChannel&) noexcept;
            void operator=(const ioChannel&) noexcept;
            ioChannel(ioChannel&&) noexcept;
            void operator=(ioChannel&&) noexcept;
            ~ioChannel() noexcept;
        protected:
            inline ioChannel() :promise(nullptr) {}
        private:
            ioChannel(promise_type *prom);
            promise_type* promise;
            void cdd() noexcept;
        };



        // -------------------------------channel/protocol--------------------------------

        namespace http {
#include "protocol/http.h"
            ioChannel<request, std::span<uint8_t>> request_praser(ioManager* m)
            {
                using promise_value = ioChannel<request, std::span<uint8_t>>::promise_value;
                promise_value* pv;
                co_yield &pv;
                request req;
                while (1)
                {
                    std::span<uint8_t>* recv = task_await(pv->in_handle);
                    if (pv->in_handle.isResolve())
                    {
                        err ret = req.fromChar((const char*)recv->data(), recv->size());
                        if (ret == io::err::ok)
                        {
                            for (auto& out : pv->out_slot)
                            {
                                *out.data() = req;
                                req.clear();
                                out.resolveLocal();
                            }
                        }
                        else if (ret != io::err::less)
                        {
                            for (auto& out : pv->out_slot)
                            {
                                out.rejectLocal();
                            }
                            req.clear();
                        }
                    }
                    if (pv->destroy)
                        co_return;
                    pv->in_handle.reset();
                }
                co_return;
            }
            ioChannel<responce, std::span<uint8_t>> responce_praser(ioManager* m)
            {
                using promise_value = ioChannel<responce, std::span<uint8_t>>::promise_value;
                promise_value* pv;
                co_yield &pv;
                responce rsp;
                while (1)
                {
                    std::span<uint8_t>* recv = task_await(pv->in_handle);
                    if (pv->in_handle.isResolve())
                    {
                        err ret = rsp.fromChar((const char*)recv->data(), recv->size());
                        if (ret == io::err::ok)
                        {
                            for (auto& out : pv->out_slot)
                            {
                                *out.data() = rsp;
                                rsp.clear();
                                out.resolveLocal();
                            }
                        }
                        else if (ret != io::err::less)
                        {
                            for (auto& out : pv->out_slot)
                            {
                                out.rejectLocal();
                            }
                            rsp.clear();
                        }
                    }
                    if (pv->destroy)
                        co_return;
                    pv->in_handle.reset();
                }
                co_return;
            }
            ioChannel<std::string, request> request_sender(ioManager* m)
            {
                using promise_value = ioChannel<std::string, request>::promise_value;
                promise_value* pv;
                co_yield &pv;
                while (1)
                {
                    request* recv = task_await(pv->in_handle);
                    if (pv->in_handle.isResolve())
                    {
                        for (auto& out : pv->out_slot)
                        {
                            *out.data() = recv->toString();
                            out.resolveLocal();
                        }
                    }
                    if (pv->destroy)
                        co_return;
                    pv->in_handle.reset();
                }
                co_return;
            }
            ioChannel<std::string, responce> responce_sender(ioManager * m)
            {
                using promise_value = ioChannel<std::string, responce>::promise_value;
                promise_value* pv;
                co_yield &pv;
                while (1)
                {
                    responce* recv = task_await(pv->in_handle);
                    if (pv->in_handle.isResolve())
                    {
                        for (auto& out : pv->out_slot)
                        {
                            *out.data() = recv->toString();
                            out.resolveLocal();
                        }
                    }
                    if (pv->destroy)
                        co_return;
                    pv->in_handle.reset();
                }
                co_return;
            }
        };

        class adc{};

        class dac{};

        class uart{};

        class i2c{};

        class spi{};

        class can{};

        class camera{};

        class usb{};

#include "internal/definitions.h"
        }
}