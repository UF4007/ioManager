/* C++20 coroutine scheduler, protocol, RPC lib.
 * ------Head-Only------
 * 
 * A clear data protocol stream processing solution is provided.
 * 
 * using asio for network support.
 * 
 * C++ standard: 20 or higher
 * 
 * Licensed under the MIT License.
 * Looking forward to visiting https://github.com/UF4007/ to propose issues, pull your protocol driver, and make io::manager stronger and more universal.
*/
#pragma once
#include "internal/config.h"
#include "internal/includes.h"
namespace io
{
    //headonly version distinguish, prevent the linker from mixing differental versions when multi-reference.
    inline namespace v247a {

#include "internal/forwardDeclarations.h"
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

        //Generic memory pool for single type category. 
        // Not Thread safe.
        template <typename T>
        struct hive {
            explicit inline hive(size_t vacancy_limit = 100) :size_limit(vacancy_limit) {}
            inline ~hive() {
                clean_vacancy();
            }
            template<typename ...Args>
            inline T* emplace(Args&&... args) {
                if constexpr (debug)
                {
                    return new T(std::forward<Args>(args)...);
                }
                if (freed.empty())
                    return new T(std::forward<Args>(args)...);
                else
                {
                    T* ptr = freed.top();
                    freed.pop();
                    return std::launder(new(ptr)T(std::forward<Args>(args)...));
                }
            }
            inline void erase(T* ptr) {
                if constexpr (debug)
                {
                    delete ptr;
                    return;
                }
                if constexpr (std::is_destructible_v<T>)
                    ptr->~T();
                freed.push(ptr);
                if (freed.size() > size_limit)
                {
                    size_t num_to_del = (size_t)(size_limit * erase_multiple);
                    while (num_to_del)
                    {
                        T* ptr = freed.top();
                        freed.pop();
                        ::operator delete(ptr);
                        num_to_del--;
                    }
                }
            }
            inline size_t size() { return freed.size(); }
            inline void clean_vacancy() {
                while (!freed.empty()) {
                    T* ptr = freed.top();
                    freed.pop();
                    ::operator delete(ptr);
                }
            }
            hive(const hive&) = delete;
            hive& operator=(const hive&) = delete;
            inline hive(hive&& other) noexcept
                : freed(std::move(other.freed))
                , size_limit(other.size_limit) {
            }
            inline hive& operator=(hive&& other) noexcept {
                if (this != &other) {
                    clean_vacancy();
                    freed = std::move(other.freed);
                    size_limit = other.size_limit;
                }
                return *this;
            }
        private:
            std::stack<T*> freed;
            const size_t size_limit;
            static constexpr double erase_multiple = 0.5;  // in (0,1) zone
            static constexpr bool debug = false;
        };

        //FIFO buffer. Multi-thread outbound, multi-thread inbound safety. Reentrancy safe. 
        // UB: outbound and inbound in diff threads simultaneously.
        // Its size will increase solely directionally until the buffer capacity has been fully occupied. Use clear to reset it.
        template <typename T, size_t capacity>
        struct forward_fifo {
            //forward constructor of std::array
            constexpr forward_fifo(std::initializer_list<T> init_list) {
                std::copy(init_list.begin(), init_list.end(), _data.begin());
            }
            //inbound function.
            // returns null when fifo full
            // if size larger than remaining size, returns remaining size.
            inline friend std::span<T> operator <<(forward_fifo& buffer, size_t size) {
                size_t start = buffer.produced.fetch_add(size);
                size_t end = start + size;
                if (start >= capacity) {
                    return {};
                }
                size_t available_size = capacity - start;
                size_t actual_size = (end <= capacity) ? size : available_size;
                return { buffer._data.data() + start, actual_size };
            }
            //outbound function
            // if size larger than the read-valid size, returns valid size.
            inline friend std::span<T> operator >>(forward_fifo& buffer, size_t size) {
                size_t start = buffer.consumed.fetch_add(size);
                size_t end = start + size;
                if (start >= capacity) {
                    return {};
                }
                size_t available_size = capacity - start;
                size_t actual_size = (end <= capacity) ? size : available_size;
                return { buffer._data.data() + start, actual_size };
            }
            inline void clear() { consumed = produced = 0; }
        private:
            std::array<T, capacity> _data;
            std::atomic<size_t> produced = 0;
            std::atomic<size_t> consumed = 0;
        };

        //dual buffer. Thread safe. Reentrancy safe. We don't need a ring buffer to handle multi-thread scenario
        // Lock free structure.
        // When outbound_get returns not-nullptr, and the caller empties the obtained buffer, a rotate operation is recommended.
        template <typename _Struc>
        class dualbuf {
            _Struc buffer[2];
            std::atomic_flag bufferLock[2] = { ATOMIC_FLAG_INIT, ATOMIC_FLAG_INIT };
            std::atomic<bool> rotateLock{ false };

            //dualbuf(const dualbuf&) = delete;     //not need to ban
            //const dualbuf& operator=(const dualbuf&) = delete;

        public:
            template <typename... Args>
            inline dualbuf(Args&&... args)
                : buffer{ _Struc(std::forward<Args>(args)...), _Struc(std::forward<Args>(args)...) } {};
            inline dualbuf() {};
            [[nodiscard]] inline _Struc* outbound_get()
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
                    rotateLock.store(true, std::memory_order_seq_cst);
            }

            //inbound_get cannot occupy the pointer for a long time, otherwise, it will block the reading(outbound_get)
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

        //ring buffer.
        // Not Thread safe.
        // UB: Visit a std::span that was got by the previous operator call after another operator call.
        template<typename T, size_t N>
        struct ringbuf {
            //inbound function. get a continuous memory span.
            // returns null when full.
            // if the size is larger than the remaining size, return the remaining size.
            inline friend std::span<T> operator<<(ringbuf& buf, size_t size) {
                if (buf.is_full || size == 0) {
                    return std::span<T>();
                }

                size_t avail_contiguous = 0;

                if (buf.write_pos > buf.read_pos) {
                    //split into 2 blocks. needs to call again next turn.
                    avail_contiguous = N - buf.write_pos;
                }
                else if (buf.write_pos == buf.read_pos) {
                    avail_contiguous = N - buf.write_pos;
                }
                else {
                    //continuous memory.
                    avail_contiguous = buf.read_pos - buf.write_pos;
                }

                size_t len = std::min(size, avail_contiguous);

                std::span<T> result(buf.buffer.data() + buf.write_pos, len);
                buf.write_pos += len;
                if (buf.write_pos >= N) {
                    buf.write_pos = 0;
                }

                buf.is_full = (buf.write_pos == buf.read_pos);

                return result;
            }

            //outbound function. get a continuous memory span.
            // returns null when vacancy.
            // if the size is larger than the read-valid size, return the valid size.
            inline friend std::span<T> operator>>(ringbuf& buf, size_t size) {
                if ((buf.write_pos == buf.read_pos && !buf.is_full) || size == 0) {
                    return std::span<T>();
                }

                size_t avail_contiguous = 0;

                if (buf.read_pos > buf.write_pos) {
                    //split into 2 blocks. needs to call again next turn.
                    avail_contiguous = N - buf.read_pos;
                }
                else if (buf.read_pos == buf.write_pos && buf.is_full) {
                    avail_contiguous = N - buf.read_pos;
                }
                else {
                    //continuous memory.
                    avail_contiguous = buf.write_pos - buf.read_pos;
                }

                size_t len = std::min(size, avail_contiguous);

                std::span<T> result(buf.buffer.data() + buf.read_pos, len);
                buf.read_pos += len;
                if (buf.read_pos >= N) {
                    buf.read_pos = 0;
                }

                buf.is_full = false;

                return result;
            }
            //forward constructor of std::array
            constexpr ringbuf(std::initializer_list<T> init_list) {
                std::copy(init_list.begin(), init_list.end(), buffer.begin());
            }
        private:
            std::array<T, N> buffer;
            size_t write_pos = 0;
            size_t read_pos = 0;
            bool is_full = false;
        };

#include "internal/lowlevel.h"

        //simple awaitable type for finite-state machine
        // UB: resume a null or a destroyed coroutine. This awaitable doesn't ensure the coroutine is valid.
        // Deconstruct a valid awaitable will lead to the leak of resources. Beware.
        struct awaitable {
            __IO_INTERNAL_HEADER_PERMISSION
            inline operator bool() {
                return coro.operator bool();
            }
            inline void resume() {
                coro.resume();
            }
            inline ~awaitable() {
                assert(coro == nullptr || !"Awaitable ERROR: resource leak detected.");
            }
            inline awaitable() {}
            awaitable(const awaitable&) = delete;
            awaitable& operator =(const awaitable&) = delete;
            awaitable(awaitable&&) = default;
            inline awaitable& operator =(awaitable&& right) noexcept {
                coro = right.coro;
                right.coro = nullptr;
                return *this;
            }
        private:
            std::coroutine_handle<> coro = nullptr;
        };

        template<typename T>
        concept FutureConvertible =
            std::is_convertible_v<T&, io::future&> ||
            std::is_convertible_v<T&&, io::future&&>;

        template<typename... Args>
        concept MultipleFuturesConvertible =
            (FutureConvertible<Args> && ...) &&
            sizeof...(Args) >= 2;

        //awaitable future type
        // Not Thread safe.
        // UB: The lifetime of memory binding by the future-promise pair is shorter than the io::future
        // future cannot being co_await by more than one coroutine.
        struct future {
            __IO_INTERNAL_HEADER_PERMISSION;
            template<typename... Args>
                requires MultipleFuturesConvertible<Args...>
            static inline lowlevel::all<Args...> all(Args&&... args) {
                return lowlevel::all<Args...>(std::forward<Args>(args)...);
            }
            template<typename... Args>
                requires MultipleFuturesConvertible<Args...>
            static inline lowlevel::any<Args...> any(Args&&... args) {
                return lowlevel::any<Args...>(std::forward<Args>(args)...);
            }
            template<typename... Args>
                requires MultipleFuturesConvertible<Args...>
            static inline lowlevel::race<Args...> race(Args&&... args) {
                return lowlevel::race<Args...>(std::forward<Args>(args)...);
            }
            template<typename... Args>
                requires MultipleFuturesConvertible<Args...>
            static inline lowlevel::allSettle<Args...> allSettle(Args&&... args) {
                return lowlevel::allSettle<Args...>(std::forward<Args>(args)...);
            }
            inline future() {}
            future(const future&) = delete;
            future& operator=(const future&) = delete;
            inline future(future&& right) noexcept :awaiter(right.awaiter) {
                right.awaiter = nullptr;
            }
            inline future& operator=(future&& right) noexcept {
                decons();
                this->awaiter = right.awaiter;
                right.awaiter = nullptr;
                return *this;
            }
            inline ~future() noexcept {
                decons();
            }
            inline std::error_code getErr(){
                return awaiter->no_tm.err;
            }
            inline bool isSet() {
                return awaiter->bit_set & awaiter->set_lock;
            }
            enum class status_t{
                null = 0,
                idle = 1,
                pedning = 2,
                fullfilled = 3,
                rejected = 4
            };
            inline status_t status() {
                if (awaiter)
                {
                    if (isSet())
                    {
                        if (getErr())
                            return status_t::rejected;
                        return status_t::fullfilled;
                    }
                    else if (awaiter->coro)
                    {
                        return status_t::pedning;
                    }
                    return status_t::idle;
                }
                return status_t::null;
            }
            inline void invalidate() {
                decons();
                awaiter = nullptr;
            }
            promise<void> getPromise();
        private:
            future(clock&&) = delete;
            lowlevel::awaiter* awaiter = nullptr;
            void decons() noexcept;
        };

        //future with data.
        template<typename T>
        struct future_with : future {
            T data;
            inline future_with() {}
            template <typename ...Args>
            inline future_with(Args... args) :T(std::forward<Args>(args)...) {}
            future_with(const future_with&) = delete;
            future_with& operator =(const future_with&) = delete;
            future_with(future_with&&) = delete;
            future_with& operator =(future_with&&) = delete;
            promise<T> getPromise();
        };

        //awaitable promise type
        // Not Thread safe.
        template <typename T = void>
        struct promise : lowlevel::promise_base {
            __IO_INTERNAL_HEADER_PERMISSION
            inline promise(promise &&right) noexcept : lowlevel::promise_base(static_cast<lowlevel::promise_base &&>(right)), ptr(right.ptr) {
                right.ptr = nullptr;
                right.awaiter = nullptr;
            }
            inline promise& operator=(promise&& right) noexcept {
                this->decons();
                this->ptr = right.ptr;
                this->awaiter = static_cast<lowlevel::promise_base&&>(right).awaiter;
                right.ptr = nullptr;
                right.awaiter = nullptr;
                return *this;
            }
            inline promise() {}
            // the pointer got from function resolve() will be invalid next co_await.
            //  gets nullptr when the promise is invalid.
            inline T* resolve() {
                if (static_cast<lowlevel::promise_base*>(this)->resolve())
                    return ptr;
                else 
                    return nullptr;
            }
            inline T* resolve_later() {
                if (static_cast<lowlevel::promise_base*>(this)->resolve_later())
                    return ptr;
                else
                    return nullptr;
            }
            inline T* data() {
                if (valid())
                    return ptr;
                else
                    return nullptr;
            }
        private:
            inline promise(lowlevel::awaiter* a, T* p) noexcept :lowlevel::promise_base(a), ptr(p) {}
            T* ptr;
        };

        template <>
        struct promise<void> : lowlevel::promise_base { };

        //Clock. One-shot clock. Treated as reject by default.
        struct clock: public future{
            __IO_INTERNAL_HEADER_PERMISSION
            inline bool isSet() { return static_cast<future*>(this)->isSet(); }
            inline ~clock() noexcept {
                decons();
            }
            inline clock& operator=(clock&& right) noexcept {
                decons();
                static_cast<future*>(this)->operator=(static_cast<future&&>(right));
                return *this;
            }
            clock(clock&&) = default;
            inline clock() {}
            bool set();
            bool set_later();
        private:
            void decons() noexcept;
        };

        // Repeatly Timer and Counter
        // Not thread safe
        //struct timer
        //{
        //    enum mode_t
        //    {
        //        still_mode = 0,
        //        counter,
        //        up_timer,               //use normal future
        //        down_timer,
        //        down_timer_compensated  //use clock
        //    };
        //    inline timer(
        //        mode_t mode,
        //        std::chrono::steady_clock::duration interval,
        //        size_t _stop_count = 0,
        //        size_t _count = 0) {
        //        assert(mode != still_mode || !"timer ERROR: invaild construct mode");
        //        this->reset(mode, interval, _stop_count, _count);
        //    }
        //    inline void reset(
        //        mode_t mode = still_mode,
        //        std::chrono::steady_clock::duration interval = std::chrono::seconds(0),
        //        size_t _stop_count = 0,
        //        size_t _count = 0);
        //    inline mode_t getMode() { return _mode; }
        //    // Use only for a count-up timer. Count and calculate the duration between the previous lap() time point and now.
        //    template <typename T_fsm>
        //    inline std::chrono::steady_clock::duration lap(fsm<T_fsm>& _fsm) {
        //        assert(mode == up_timer || !"timer ERROR: invaild construct mode");
        //        auto now = std::chrono::steady_clock::now();
        //        auto time = now - _previous;
        //        _previous = now;
        //        count(_fsm);
        //        return time;
        //    }
        //    template <typename T_fsm>
        //    inline bool count(fsm<T_fsm>& _fsm) {
        //        if (count_num <= stop_count)
        //        {
        //            count_num++;
        //            if (this->_mode == counter ||
        //                this->_mode == up_timer)
        //                when_count_future2.getPromise().resolve();
        //            else
        //            {
        //                when_count_future.set();
        //                _fsm.make_clock(when_count_future)
        //            }
        //            if (count_num == stop_count)
        //            {
        //                when_stop_future.getPromise().resolve();
        //            }
        //        }
        //        else
        //        {
        //            return false;
        //        }
        //        return true;
        //    }
        //    template <typename T_fsm>
        //    inline void start(fsm<T_fsm>& _fsm, bool isResolve = false) {
        //        if (this->_mode == counter ||
        //            this->_mode == up_timer)
        //            _fsm.make_future(when_count_future2);
        //        else
        //        {
        //            assert(this->_interval != std::chrono::seconds(0) || !"reset ERROR: invalid null interval");
        //            _fsm.make_clock(when_count_future, this->_interval);
        //        }
        //        _fsm.make_future(when_stop_future);
        //    }
        //    inline void stop() {
        //        when_stop_future.getPromise().resolve();
        //    }
        //    inline io::future& when_count() {
        //        if (this->_mode == counter ||
        //            this->_mode == up_timer)
        //            return when_count_future2;
        //        else
        //            return when_count_future;
        //    }
        //    inline io::future& when_stop() {
        //        return when_stop_future;
        //    }
        //
        //    size_t count_num;
        //    size_t stop_count;
        //private:
        //    mode_t _mode;
        //    std::chrono::steady_clock::time_point _previous;
        //    std::chrono::steady_clock::duration _interval;
        //    clock when_count_future;
        //    future when_count_future2;
        //    future when_stop_future;
        //};

        //awaitable future receiver type
        // Not Thread safe.
        // UB: submit async_promise to another thread, and getErr before co_await.
        struct async_future : future {};

        //awaitable promise sender type
        // Thread safe.
        struct async_promise {
            __IO_INTERNAL_HEADER_PERMISSION
                async_promise(const async_promise&) = delete;
            async_promise& operator=(const async_promise&) = delete;
            inline async_promise(async_promise&& right) noexcept :awaiter(right.awaiter.exchange(nullptr)) {}
            inline async_promise& operator=(async_promise&& right) noexcept {
                decons(right.awaiter.exchange(nullptr));
                return *this;
            }
            inline async_promise() {}
            inline bool resolve();
            inline bool reject(std::error_code ec);
            inline ~async_promise() {
                decons();
            }
        private:
            std::atomic<lowlevel::awaiter*> awaiter = nullptr;
            void decons(lowlevel::awaiter* exchange_ptr = nullptr) noexcept;
        };

        //single manager(thread) internal channel
        // Not Thread safe.
        // UB: Visit a std::span that was got by the previous operator>>() after operator<<() or co_await.
        // When co_await the future of get_span() (alias operator>>()), the awaiter must be triggered when the future turns to resolve immediately. Otherwise, the span got by the future will be invalid.
        template <typename T>
            requires std::is_move_constructible_v<T>
        struct chan {
            __IO_INTERNAL_HEADER_PERMISSION
            //the lifitime of span_guard must be longer than chan.
            struct span_guard{
                friend chan;
                inline void commit() {
                    if (channel)
                    {
                        channel->get_commit();
                        channel = nullptr;
                    }
                }
                inline ~span_guard() {
                    commit();
                }
                std::span<T> span;
                inline span_guard() {}
                span_guard(span_guard&) = delete;
                void operator =(span_guard&) = delete;
                void operator =(span_guard&&) = delete;
            private:
                chan<T> *channel = nullptr;
            };
            inline future operator>>(span_guard& data_out) {
                return get_span(data_out);
            }
            inline future operator<<(std::span<T>& data_in) {
                return put_in(data_in);
            }
            //Future will not resolve until the span gets enough data.
            inline void get_and_copy(std::span<T> where_copy, future_with<std::span<T>>& ret){
                chan_base* base = this->getPtr();
                if (isClosed())
                {
                    getClosedFuture2(base, ret);
                    return;
                }
                ret.data = where_copy;
                if (base->status != chan_base::status_t::recv_block)
                {
                    //find in buffer
                    std::span<T> span_buf = base->get_out(ret.data.size());
                    if (span_buf.size() != 0 || base->capacity == 0)
                    {
                        std::copy(
                            std::make_move_iterator(span_buf.begin()),
                            std::make_move_iterator(span_buf.end()),
                            ret.data.begin()
                        );
                        //another side of ring buffer
                        ret.data = ret.data.subspan(span_buf.size());
                        if (ret.data.size() != 0)
                        {
                            std::span<T> span_buf = base->get_out(ret.data.size());
                            if (span_buf.size() != 0)
                            {
                                std::copy(
                                    std::make_move_iterator(span_buf.begin()),
                                    std::make_move_iterator(span_buf.end()),
                                    ret.data.begin()
                                );
                                ret.data = ret.data.subspan(span_buf.size());
                            }
                        }
                        //if send is blocking, move the waiting coroutine data into the copy place first.
                        if (ret.data.size() != 0 && base->status == chan_base::status_t::send_block)
                        {
                            while (base->waiting.size())
                            {
                                auto& [prom, is_copy] = *(base->waiting.begin());
                                if (prom.valid() == false)
                                {
                                    base->waiting.erase(base->waiting.begin());
                                    continue;
                                }
                                else
                                {
                                    std::span<T>* span = prom.data();
                                    if (ret.data.size() <= span->size())
                                    {
                                        std::copy(
                                            std::make_move_iterator(span->begin()),
                                            std::make_move_iterator(span->begin() + ret.data.size()),
                                            ret.data.begin()
                                        );
                                        *span = span->subspan(ret.data.size());
                                        ret.data = {};
                                        //out span is full. if send is blocking still, move the waiting coroutine data into the buffer then.
                                        break;
                                    }
                                    else
                                    {
                                        std::copy(
                                            std::make_move_iterator(span->begin()),
                                            std::make_move_iterator(span->end()),
                                            ret.data.begin()
                                        );
                                        ret.data = ret.data.subspan(span->size());
                                        prom.resolve_later();
                                        base->waiting.erase(base->waiting.begin());
                                    }
                                }
                            }
                        }
                        //move the waiting coroutine data into the buffer then.
                        while (base->waiting.size())
                        {
                            auto& [prom, is_copy] = *(base->waiting.begin());
                            if (prom.valid() == false)
                            {
                                base->waiting.erase(base->waiting.begin());
                                continue;
                            }
                            else
                            {
                                std::span<T>* span = prom.data();
                                span_buf = base->get_in(span->size());
                                if (span_buf.size() == span->size())
                                {
                                    std::copy(
                                        std::make_move_iterator(span->begin()),
                                        std::make_move_iterator(span->begin() + span_buf.size()),
                                        span_buf.begin()
                                    );
                                    prom.resolve_later();
                                    base->waiting.erase(base->waiting.begin());
                                }
                                else // buf will be full after move in.
                                {
                                    std::copy(
                                        std::make_move_iterator(span->begin()),
                                        std::make_move_iterator(span->begin() + span_buf.size()),
                                        span_buf.begin()
                                    );
                                    *span = span->subspan(span_buf.size());
                                    return getResolvedFuture2(base, ret);
                                }
                            }
                        }
                        base->status = chan_base::status_t::normal;
                        if (ret.data.size() == 0)
                            return getResolvedFuture2(base, ret);
                    }
                }
                else
                {
                    base->erase_invalid();
                }

                //block wait recv
                base->status = chan_base::status_t::recv_block;
                promise<std::span<T>> prom = base->mngr->make_future(ret, &ret.data);
                base->waiting.emplace_back(std::move(prom), true);
            }
            //Once the channel gets any data, future resolve immediately.
            inline future get_span(span_guard& data_out) {
                chan_base* base = this->getPtr();
                if (isClosed()) return getClosedFuture(base);
                data_out.commit();
                if (base->status != chan_base::status_t::recv_block)
                {
                    //find in buffer
                    data_out.span = base->get_out(std::numeric_limits<uint64_t>::max());
                    if (data_out.span.size() != 0)
                    {
                        //if send is blocking, move the waiting coroutine data into the buffer later.
                        data_out.channel = this;
                        return getResolvedFuture(base);
                    }
                }
                else
                {
                   base->erase_invalid();
                }

                //block wait recv
                base->status = chan_base::status_t::recv_block;
                future ret;
                promise<std::span<T>> prom = base->mngr->make_future(ret, &data_out.span);
                base->waiting.emplace_back(std::move(prom), false);
                return ret;
            }
            inline future put_in(std::span<T>& data_in) {
                chan_base* base = this->getPtr();
                if (isClosed()) return getClosedFuture(base);
                assert(base->closed == false || !"channel ERROR: channel has been closed!");
                if (base->status != chan_base::status_t::send_block)
                {
                    while (base->waiting.size())
                    {
                        auto& [prom, is_copy] = *(base->waiting.begin());
                        if (prom.valid() == false)
                        {
                            base->waiting.erase(base->waiting.begin());
                            continue;
                        }
                        else
                        {
                            if (is_copy == false)
                            {
                                *prom.data() = data_in;
                                auto prom_copy = std::move(prom);
                                base->waiting.erase(base->waiting.begin());
                                prom_copy.resolve();
                                return getResolvedFuture(base);
                            }
                            std::span<T>* span = prom.data();
                            if (data_in.size() >= span->size())
                            {
                                std::copy(
                                    std::make_move_iterator(data_in.begin()),
                                    std::make_move_iterator(data_in.begin() + span->size()),
                                    span->begin()
                                );
                                data_in = data_in.subspan(span->size());
                                prom.resolve_later();
                                base->waiting.erase(base->waiting.begin());
                            }
                            else // data_in will be depleted after move out.
                            {
                                std::copy(
                                    std::make_move_iterator(data_in.begin()),
                                    std::make_move_iterator(data_in.end()),
                                    span->begin()
                                );
                                *span = span->subspan(data_in.size());
                                return getResolvedFuture(base);
                            }
                        }
                    }
                    base->status = chan_base::status_t::normal;
                    if (data_in.size() == 0)
                        return getResolvedFuture(base);
                    else
                    {
                        std::span<T> span_buf = base->get_in(data_in.size());
                        if (span_buf.size() != 0)
                        {
                            std::copy(
                                std::make_move_iterator(data_in.begin()),
                                std::make_move_iterator(data_in.begin() + span_buf.size()),
                                span_buf.begin()
                            );
                            //another side of ring buffer
                            data_in = data_in.subspan(span_buf.size());
                            if (data_in.size() != 0)
                            {
                                std::span<T> span_buf = base->get_in(data_in.size());
                                if (span_buf.size() != 0)
                                {
                                    std::copy(
                                        std::make_move_iterator(data_in.begin()),
                                        std::make_move_iterator(data_in.begin() + span_buf.size()),
                                        span_buf.begin()
                                    );
                                    data_in = data_in.subspan(span_buf.size());
                                    if (data_in.size() == 0)
                                        return getResolvedFuture(base);
                                }
                            }
                            else
                            {
                                return getResolvedFuture(base);
                            }
                        }
                    }
                }
                else
                {
                    base->erase_invalid();
                }

                //block wait send
                base->status = chan_base::status_t::send_block;
                future ret;
                promise<std::span<T>> prom = base->mngr->make_future(ret, &data_in);
                base->waiting.emplace_back(std::move(prom), false );
                return ret;
            }
            inline bool isClosed() {
                return this->getPtr()->closed;
            }
            inline bool isFull() {
                return this->getPtr()->is_full;
            }
            inline size_t size() {
                auto base = this->getPtr();
                if (base->is_full)
                    return base->capacity;
                if (base->write_pos >= base->read_pos)
                    return base->write_pos - base->read_pos;
                else
                    return (base->capacity - (base->write_pos - base->read_pos));
            }
            inline size_t capacity() {
                return this->getPtr()->capacity;
            }
            //close, this will deconstruct all element in buffer, and resume all coroutines.
            inline void close() {
                return this->getPtr()->close();
            }
            struct err_category : public std::error_category {
                inline const char* name() const noexcept override {
                    return "channel error";
                }
                inline std::string message(int ev) const override {
                    switch (ev) {
                    case 1:
                        return "Error 1: Channel closed!";
                    default:
                        return "Unknown error!";
                    }
                }
                static const std::error_category& global() {
                    thread_local err_category instance;
                    return instance;
                }
            };
        private:
            inline void get_commit() {
                chan_base* base = this->getPtr();
                if (isClosed()) return;
                //if send is blocking, move the waiting coroutine data into the buffer.
                if (base->status == chan_base::status_t::send_block)
                {
                    while (base->waiting.size())
                    {
                        auto& [prom, is_copy] = *(base->waiting.begin());
                        if (prom.valid() == false)
                        {
                            base->waiting.erase(base->waiting.begin());
                            continue;
                        }
                        else
                        {
                            std::span<T>* span = prom.data();
                            std::span<T> span_buf = base->get_in(span->size());
                            if (span_buf.size() == span->size())
                            {
                                std::copy(
                                    std::make_move_iterator(span->begin()),
                                    std::make_move_iterator(span->begin() + span_buf.size()),
                                    span_buf.begin()
                                );
                                prom.resolve_later();
                                base->waiting.erase(base->waiting.begin());
                            }
                            else // buf will be full after move in.
                            {
                                std::copy(
                                    std::make_move_iterator(span->begin()),
                                    std::make_move_iterator(span->begin() + span_buf.size()),
                                    span_buf.begin()
                                );
                                *span = span->subspan(span_buf.size());
                                return;
                            }
                        }
                    }
                    base->status = chan_base::status_t::normal;
                }
            }
            std::shared_ptr<void> _base;    //type erasure ringbuf. we assert that this shared_ptr cannot be null.
            struct chan_base {
                struct waiting_t {
                    promise<std::span<T>> prom;
                    bool is_copy;
                };
                std::deque<waiting_t> waiting;
                manager* mngr;
                enum class status_t :char {
                    normal = 0,
                    send_block = 1,
                    recv_block = 2
                }status;
                bool closed = false;

                bool is_full;
                size_t capacity;
                size_t write_pos = 0;
                size_t read_pos = 0;
                inline void erase_invalid() {
                    if (waiting.size() == 0)
                        return;
                    for (auto iter = waiting.begin(); iter != waiting.end(); iter++)
                    {
                        auto& [prom, is_copy] = *iter;
                        if (prom.valid() == false)
                        {
                            iter = waiting.erase(iter);
                            if (iter == waiting.end())
                                break;
                        }
                    }
                }
                inline std::span<T> get_in(size_t size) {
                    if (this->is_full || size == 0) {
                        return std::span<T>();
                    }

                    size_t avail_contiguous = 0;

                    if (this->write_pos > this->read_pos) {
                        //split into 2 blocks. needs to call again next turn.
                        avail_contiguous = capacity - this->write_pos;
                    }
                    else if (this->write_pos == this->read_pos) {
                        avail_contiguous = capacity - this->write_pos;
                    }
                    else {
                        //continuous memory.
                        avail_contiguous = this->read_pos - this->write_pos;
                    }

                    size_t len = std::min(size, avail_contiguous);

                    std::span<T> result((T*)(this + 1) + this->write_pos, len);
                    this->write_pos += len;
                    if (this->write_pos >= capacity) {
                        this->write_pos = 0;
                    }

                    this->is_full = (this->write_pos == this->read_pos);

                    return result;
                }
                inline std::span<T> get_out(size_t size) {
                    if ((this->write_pos == this->read_pos && !this->is_full) || size == 0) {
                        return std::span<T>();
                    }

                    size_t avail_contiguous = 0;

                    if (this->read_pos > this->write_pos) {
                        //split into 2 blocks. needs to call again next turn.
                        avail_contiguous = capacity - this->read_pos;
                    }
                    else if (this->read_pos == this->write_pos && this->is_full) {
                        avail_contiguous = capacity - this->read_pos;
                    }
                    else {
                        //continuous memory.
                        avail_contiguous = this->write_pos - this->read_pos;
                    }

                    size_t len = std::min(size, avail_contiguous);

                    std::span<T> result((T*)(this + 1) + this->read_pos, len);
                    this->read_pos += len;
                    if (this->read_pos >= capacity) {
                        this->read_pos = 0;
                    }

                    this->is_full = false;

                    return result;
                }
                inline void close() {
                    T* buffer = (T*)(this + 1);
                    while (waiting.size())
                    {
                        auto& [prom, is_copy] = *(waiting.begin());
                        prom.reject_later(std::error_code(1, err_category::global()));
                        waiting.erase(waiting.begin());
                    }
                    std::destroy_n(buffer, capacity);
                }
                inline static void deleter(void* ptr) {
                    chan_base* base = (chan_base*)ptr;
                    base->~chan_base();
                    operator delete(ptr);
                }
                inline ~chan_base() {
                    if (this->closed == false)
                        close();
                }
            };
            inline future getResolvedFuture(chan_base* base) {
                future ret;
                promise<> prom = base->mngr->make_future(ret);
                prom.resolve();
                return ret;
            }
            inline future getClosedFuture(chan_base* base) {
                future ret;
                promise<> prom = base->mngr->make_future(ret);
                prom.reject_later(std::error_code(1, err_category::global()));
                return ret;
            }
            inline void getResolvedFuture2(chan_base* base, future_with<std::span<T>>& ret) {
                promise<> prom = base->mngr->make_future(ret);
                prom.resolve();
            }
            inline void getClosedFuture2(chan_base* base, future_with<std::span<T>>& ret) {
                promise<> prom = base->mngr->make_future(ret);
                prom.reject_later(std::error_code(1, err_category::global()));
            }
            inline chan_base* getPtr() { return (chan_base*)_base.get(); }
            inline T* getPtrContent() { return (T*)((chan_base*)_base.get() + 1); }
            //use make_chan
            //forward constructor of ringbuf
            inline chan(size_t size, manager* mngr, std::initializer_list<T> init_list) {
                _base = std::shared_ptr<void>(::operator new(sizeof(chan_base) + size * sizeof(T)), chan_base::deleter);
                assert(init_list.size() <= size || !"make_chan ERROR: Too many initialize args!");
                chan_base* base = getPtr();
                new(base)chan_base();
                base->mngr = mngr;
                if (init_list.size() == 0)
                {
                    base->status = chan_base::status_t::recv_block;
                }
                else if (init_list.size() == size)
                {
                    base->status = chan_base::status_t::send_block;
                }
                else
                {
                    base->status = chan_base::status_t::normal;
                }
                base->is_full = (init_list.size() == size);
                base->capacity = size;
                base->write_pos = init_list.size();
                T* buffer = getPtrContent();
                std::uninitialized_copy(
                    init_list.begin(),
                    init_list.end(),
                    buffer
                );
                std::uninitialized_default_construct_n(
                    buffer + init_list.size(),
                    size - init_list.size()
                );
            }
        };

        // receive only channel
        template <typename T>
            requires std::is_move_constructible_v<T>
        struct chan_r {
            inline future operator>>(chan<T>::span_guard& data_out) { return get_span(data_out); }
            inline void get_and_copy(std::span<T> where_copy, future_with<std::span<T>>& ret){ return c.get_and_copy(where_copy, ret); }
            inline future get_span(chan<T>::span_guard& data_out) { return c.get_span(data_out); }
            inline chan_r(chan<T>& r) : c(r) {}
            inline chan_r(chan<T>&& r) : c(r) {}
        private:
            chan<T> c;
        };

        // send only channel
        template <typename T>
            requires std::is_move_constructible_v<T>
        struct chan_s {
            inline future operator<<(std::span<T>& data_in) { return put_in(data_in); }
            inline future put_in(std::span<T>& data_in) { return c.put_in(data_in); }
            inline chan_s(chan<T>& r) : c(r) {}
            inline chan_s(chan<T>&& r) : c(r) {}
        private:
            chan<T> c;
        };

        //async channel
        // Thread safe.
        template <typename T>
        struct async_chan { };

        // receive only channel
        template <typename T>
        struct async_chan_r { };

        // send only channel
        template <typename T>
        struct async_chan_s { };

        struct get_fsm_t {};
        inline static constexpr get_fsm_t get_fsm;

        template<typename>
        struct is_future_with : std::false_type {};

        template<typename U>
        struct is_future_with<io::future_with<U>> : std::true_type {};

        template<typename T>
        struct is_future { static constexpr bool value = (io::is_future_with<T>::value || std::is_same_v<T, io::future>); };

        // finite-state machine function literally return type
        template <typename T>
            requires (std::is_same_v<T, void> || std::is_default_constructible_v<T>)
        struct fsm_func {
            __IO_INTERNAL_HEADER_PERMISSION;
            struct get_fsm_awaitable;
            struct promise_type {
                inline fsm_func<T> get_return_object() { return { this }; }
                inline get_fsm_awaitable await_transform(get_fsm_t x) {
                    return get_fsm_awaitable{};
                }
                inline lowlevel::awa_awaitable await_transform(awaitable& x) {
                    assert(x.operator bool() == false || !"repeatly co_await in same object!");
                    return { &x.coro, &_fsm.is_awaiting };
                }
                template <typename T_Fut>
                    requires std::is_convertible_v<T_Fut&, io::future&>
                inline lowlevel::awaitable_base<T, lowlevel::selector_status::all, T_Fut> await_transform(T_Fut& x) {
                    assert(static_cast<future&>(x).awaiter->coro == nullptr || !"await ERROR: future is not clean: being co_awaited by another coroutine, or not processed by make_future function.");
                    return lowlevel::awaitable_base<T, lowlevel::selector_status::all, T_Fut>(*this, { static_cast<future&>(x).awaiter });
                }
                template <typename T_Fut>
                    requires std::is_convertible_v<T_Fut&, io::future&>
                inline lowlevel::awaitable_base<T, lowlevel::selector_status::all, T_Fut> await_transform(T_Fut&& x) {
                    assert(static_cast<future&&>(x).awaiter->coro == nullptr || !"await ERROR: future is not clean: being co_awaited by another coroutine, or not processed by make_future function.");
                    static_cast<future&&>(x).awaiter->coro = (std::function<void(lowlevel::awaiter*)>*)1;
                    return lowlevel::awaitable_base<T, lowlevel::selector_status::all, T_Fut>(*this, { static_cast<future&&>(x).awaiter });
                }
                template <typename ...Args>
                inline lowlevel::awaitable_base<T, lowlevel::selector_status::all, Args...> await_transform(lowlevel::all<Args...>&& x) {
                    return lowlevel::awaitable_base<T, lowlevel::selector_status::all, Args...>(*this, x.il);
                }
                template <typename ...Args>
                inline lowlevel::awaitable_base<T, lowlevel::selector_status::any, Args...> await_transform(lowlevel::any<Args...>&& x) {
                    return lowlevel::awaitable_base<T, lowlevel::selector_status::any, Args...>(*this, x.il);
                }
                template <typename ...Args>
                inline lowlevel::awaitable_base<T, lowlevel::selector_status::race, Args...> await_transform(lowlevel::race<Args...>&& x) {
                    return lowlevel::awaitable_base<T, lowlevel::selector_status::race, Args...>(*this, x.il);
                }
                template <typename ...Args>
                inline lowlevel::awaitable_base<T, lowlevel::selector_status::allsettle, Args...> await_transform(lowlevel::allSettle<Args...>&& x) {
                    return lowlevel::awaitable_base<T, lowlevel::selector_status::allsettle, Args...>(*this, x.il);
                }
                inline std::suspend_always initial_suspend() { return {}; }
                inline std::suspend_always final_suspend() noexcept { return {}; }
                inline void unhandled_exception() {
                    std::terminate();
                }
                inline void return_void() {
                    if (_fsm.is_detached)
                    {
                        std::coroutine_handle<promise_type> h;
                        h = h.from_promise(*this);
                        std::coroutine_handle<> erased_handle;
                        erased_handle = h;
                        this->_fsm.mngr->delay_deconstruct.push(erased_handle);
                    }
                    if constexpr (io::is_future<T>::value)
                    {
                        if (_fsm._data.awaiter)
                        {
                            auto awaiter = _fsm._data.awaiter;
                            if ((awaiter->occupy_lock & awaiter->bit_set) == false)
                            {
                                awaiter->bit_set |= awaiter->occupy_lock;
                                awaiter->set();
                            }
                        }
                    }
                }
                fsm<T> _fsm;
            };
            struct get_fsm_awaitable
            {
                inline bool await_ready() noexcept { return false; }
                inline bool await_suspend(std::coroutine_handle<promise_type> h) {
                    handle = h;
                    return false;
                }
                inline fsm<T>& await_resume() noexcept {
                    return handle.promise()._fsm;
                }
                std::coroutine_handle<promise_type> handle;
            };
            fsm_func(const fsm_func&) = delete;
            fsm_func& operator=(const fsm_func&) = delete;
            fsm_func(fsm_func&& right) = default;
        private:
            fsm_func& operator=(fsm_func&& right) = default;
            fsm_func(promise_type* data) :_data(data) {}
            promise_type* _data;
        };

        template <typename T> using fsm_promise = fsm_func<T>::promise_type;

        // promise_type of finite-state machine
        template <typename T>
        struct fsm: lowlevel::fsm_base {
            __IO_INTERNAL_HEADER_PERMISSION;
            inline T* data() { return &_data; }
            inline T* operator->() { return &_data; }
        private:
            fsm() = default;
            T _data;
        };

        //reference of FSM
        // Not Thread safe.
        template <>
        struct fsm<void> : lowlevel::fsm_base {
            __IO_INTERNAL_HEADER_PERMISSION;
        };

        //finite-state machine external handle
        // Not thread safe at all.
        // FSM coroutine will be destroyed when fsm_handle deconstruction without detach.
        // If the coroutine is in the call stack, we cannot destroy it immediately. we will push the coroutine into the destroy queue, and destroy it later.
        template <typename T>
        struct fsm_handle {
            __IO_INTERNAL_HEADER_PERMISSION;
            inline fsm_handle() {}
            inline fsm_handle(fsm_handle<T>&& fsm_) noexcept :_fsm(fsm_._fsm) {
                fsm_._fsm = nullptr;
            }
            fsm_handle(const fsm_handle&) = delete;
            fsm_handle& operator=(const fsm_handle&) = delete;
            inline fsm_handle& operator=(fsm_handle&& right) noexcept {
                decons();
                this->_fsm = right._fsm;
                right._fsm = nullptr;
                return *this;
            }
            inline void detach() { 
                _fsm->_fsm.is_detached = true;
                _fsm = nullptr;
            }
            inline bool done() {
                if (_fsm)
                {
                    std::coroutine_handle<fsm_promise<T>> h;
                    h = h.from_promise(*_fsm);
                    return h.done();
                }
                return true;
            }
            inline ~fsm_handle() {
                decons();
            }
            // UB: Try get data when a fsm had been detached.
            inline T* data() requires (!std::is_same_v<T, void>) { return &_fsm->_fsm._data; }
            inline T* operator->() requires (!std::is_same_v<T, void>) { return &_fsm->_fsm._data; }
            inline void destroy() { this->decons(); _fsm = nullptr; }
            inline operator bool() {
                return _fsm;
            }
        private:
            inline void decons() {
                if (_fsm)
                {
                    std::coroutine_handle<fsm_promise<T>> h;
                    h = h.from_promise(*_fsm);
                    if (_fsm->_fsm.is_awaiting)     //awaiting, not on call chain. deconstruct immediately.
                    {
                        h.destroy();
                    }
                    else                            //on call chain. delay deconstruct.
                    {
                        std::coroutine_handle<> erased_handle;
                        erased_handle = h;
                        _fsm->_fsm.mngr->delay_deconstruct.push(erased_handle);
                        _fsm->_fsm.is_deconstructing = true;
                    }
                }
            }
            inline fsm_handle(fsm_func<T>::promise_type* fsm_) :_fsm(fsm_) {}
            fsm_func<T>::promise_type* _fsm = nullptr;
        };

        //io::future_fsm_func<T> == io::fsm_func<io::future_with<T>>
        // When the coroutine begins, the future will be initialized automately.
        // When co_return, resolve(null) the future.
        template <typename T>
        using future_fsm_func = io::fsm_func<io::future_with<T>>;

        using future_fsm_func_ = io::fsm_func<io::future>;

        template <typename T>
        using future_fsm_handle = io::fsm_handle<io::future_with<T>>;

        using future_fsm_handle_ = io::fsm_handle<io::future>;

        //io::dispatcher<T> == std::deque<io::fsm_handle<T>>
        // a simple dispatcher of coroutines.
        template <typename T>
        struct dispatcher {
            using iterator = std::deque<io::fsm_handle<T>>::iterator;

            inline iterator end() { return _deque.end(); }

            inline iterator begin() { return _deque.begin(); }

            inline iterator erase(iterator iter) { return _deque.erase(iter); }

            inline iterator erase(iterator begin, iterator end) { return _deque.erase(begin, end); }

            inline void insert_before(io::fsm_handle<T>&& handle, iterator iter) {
                _deque.insert(iter, std::move(handle));
            }

            inline void insert(io::fsm_handle<T>&& handle) {
                _deque.emplace_back(std::move(handle));
            }

            inline void clear() {
                return _deque.clear();
            }

            inline void remove_vacancy() {
                auto it = _deque.begin();
                while (it != _deque.end()) {
                    if (it->done()) {
                        it = _deque.erase(it);
                        continue;
                    }
                };
            }

            inline size_t size() {
                return _deque.size();
            }

            template <typename U>
            inline iterator find(const U& key) requires requires (const T& a, const U& b) { { a == b } -> std::convertible_to<bool>; } {
                iterator result = this->end();

                auto it = _deque.begin();
                while (it != _deque.end()) {
                    if (it->done()) {
                        it = _deque.erase(it);
                        continue;
                    }

                    if (result == this->end() && key == *it->data()) {
                        result = it;
                    }
                    ++it;
                }

                return result;
            }

        private:
            std::deque<io::fsm_handle<T>> _deque;
        };

        //scheduler, executor
        // 1 manager == 1 thread
        struct manager {
            __IO_INTERNAL_HEADER_PERMISSION;
            inline manager() {}
            inline void drive(){
                //pending task
                while (1)
                {
                    while (spinLock_pd.test_and_set(std::memory_order_acquire));
                    if (pendingTask.size())
                    {
                        auto i = pendingTask.front();
                        pendingTask.pop();
                        spinLock_pd.clear(std::memory_order_release);
                        i.resume();
                        continue;
                    }
                    spinLock_pd.clear(std::memory_order_release);
                    break;
                }

                //async queueing awaiter to resolve
                while (resolve_queue.lock.test_and_set(std::memory_order_acquire));
                lowlevel::awaiter* readys = resolve_queue.queue;
                resolve_queue.queue = nullptr;
                resolve_queue.lock.clear(std::memory_order_release);
                while (1)
                {
                    if (readys != nullptr)
                    {
                        auto next = readys->no_tm.queue_next;
                        io::promise<void> prom(readys);
                        prom.resolve();
                        readys = next;
                        continue;
                    }
                    break;
                }

                //async queueing awaiter to reject
                while (reject_queue.lock.test_and_set(std::memory_order_acquire));
                readys = reject_queue.queue;
                reject_queue.queue = nullptr;
                reject_queue.lock.clear(std::memory_order_release);
                while (1)
                {
                    if (readys != nullptr)
                    {
                        auto next = readys->no_tm.queue_next;
                        io::promise<void> prom(readys);
                        prom.resolve();
                        readys = next;
                        continue;
                    }
                    break;
                }

                //async queueing awaiter to deconstruct
                while (prom_decons_queue.lock.test_and_set(std::memory_order_acquire));
                readys = prom_decons_queue.queue;
                prom_decons_queue.queue = nullptr;
                prom_decons_queue.lock.clear(std::memory_order_release);
                while (1)
                {
                    if (readys != nullptr)
                    {
                        auto next = readys->no_tm.queue_next;
                        io::promise<void> prom(readys);
                        readys = next;
                        continue;
                    }
                    break;
                }

                //local queueing awaiter to resolve
                readys = resolve_queue_local;
                resolve_queue_local = nullptr;
                while (1)
                {
                    if (readys != nullptr)
                    {
                        auto next = readys->no_tm.queue_next;
                        io::promise<void> prom(readys);
                        prom.resolve();
                        readys = next;
                        continue;
                    }
                    break;
                }

                //clocks
                std::chrono::steady_clock::time_point suspend_next;
                auto suspend_max_p = suspend_max + std::chrono::steady_clock::now();
                while (1)
                {
                    if (auto iter = time_chain.begin(); iter != time_chain.end())
                    {
                        const auto& now = std::chrono::steady_clock::now();
                        const auto& [tm, a] = *iter;
                        if (tm <= now)
                        {
                            lowlevel::awaiter* awa = const_cast<lowlevel::awaiter*>(a);
                            promise<void> prom(awa);
                            time_chain.erase(iter);
                            prom.resolve();
                            continue;
                        }
                        else
                        {
                            suspend_next = tm;
                            if (suspend_next >= suspend_max_p)
                            {
                                suspend_next = suspend_max_p;
                            }
                        }
                    }
                    else
                    {
                        suspend_next = suspend_max_p;
                    }
                    break;
                }

                //deconstructed fsm
                while (delay_deconstruct.size())
                {
                    auto i = delay_deconstruct.front();
                    delay_deconstruct.pop();
                    i.destroy();
                }

                //suspend
                bool _nodiscard = suspend_sem.try_acquire_until(suspend_next);
            }
            // async co_spawn, coroutine will be run by the caller manager next turn.
            template <typename T_spawn>
            [[nodiscard]] inline fsm_handle<T_spawn> spawn_later(fsm_func<T_spawn> new_fsm) {
                new_fsm._data->_fsm.mngr = this;

                while (spinLock_pd.test_and_set(std::memory_order_acquire));
                std::coroutine_handle<fsm_promise<T_spawn>> h;
                h = h.from_promise(*new_fsm._data);
                std::coroutine_handle<> erased_handle;
                erased_handle = h;
                pendingTask.push(erased_handle);
                spinLock_pd.clear(std::memory_order_release);
                this->suspend_sem.release();

                if constexpr (io::is_future_with<T_spawn>::value)
                {
                    this->make_future(new_fsm._data->_fsm._data, &new_fsm._data->_fsm._data.data);
                }
                else if constexpr (io::is_future<T_spawn>::value)
                {
                    this->make_future(new_fsm._data->_fsm._data);
                }

                return { new_fsm._data };
            }
            template <typename T_Prom>
            inline promise<T_Prom> make_future(future& fut, T_Prom* mem_bind)
            {
                FutureVaild(fut);
                return { fut.awaiter ,mem_bind };
            }
            inline promise<void> make_future(future& fut)
            {
                FutureVaild(fut);
                return { fut.awaiter };
            }
            // make a clock
            template <typename T_Duration>
            inline void make_clock(clock& fut, T_Duration duration, bool isResolve = false)
            {
                fut.decons();
                FutureVaild(fut);
                fut.awaiter->bit_set |= fut.awaiter->is_clock;
                if (isResolve)
                    fut.awaiter->bit_set |= fut.awaiter->clock_resolve;
                std::memset(&fut.awaiter->tm, 0, sizeof(fut.awaiter->tm));
                fut.awaiter->tm = this->time_chain.insert(
                    std::make_pair(
                        static_cast<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now() + duration),
                        static_cast<lowlevel::awaiter*>(fut.awaiter)
                    )
                );
            }
            // make async future pair
            inline async_promise make_future(async_future& fut)
            {
                do {
                    if (fut.awaiter != nullptr)
                    {
                        if (fut.awaiter->bit_set & fut.awaiter->promise_handled &&
                            fut.awaiter->coro == nullptr)
                        {
                            fut.awaiter->reset();
                            break;            //reuse of future obj
                        }
                        else
                        {
                            fut.awaiter->bit_set &= ~fut.awaiter->future_handled;
                        }
                    }
                    auto new_awaiter = this->awaiter_hive.emplace();
                    fut.awaiter = new_awaiter;
                    fut.awaiter->mngr = this;
                    break;
                } while (0);
                async_promise ret;
                ret.awaiter = fut.awaiter;
                return ret;
            }
            template <typename T_chan>
            inline chan<T_chan> make_chan(size_t size, std::initializer_list<T_chan> init_list = {}) {
                chan<T_chan> ret(size, this, init_list);
                return ret;
            }
            template <typename T_chan>
            inline async_chan<T_chan> make_async_chan() {}
            manager(const manager&) = delete;
            manager& operator=(const manager&) = delete;
            manager(manager&& right) = delete;
            manager& operator=(manager&& right) = delete;
        private:
            inline bool FutureVaild(future& fut)
            {
                if (fut.awaiter != nullptr)
                {
                    if ((fut.awaiter->bit_set & fut.awaiter->promise_handled) == false &&
                        fut.awaiter->coro == nullptr)
                    {
                        fut.awaiter->reset();
                        return true;            //reuse of future obj
                    }
                    else
                    {
                        fut.awaiter->bit_set &= ~fut.awaiter->future_handled;
                    }
                }
                auto new_awaiter = this->awaiter_hive.emplace();
                fut.awaiter = new_awaiter;
                fut.awaiter->mngr = this;
                return false;
            }
            std::queue<std::coroutine_handle<>> pendingTask; //async queueing to pending task
            std::atomic_flag spinLock_pd = ATOMIC_FLAG_INIT;

            std::multimap<std::chrono::steady_clock::time_point, lowlevel::awaiter*> time_chain;

            lowlevel::awaiter* resolve_queue_local = nullptr;   //local queueing to resolve

            lowlevel::await_queue resolve_queue;      //async queueing to resolve

            lowlevel::await_queue reject_queue;       //async queueing to reject

            lowlevel::await_queue prom_decons_queue;  //async queueing to deconstruct the promise

            std::queue<std::coroutine_handle<>> delay_deconstruct;  //coroutines on call chain and needs deconstruct.

            std::chrono::nanoseconds suspend_max = std::chrono::nanoseconds(400000000);   //defalut 400ms
            std::binary_semaphore suspend_sem = std::binary_semaphore(1);

            io::hive<lowlevel::awaiter> awaiter_hive = io::hive<lowlevel::awaiter>(1000);
        };



