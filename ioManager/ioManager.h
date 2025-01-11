/* driver library for cross-platform IoT dev.
 * ------Head-Only------
 * 
 * for unifying various interfaces and their multiplexing provided by controllers, systems even computers.
 * 
 * ---EXPERIMENTAL LIBRARY---
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

        //For temporary use. Not for library use.
        template <typename T>
        struct ban_copy : public T {
            ban_copy(const ban_copy&) = delete;
            ban_copy& operator =(const ban_copy&) = delete;
        };

        //For temporary use. Not for library use.
        template <typename T>
        struct ban_copy_and_move :public T {
            ban_copy_and_move(const ban_copy_and_move&) = delete;
            ban_copy_and_move& operator =(const ban_copy_and_move&) = delete;
            ban_copy_and_move(ban_copy_and_move&&) = delete;
            ban_copy_and_move& operator =(ban_copy_and_move&&) = delete;
        };

        struct buffer {
            inline buffer(size_t capacity) { this->capacity = capacity; _data.resize(capacity); }
            inline size_t size() { return depleted; }
            inline size_t remain() { return capacity - depleted; }
            inline char* data() { return _data.data(); }
            inline char* push_ptr() { return _data.data() + depleted; }
            inline bool push_size(size_t size) { depleted += size; return (depleted > capacity ? depleted = capacity : false); }
            inline void clear() { depleted = 0; }
        private:
            size_t capacity;
            std::string _data;
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
        // one promise can only be awaited by one coroutine.
        template <typename _T = void>
        class coPromise
        {
            __IO_INTERNAL_HEADER_PERMISSION
            void cdd();
            void complete_base_local();
            void complete_base();
            lowlevel::awaiter* _base = nullptr;
        public:
            struct awaitable
            {
                inline awaitable(coPromise<_T>* prom) :_prom(prom) {}
                inline bool await_ready() noexcept { 
                    if (_prom->_base->set_lock.test_and_set(std::memory_order_acq_rel) == false)
                        return false;
                    waited = false;
                    return true;
                }
                inline void await_suspend(std::coroutine_handle<> h) { _prom->_base->coro = h; }
                inline _T* await_resume() noexcept { if (waited)_prom->_base->coro = nullptr; return _prom->data(); }
            private:
                coPromise<_T>* _prom;
                bool waited = true;
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
            template <typename _Duration>
            inline awaitable delay(_Duration time)
            {
                this->setTimeout(time);
                return this->operator*();
            }

            inline bool countCheck() {                       //if true, this count == 1, which means only this coroutine handle the promise.
                uint32_t expected = 1;
                return !_base->count.compare_exchange_strong(expected, 1);
            }
            inline awaitable operator*() { return { this }; }
            inline awaitable getAwaiter() { return { this }; }
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

            void reset();
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

            coAsync(const coAsync<_T>&) = delete;
            coAsync(coAsync<_T>&&) = delete;
            void operator=(const coAsync<_T>&) = delete;
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
            struct awaitable {
                coSelector* _a;
                bool waited = true;
                inline awaitable(coSelector* awt) { _a = awt; }
                inline bool await_ready() noexcept {
                    if (_a->processAllPending())
                        return false;
                    waited = false;
                    return true;
                }
                inline void await_suspend(std::coroutine_handle<> h) { _a->coro = h; }
                inline void await_resume() noexcept { if (waited)_a->coro = nullptr; }
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
            inline awaitable operator*() { return { this }; }
            inline awaitable getAwaiter() { return { this }; }

            template <typename Last>
            void add(coPromise<Last>& last);
            template <typename First, typename ...Remain>
            void add(coPromise<First>& first, coPromise<Remain>&... remain);
            template <typename T>
            err remove(coPromise<T>& right);
        };

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



        // -------------------------------channel/protocol--------------------------------

        namespace tcp {
            struct socket : public asio::ip::tcp::socket {
                inline socket() :asio::ip::tcp::socket(asioManager) {}

                template<typename ...Args>
                    requires std::is_constructible_v<asio::ip::tcp::socket, Args...>
                inline socket(size_t buffer_capacity, ioManager* mngr, Args&&... args) :
                    asio::ip::tcp::socket(std::forward<Args>(args)...),
                    _prom_recv(mngr, buffer_capacity),
                    _prom_send(mngr) {}

                template<typename ...Args>
                    requires std::is_constructible_v<asio::ip::tcp::socket, asio::io_context, Args...>
                inline socket(size_t buffer_capacity, ioManager* mngr, Args&&... args) :
                    asio::ip::tcp::socket(asioManager, std::forward<Args>(args)...),
                    _prom_recv(mngr, buffer_capacity),
                    _prom_send(mngr) {
                }

                template<typename ...Args>
                    requires requires (Args&&... args) { asio::buffer(std::forward<Args>(args)...); }
                inline coPromise<> send_io(Args&&... args)
                {
                    _prom_send.reset();
                    coPromise<> prom = _prom_send;
                    this->async_send(asio::buffer(std::forward<Args>(args)...),
                        [prom](const asio::error_code& ec, size_t length) mutable {
                        if (prom.tryOccupy() == io::err::ok)
                        {
                            if (!ec)
                            {
                                prom.resolve();
                            }
                            else
                            {
                                prom.reject();
                            }
                        }
                        }
                    );
                    return _prom_send;
                }

                inline coPromise<buffer> recv_io()
                {
                    _prom_recv.reset();
                    buffer* data = _prom_recv.data();
                    coPromise<buffer> prom = _prom_recv;
                    this->async_read_some(asio::buffer((void*)data->push_ptr(),data->remain()), [prom](const asio::error_code& ec, size_t n) mutable {
                        if (prom.tryOccupy() == io::err::ok)
                        {
                            if (!ec)
                            {
                                prom.data()->push_size(n);
                                prom.resolve();
                            }
                            else
                            {
                                prom.reject();
                            }
                        }
                        });
                    return _prom_recv;
                }
            private:
                coPromise<buffer> _prom_recv;
                coPromise<> _prom_send;
            };
            // all accepted sockets are owned by the ioManager which the constructor gave.
            struct acceptor : public asio::ip::tcp::acceptor {
                
                template<typename ...Args>
                inline acceptor(size_t buffer_capacity, ioManager* mngr, Args&&... args) :
                    asio::ip::tcp::acceptor(asioManager, std::forward<Args>(args)...),
                    _prom(mngr),
                    buf_cap(buffer_capacity) {}

                inline coPromise<tcp::socket> accept_io() {
                    _prom.reset();
                    coPromise<tcp::socket> prom = _prom;
                    size_t buf_cap = this->buf_cap;
                    this->async_accept([prom, buf_cap](const asio::error_code& ec, asio::ip::tcp::socket sock) mutable {
                        if (prom.tryOccupy() == io::err::ok)
                        {
                            if (!ec)
                            {
                                *prom.data() = socket(buf_cap, prom.getManager(), std::move(sock));
                                prom.resolve();
                            }
                            else
                            {
                                prom.reject();
                            }
                        }
                        });
                    return _prom;
                }
            private:
                coPromise<tcp::socket> _prom;
                size_t buf_cap;
            };
        };

        namespace udp {
            struct socket : public asio::ip::udp::socket {
                template<typename ...Args>
                inline socket(size_t buffer_capacity, ioManager* mngr, Args&&... args) :
                    asio::ip::udp::socket(asioManager, std::forward<Args>(args)...),
                    _prom_recv(mngr, std::make_tuple(buffer(buffer_capacity), asio::ip::udp::endpoint())),
                    _prom_send(mngr) { }

                template<typename ...Args>
                inline coPromise<> send_io(asio::ip::udp::endpoint addr, Args&&... args)
                {
                    _prom_send.reset();
                    coPromise<> prom = _prom_send;
                    this->async_send_to(asio::buffer(std::forward<Args>(args)...), addr, [prom](const asio::error_code& ec, size_t length) mutable {
                        if (prom.tryOccupy() == io::err::ok)
                        {
                            if (!ec)
                            {
                                prom.resolve();
                            }
                            else
                            {
                                prom.reject();
                            }
                        }
                        });
                    return _prom_send;
                }
                inline coPromise<std::tuple<buffer, asio::ip::udp::endpoint>> recv_io()
                {
                    _prom_recv.reset();
                    buffer* data = &std::get<0>(*_prom_recv.data());
                    asio::ip::udp::endpoint* addr = &std::get<1>(*_prom_recv.data());
                    coPromise<std::tuple<buffer, asio::ip::udp::endpoint>> prom = _prom_recv;
                    this->async_receive_from(asio::buffer((void*)data->push_ptr(), data->remain()), *addr, 
                        [prom](const asio::error_code& ec, size_t n) mutable {
                        if (prom.tryOccupy() == io::err::ok)
                        {
                            if (!ec)
                            {
                                std::get<0>(*prom.data()).push_size(n);
                                prom.resolve();
                            }
                            else
                            {
                                prom.reject();
                            }
                        }
                        }
                    );
                    return _prom_recv;
                }
            private:
                coPromise<std::tuple<buffer, asio::ip::udp::endpoint>> _prom_recv;
                coPromise<> _prom_send;
            };
        };

        namespace dns {
#include "protocol/dns.h"
        };

        namespace http {
#include "protocol/http.h"
        };

        // software simulated protocol, the same as below
        namespace uart{};

        namespace i2c{};

        namespace spi{};

        namespace can{};

        namespace usb{};

#include "internal/definitions.h"
        }
}