        // -------------------------------channel/protocol--------------------------------

        namespace tcp {
            struct socket {
                __IO_INTERNAL_HEADER_PERMISSION;
                inline socket() {}

                template <typename T_FSM>
                inline async_future& wait_read(fsm<T_FSM>& state_machine, size_t size = BUF_SIZE) {
                    auto status = recv_future.status();
                    if (status == io::future::status_t::null ||
                        status == io::future::status_t::fullfilled ||
                        status == io::future::status_t::rejected)
                    {
                        async_promise prom = state_machine.make_future(recv_future);
                        recv_buf->resize(size);
                        asio_sock.async_read_some(asio::buffer(recv_buf->data(), recv_buf->size()),
                                                  [pr = std::move(prom),
                                                   recv_buf = this->recv_buf](const asio::error_code &ec, size_t size) mutable
                                                  {
                                                      recv_buf->resize(size);
                                                      if (ec)
                                                          pr.reject(ec);
                                                      else
                                                          pr.resolve();
                                                  });
                    }
                    return recv_future;
                }

                inline std::span<char> read() {
                    return std::span<char>(*recv_buf.get());
                }

                template <typename T_FSM>
                inline async_future wait_send(fsm<T_FSM>& state_machine, std::span<const char> span) {
                    if (send_buf.use_count() != 1)
                        send_buf = std::make_shared<std::string>();
                    async_future fut;
                    async_promise prom = state_machine.make_future(fut);
                    send_buf->resize(span.size());
                    std::memcpy(send_buf->data(), span.data(), span.size());
                    asio_sock.async_write_some(asio::buffer(*send_buf.get()),
                        [
                            pr = std::move(prom),
                                send_buf = this->send_buf
                        ](const asio::error_code& ec, size_t size)mutable {
                            if (ec)
                                pr.reject(ec);
                            else
                                pr.resolve();
                        });
                            return fut;
                }

                template <typename T_FSM>
                inline async_future connect(fsm<T_FSM>& state_machine,
                    const asio::ip::tcp::endpoint& endpoint) {
                    async_future fut;
                    async_promise prom = state_machine.make_future(fut);
                    asio_sock.async_connect(endpoint,
                        [pr = std::move(prom)](const asio::error_code& ec)mutable {
                            if (ec)
                                pr.reject(ec);
                            else
                                pr.resolve();
                        });
                    return fut;
                }

                inline void close() {
                    asio::error_code ec;
                    asio_sock.close(ec);
                }

                inline auto readGetErr()
                {
                    return recv_future.getErr();
                }

                IO_MANAGER_FORWARD_FUNC(asio_sock, native_handle);
                IO_MANAGER_FORWARD_FUNC(asio_sock, is_open);
                IO_MANAGER_FORWARD_FUNC(asio_sock, remote_endpoint);
                IO_MANAGER_FORWARD_FUNC(asio_sock, local_endpoint);
                IO_MANAGER_FORWARD_FUNC(asio_sock, available);

            private:
                inline socket(asio::ip::tcp::socket&& sock) {
                    asio_sock = std::move(sock);
                    recv_buf = std::make_shared<std::string>();
                    send_buf = std::make_shared<std::string>();
                }
                asio::ip::tcp::socket asio_sock = asio::ip::tcp::socket(lowlevel::asioManager);
                static constexpr size_t BUF_SIZE = 10240;
                std::shared_ptr<std::string> recv_buf = std::make_shared<std::string>();
                std::shared_ptr<std::string> send_buf = std::make_shared<std::string>();
                io::async_future recv_future;
            };

            struct acceptor {
                __IO_INTERNAL_HEADER_PERMISSION;
                inline acceptor(uint16_t port) {
                    asio::error_code ec;
                    asio_accp.open(asio::ip::tcp::v4(), ec);
                    if (!ec) {
                        asio_accp.bind(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port), ec);
                        if (!ec) {
                            asio_accp.listen(asio::socket_base::max_listen_connections, ec);
                        }
                    }
                }
                inline acceptor() {}

                template <typename T_FSM>
                inline async_future wait_accept(fsm<T_FSM>& state_machine) {
                    async_future fut;
                    async_promise prom = state_machine.make_future(fut);
                    asio_accp.async_accept(
                        [pr = std::move(prom),
                        accptr = accepted_ptr
                        ]
                        (const asio::error_code& ec, asio::ip::tcp::socket sock) mutable {
                            if (!ec)
                            {
                                if (accptr->ptr.has_value() == false)
                                {
                                    accptr->ptr.emplace(std::move(sock));
                                    pr.resolve();
                                    return;
                                }
                            }
                            pr.reject(ec);
                        });
                    return fut;
                }

                inline bool accept(socket& sock) {
                    if (accepted_ptr->ptr.has_value())
                    {
                        sock = std::move(accepted_ptr->ptr.value());
                        accepted_ptr->ptr.reset();
                        return true;
                    }
                    return false;
                }

                inline void close() {
                    asio::error_code ec;
                    asio_accp.close(ec);
                }

                IO_MANAGER_FORWARD_FUNC(asio_accp, open);
                IO_MANAGER_FORWARD_FUNC(asio_accp, bind);
                IO_MANAGER_FORWARD_FUNC(asio_accp, listen);
                IO_MANAGER_FORWARD_FUNC(asio_accp, is_open);
                IO_MANAGER_FORWARD_FUNC(asio_accp, local_endpoint);

            private:
                struct shared_struct {
                    std::optional<asio::ip::tcp::socket> ptr;
                };
                std::shared_ptr<shared_struct> accepted_ptr = std::make_shared<shared_struct>();
                asio::ip::tcp::acceptor asio_accp = asio::ip::tcp::acceptor(lowlevel::asioManager);
            };
        };

        namespace udp {
            struct socket {
                __IO_INTERNAL_HEADER_PERMISSION;

                inline socket() {}

                inline socket(uint16_t port) {
                    asio::error_code ec;
                    asio_sock.open(asio::ip::udp::v4(), ec);
                    if (!ec) {
                        asio_sock.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), port), ec);
                    }
                }

                template <typename T_FSM>
                inline async_future& wait_read(fsm<T_FSM>& state_machine, size_t size = BUF_SIZE)
                {
                    auto status = recv_future.status();
                    if (status == io::future::status_t::null ||
                        status == io::future::status_t::fullfilled||
                        status == io::future::status_t::rejected)
                    {
                        async_promise prom = state_machine.make_future(recv_future);

                        if (size > recv_buf->buf.size())
                        {
                            recv_buf->buf.resize(size);
                        }

                        asio_sock.async_receive_from(
                            asio::buffer(recv_buf->buf.data(), size),
                            recv_buf->endpoint,
                            [pr = std::move(prom), recv_buf = this->recv_buf](const asio::error_code &ec, size_t bytes_received) mutable
                            {
                                if (ec)
                                {
                                    pr.reject(ec);
                                }
                                else
                                {
                                    recv_buf->buf.resize(bytes_received);
                                    pr.resolve();
                                }
                            });
                    }
                    return recv_future;
                }

                inline std::tuple<std::span<const char>, asio::ip::udp::endpoint> read() const {
                    return std::make_tuple(std::span<const char>(recv_buf->buf.data(), recv_buf->buf.size()), recv_buf->endpoint);
                }

                template <typename T_FSM>
                inline async_future wait_send(fsm<T_FSM>& state_machine,
                    std::span<const char> data,
                    const asio::ip::udp::endpoint& target) {
                    if (send_buf.use_count() != 1)
                        send_buf = std::make_shared<data_with_endpoint>();
                    async_future fut;
                    async_promise prom = state_machine.make_future(fut);

                    if (data.size() > send_buf->buf.size()) {
                        send_buf->buf.resize(data.size());
                    }

                    send_buf->endpoint = target;

                    std::memcpy(send_buf->buf.data(), data.data(), data.size());

                    asio_sock.async_send_to(
                        asio::buffer(send_buf->buf.data(), data.size()),
                        send_buf->endpoint,
                        [pr = std::move(prom), send_buf = this->send_buf]
                        (const asio::error_code& ec, size_t) mutable {
                            if (ec) {
                                pr.reject(ec);
                            }
                            else {
                                pr.resolve();
                            }
                        });
                    return fut;
                }

                inline void close() {
                    asio::error_code ec;
                    asio_sock.close(ec);
                }

                inline auto readGetErr() {
                    return recv_future.getErr();
                }

                IO_MANAGER_FORWARD_FUNC(asio_sock, open);
                IO_MANAGER_FORWARD_FUNC(asio_sock, bind);
                IO_MANAGER_FORWARD_FUNC(asio_sock, is_open);
                IO_MANAGER_FORWARD_FUNC(asio_sock, local_endpoint);
                IO_MANAGER_FORWARD_FUNC(asio_sock, available);

            private:
                asio::ip::udp::socket asio_sock = asio::ip::udp::socket(lowlevel::asioManager);
                static constexpr size_t BUF_SIZE = 10240;
                struct data_with_endpoint {
                    std::string buf;
                    asio::ip::udp::endpoint endpoint;
                };
                std::shared_ptr<data_with_endpoint> recv_buf = std::make_shared<data_with_endpoint>();
                std::shared_ptr<data_with_endpoint> send_buf = std::make_shared<data_with_endpoint>();
                io::async_future recv_future;
            };
        };

        namespace dns {
#include "protocol/dns.h"
        };

        namespace kcp {
            namespace detail {
#include "protocol/ikcp.c"
            };
            //protocol control block
            struct pcb {
                using lowlevel_output = std::function<int(std::span<const char>)>;
                template<typename T_FSM>
                inline pcb(fsm<T_FSM>& fsm_user) {
                    _kcp = detail::ikcp_create(0, nullptr);
                    _fsm = fsm_user.spawn_now([](detail::ikcpcb* _kcp)->fsm_func<fsm_builtin> {
                        auto& fsm_user = co_await io::get_fsm;
                        io::clock delayer;
                        while (1)
                        {
                            auto now = std::chrono::system_clock::now();
                            auto duration = now.time_since_epoch();
                            uint32_t timestamp = (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
                            detail::ikcp_update(_kcp, timestamp);
                            uint32_t delay_time = detail::ikcp_check(_kcp, timestamp) - timestamp;
                            fsm_user.make_clock(delayer, std::chrono::milliseconds(delay_time));
                            co_await delayer;
                        }
                        }(_kcp));
                }
                inline pcb(pcb&& right) noexcept :_kcp(right._kcp), _fsm(std::move(right._fsm)) {}
                pcb& operator=(pcb&& right) noexcept {
                    if (this != &right) {
                        if (_kcp) {
                            detail::ikcp_release(_kcp);
                        }
                        _kcp = right._kcp;
                        right._kcp = nullptr;
                        _fsm = std::move(right._fsm);
                    }
                    return *this;
                }
                inline ~pcb() {
                    if (_kcp)
                        detail::ikcp_release(_kcp);
                }
                inline int send(const char* buffer, int len) { return detail::ikcp_send(_kcp, buffer, len); }
                template <typename T_FSM>
                inline future& future_recv(fsm<T_FSM>& state_machine) {
                    auto status = recv_future.status();
                    if (status == io::future::status_t::null ||
                        status == io::future::status_t::fullfilled ||
                        status == io::future::status_t::rejected)
                    {
                        state_machine.make_future(recv_future);
                    }
                    return recv_future;
                }
                inline auto readGetErr() { return recv_future.getErr(); }
                inline int recv(char* buffer, int len) { return detail::ikcp_recv(_kcp, buffer, len); }
                inline void setoutput(lowlevel_output output) { return detail::ikcp_setoutput(_kcp, output); }
                inline int input(const char* buffer, long len) { 
                    int ret = detail::ikcp_input(_kcp, buffer, len);
                    if (detail::ikcp_peeksize(_kcp) > 0)
                    {
                        recv_future.getPromise().resolve_later();
                    }
                    return ret;
                }
                inline void input_err(std::error_code ec) { recv_future.getPromise().reject_later(ec); }
                inline void flush() { detail::ikcp_flush(_kcp); }
                inline int peeksize() const { return detail::ikcp_peeksize(_kcp); }
                inline int setmtu(int mtu) { return detail::ikcp_setmtu(_kcp, mtu); }
                inline int wndsize(int sndwnd, int rcvwnd) { return detail::ikcp_wndsize(_kcp, sndwnd, rcvwnd); }
                inline int waitsnd() const { return detail::ikcp_waitsnd(_kcp); }
                inline int nodelay(int nodelay_enable, int interval, int resend, int nc) { return detail::ikcp_nodelay(_kcp, nodelay_enable, interval, resend, nc); }
                template<typename... Args>
                inline void log(int mask, const char* fmt, Args... args) { detail::ikcp_log(_kcp, mask, fmt, args...); }
                IO_MANAGER_BAN_COPY(pcb);
            private:
                struct fsm_builtin {};
                detail::ikcpcb* _kcp = nullptr;
                fsm_handle<fsm_builtin> _fsm;
                future recv_future;
            };
        };

        namespace http {
#include "protocol/http.h"
        };

        namespace rpc {

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