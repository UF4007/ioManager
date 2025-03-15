/* C++20 coroutine scheduler, protocol, RPC lib.
 * ------Head-Only------
 * 
 * Pipeline Concurrency, a clear data protocol stream processing solution is provided.
 * 
 * using asio for network support.
 * 
 * C++ standard: 20 or higher
 * 
 * Licensed under the MIT License.
 * Looking forward to visiting https://github.com/UF4007/ to propose issues, pull your protocol driver, and make io::manager stronger and more universal.
*/
#pragma once
#define IO_LIB_VERSION___ v3
#include "internal/config.h"
#include "internal/includes.h"
namespace io
{
    //headonly version distinguish, prevent the linker from mixing differental versions when multi-reference.
    inline namespace IO_LIB_VERSION___ {

#include "internal/forwardDeclarations.h"

        // -------------------------------basis structure--------------------------------

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



        // -------------------------------coroutine--------------------------------

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
            template <typename U>
            future(future_with<U>&&) = delete;
            template <typename U>
            void operator=(future_with<U>&&) = delete;
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
            requires (!std::is_same_v<T, void>)
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
            inline void resolve() {
                static_cast<lowlevel::promise_base*>(this)->resolve();
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
        namespace timer {
            struct counter {
                size_t stop_count;
                size_t count_sum = 0;
                inline counter(size_t stop_count) :stop_count(stop_count) {}
                inline bool count(size_t count_ = 1) {
                    if (count_sum >= stop_count) {
                        return false;
                    }
                    count_sum += count_;
                    if (count_sum >= stop_count) {
                        _on_stop.getPromise().resolve();
                    }
                    return true;
                }
                inline bool stop() {
                    if (count_sum >= stop_count) {
                        return false;
                    }
                    count_sum = stop_count;
                    //_on_stop.getPromise().resolve();
                    return true;
                }
                inline void reset(size_t count_sum_ = 0) {
                    count_sum = count_sum_;
                }
                inline bool isReach() {
                    return count_sum >= stop_count;
                }
                template <typename T_FSM>
                inline future& onReach(T_FSM& _fsm)
                {
                    _fsm.make_future(_on_stop);
                    return _on_stop;
                }
            private:
                future _on_stop;
            };
            // compensated countdown timer, when it being await, current time will be compared with (start_timepoint + count_sum * count_duration), rather (previous_timepoint + count_duration)
            struct down : private counter {
                inline down(size_t stop_count) : counter(stop_count) {}
                inline void start(std::chrono::steady_clock::duration _duration)
                {
                    this->reset();
                    duration = _duration;
                    return;
                }
                template <typename T_FSM>
                inline clock& await_tm(T_FSM& _fsm)
                {
                    auto target_time = start_tp + duration * (this->count_sum + 1);
                    auto now = std::chrono::steady_clock::now();

                    bool reached = this->count() == false;
                    if (now >= target_time || reached) {
                        _fsm.make_outdated_clock(_clock);
                    }
                    else {
                        auto wait_duration = target_time - now;
                        _fsm.make_clock(_clock, wait_duration);
                    }
                    return _clock;
                }
                inline void reset(size_t count_sum_ = 0)
                {
                    counter::reset(count_sum_);
                    start_tp = std::chrono::steady_clock::now();
                }
                inline std::chrono::steady_clock::duration getDuration() const
                {
                    return duration;
                }
                inline bool isReach() {
                    return ((counter*)this)->isReach();
                }
                template <typename T_FSM>
                inline future& onReach(T_FSM& _fsm)
                {
                    return ((counter*)this)->onReach(_fsm);
                }
            private:
                std::chrono::steady_clock::time_point start_tp;
                std::chrono::steady_clock::duration duration;
                clock _clock;
            };
            // forward timer
            struct up {
                inline void start() { previous_tp = std::chrono::steady_clock::now(); }
                inline std::chrono::steady_clock::duration lap() {
                    auto now = std::chrono::steady_clock::now();
                    auto previous = previous_tp;
                    previous_tp = now;
                    if (previous_tp.time_since_epoch().count() == 0)
                        return std::chrono::steady_clock::duration{ 0 };
                    return now - previous;
                }
                inline void reset() {
                    previous_tp = {};
                }
            private:
                std::chrono::steady_clock::time_point previous_tp;
            };
        };

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
        
        struct yield_t {};
        inline static constexpr yield_t yield;

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
                template<typename U>
                get_fsm_awaitable await_transform(future_with<U>&&) = delete;
                template<typename U>
                void yield_value(U&&) = delete;
                inline get_fsm_awaitable await_transform(get_fsm_t x) {
                    return get_fsm_awaitable{};
                }
                inline lowlevel::awaitable_base<T, lowlevel::selector_status::all, future> await_transform(yield_t) {
                    future fut;
                    _fsm.make_future(fut);
                    fut.awaiter->no_tm.queue_next = _fsm.mngr->resolve_queue_local;
                    _fsm.mngr->resolve_queue_local = fut.awaiter;
                    fut.awaiter->coro = (std::function<void(lowlevel::awaiter*)>*)1;
                    return lowlevel::awaitable_base<T, lowlevel::selector_status::all, future>(*this, { fut.awaiter });
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
            inline typename std::add_lvalue_reference<T>::type operator*() { return _data; }
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
            inline typename std::add_lvalue_reference<T>::type operator*() requires (!std::is_same_v<T, void>) { return _fsm->_fsm._data; }
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
        // When co_return, if the future is pending, try to resolve the future.
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
#if IO_USE_ASIO
                io_ctx.run_until(suspend_next);
                io_ctx.restart();
#else
                bool _nodiscard = suspend_sem.try_acquire_until(suspend_next);
#endif
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
                
                this->suspend_release();

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
            inline void make_outdated_clock(clock& fut, bool isResolve = false)
            {
                fut.decons();
                FutureVaild(fut);
                fut.awaiter->bit_set &= ~fut.awaiter->promise_handled;
                fut.awaiter->bit_set |= fut.awaiter->set_lock;
                fut.awaiter->bit_set |= fut.awaiter->is_clock;
                if (isResolve)
                    fut.awaiter->bit_set |= fut.awaiter->clock_resolve;
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
#if IO_USE_ASIO
            // use run_until in io_context
            asio::io_context io_ctx{1};
            asio::executor_work_guard<asio::io_context::executor_type> work_guard = asio::make_work_guard(io_ctx);
#else
            std::binary_semaphore suspend_sem = std::binary_semaphore(1);
#endif
            inline void suspend_release() {
#if IO_USE_ASIO
                io_ctx.stop();
#else
                this->suspend_sem.release();
#endif
            }

            io::hive<lowlevel::awaiter> awaiter_hive = io::hive<lowlevel::awaiter>(1000);
        };



        // -------------------------------protocol trait/pipeline--------------------------------

        //protocol traits
        namespace trait {
            namespace detail {
                // Check if T has prot_output_type
                template <typename T, typename = void>
                struct has_prot_output_type : std::false_type {};
                
                template <typename T>
                struct has_prot_output_type<T, std::void_t<typename T::prot_output_type>> : std::true_type {};
                
                // Check if T has prot_input_type
                template <typename T, typename = void>
                struct has_prot_input_type : std::false_type {};
                
                template <typename T>
                struct has_prot_input_type<T, std::void_t<typename T::prot_input_type>> : std::true_type {};
                
                // Check if T::prot_output_type is void
                template <typename T, typename = void>
                struct is_prot_recv_void : std::false_type {};
                
                template <typename T>
                struct is_prot_recv_void<T, std::enable_if_t<has_prot_output_type<T>::value && 
                                                    std::is_same_v<typename T::prot_output_type, void>>> 
                    : std::true_type {};
                    
                // Check if T has void operator>>(future_with<prot_output_type>&)
                template <typename T, typename = void>
                struct has_future_with_op : std::false_type {};
                
                template <typename T>
                struct has_future_with_op<T, std::enable_if_t<has_prot_output_type<T>::value && 
                                                     !is_prot_recv_void<T>::value,
                                  std::void_t<decltype(std::declval<T>().operator>>(
                                      std::declval<future_with<typename T::prot_output_type>&>()))>>> 
                    : std::true_type {};
                    
                // Check if T has void operator>>(prot_output_type&)
                template <typename T, typename = void>
                struct has_direct_op : std::false_type {};
                
                template <typename T>
                struct has_direct_op<T, 
                                std::void_t<decltype(std::declval<T>().operator>>(
                                    std::declval<typename T::prot_output_type&>()))> >
                    : std::true_type {};
                    
                // Check if T has void operator>>(future&) and prot_output_type is void
                template <typename T, typename = void>
                struct has_future_op_void_type : std::false_type {};
                
                template <typename T>
                struct has_future_op_void_type<T, std::enable_if_t<is_prot_recv_void<T>::value,
                                     std::void_t<decltype(std::declval<T>().operator>>(
                                         std::declval<future&>()))>>> 
                    : std::true_type {};
                    
                // Check if T has U operator<<(const prot_input_type&) where U is convertible to io::future
                template <typename T, typename = void>
                struct has_future_send_op : std::false_type {};
                
                template <typename T>
                struct has_future_send_op<T,
                                std::void_t<decltype(
                                    std::declval<io::future&>() = std::declval<T>().operator<<(
                                        std::declval<const typename T::prot_input_type&>()))> >
                    : std::true_type {};
                    
                // Check if T has void operator<<(const prot_input_type&)
                template <typename T, typename = void>
                struct has_void_send_op : std::false_type {};
                
                template <typename T>
                struct has_void_send_op<T, 
                                std::void_t<decltype(std::declval<T>().operator<<(
                                    std::declval<const typename T::prot_input_type&>()))> >
                    : std::true_type {};

                template <typename T>
                struct movable_future_with :future_with<T> {
                    movable_future_with() {}
                    movable_future_with(movable_future_with&& m) :future_with<T>(){}
                };
            }
            
            template <typename T>
            struct is_output_prot {
                // is typename T a receive protocol
                static constexpr bool value = 
                    (detail::has_prot_output_type<T>::value && 
                    (detail::has_future_with_op<T>::value || 
                     detail::has_direct_op<T>::value));
                
                // await is true for future_with op or future op with void type
                static constexpr bool await = 
                    (detail::has_future_with_op<T>::value || 
                     detail::has_future_op_void_type<T>::value);
            };
            
            template <typename T>
            struct is_input_prot {
                // is typename T a send protocol
                static constexpr bool value = 
                    (detail::has_prot_input_type<T>::value && 
                    (detail::has_future_send_op<T>::value || 
                     detail::has_void_send_op<T>::value));
                
                // await is true for future-returning operator<< and false for void-returning operator<<
                static constexpr bool await = detail::has_future_send_op<T>::value;
            };

            template <typename T>
            inline constexpr bool is_dualput_v = is_output_prot<T>::value && is_input_prot<T>::value;

            template <typename F, typename T, typename = void> struct is_adaptor : std::false_type {
                using result_type = void;
            };

            template <typename F, typename T>
            struct is_adaptor<F, T, std::void_t<decltype(std::declval<F>()(std::declval<const T&>()))>>
            {
            private:
                using ReturnType = decltype(std::declval<F>()(std::declval<const T&>()));

                template <typename R>
                struct is_optional : std::false_type {};

                template <typename U>
                struct is_optional<std::optional<U>> : std::true_type {
                    using value_type = U;
                };

            public:
                static constexpr bool value = is_optional<ReturnType>::value;

                using result_type = typename std::conditional_t<
                    value,
                    is_optional<ReturnType>,
                    std::false_type
                >::value_type;
            };

            template <typename T, typename = void>
            inline constexpr bool is_pipeline_v = false;

            template <typename T>
            inline constexpr bool is_pipeline_v<T,
                std::void_t<
                typename T::Front_t,
                typename T::Rear_t,
                typename T::Adaptor_t,
                std::enable_if_t<std::is_same_v<T, pipeline<
                typename T::Front_t,
                typename T::Rear_t,
                typename T::Adaptor_t>>>
                >> = true;

            template <typename T>
            struct is_output_prot_gen {
                static constexpr bool value = is_output_prot<T>::value;
                static constexpr bool await = is_output_prot<T>::await;
                static constexpr bool is_pipeline = false;
                using prot_output_type = typename T::prot_output_type;
            };

            template <typename Front, typename Rear, typename Adaptor>
            struct is_output_prot_gen<pipeline<Front, Rear, Adaptor>> {
                static constexpr bool value = is_output_prot<std::remove_reference_t<Rear>>::value;
                static constexpr bool await = is_output_prot<std::remove_reference_t<Rear>>::await;
                static constexpr bool is_pipeline = true;
                using prot_output_type = typename std::remove_reference_t<Rear>::prot_output_type;
            };

            template <typename T1, typename T2>
            struct is_compatible_prot_pair {
                static constexpr bool value = 
                    is_output_prot_gen<T1>::value &&
                    is_input_prot<T2>::value && 
                    (is_output_prot_gen<T1>::await || is_input_prot<T2>::await);
            };

            template <typename T1, typename T2>
            inline constexpr bool is_compatible_prot_pair_v = is_compatible_prot_pair<T1, T2>::value;

            // Concept to check if a type is a valid error handler for pipeline
            template <typename T>
            concept PipelineErrorHandler =
                requires(T handler, int which, bool output_or_input) {
                    { handler(which, output_or_input) } -> std::same_as<void>;
            };

            template <typename F>
            struct function_traits;

            template <typename R, typename Arg>
            struct function_traits<R(*)(Arg)> {
                using result_type = R;
                template <size_t i>
                struct arg {
                    using type = typename std::tuple_element<i, std::tuple<Arg>>::type;
                };
            };

            template <typename R, typename Arg>
            struct function_traits<std::function<R(Arg)>> {
                using result_type = R;
                template <size_t i>
                struct arg {
                    using type = typename std::tuple_element<i, std::tuple<Arg>>::type;
                };
            };

            template <typename C, typename R, typename Arg>
            struct function_traits<R(C::*)(Arg)> {
                using result_type = R;
                template <size_t i>
                struct arg {
                    using type = typename std::tuple_element<i, std::tuple<Arg>>::type;
                };
            };

            template <typename C, typename R, typename Arg>
            struct function_traits<R(C::*)(Arg) const> {
                using result_type = R;
                template <size_t i>
                struct arg {
                    using type = typename std::tuple_element<i, std::tuple<Arg>>::type;
                };
            };

            template <typename F>
            struct function_traits : public function_traits<decltype(&F::operator())> {};
        };

        template <typename Front = void, typename Rear = void, typename Adaptor = void>
        struct pipeline {
            __IO_INTERNAL_HEADER_PERMISSION;
            using Rear_t = typename Rear;
            using Front_t = typename Front;
            using Adaptor_t = typename Adaptor;

            inline decltype(auto) start() && {
                return pipeline_started<std::remove_reference_t<decltype(*this)>, false, std::monostate>(std::move(*this));
            }

            template <typename ErrorHandler>
                requires trait::PipelineErrorHandler<ErrorHandler>
            inline decltype(auto) start(ErrorHandler&& e)&& {
                return pipeline_started<std::remove_reference_t<decltype(*this)>, false, ErrorHandler>(std::move(*this), std::forward<ErrorHandler>(e));
            }

            template <typename T_FSM>
            inline decltype(auto) spawn(T_FSM& fsm)&& {
                pipeline_started<std::remove_reference_t<decltype(*this)>, true, void> ret =
                    fsm.spawn_now([](decltype(*this) t)->fsm_func<void> {
                    pipeline_started<std::remove_reference_t<decltype(*this)>, false, std::monostate> pipeline_s(std::move(t));
                    while (1)
                    {
                        pipeline_s <= co_await +pipeline_s;
                    }
                        }(*this));
                return ret;
            }

            template <typename T_FSM, typename ErrorHandler>
                requires trait::PipelineErrorHandler<ErrorHandler>
            inline decltype(auto) spawn(T_FSM& fsm, ErrorHandler&& e)&& {
                pipeline_started<std::remove_reference_t<decltype(*this)>, true, ErrorHandler> ret =
                    fsm.spawn_now([](decltype(*this) t, ErrorHandler&& e)->fsm_func<void> {
                    pipeline_started<std::remove_reference_t<decltype(*this)>, false, ErrorHandler> pipeline_s(std::move(t), std::forward<ErrorHandler>(e));
                    while (1)
                    {
                        pipeline_s <= co_await +pipeline_s;
                    }
                        }(*this, std::forward<ErrorHandler>(e)));
                return ret;
            }

            //rear
            template<typename T>
                requires (
                trait::is_output_prot<std::remove_reference_t<Rear>>::value&&
                trait::is_input_prot<std::remove_reference_t<T>>::value&&
                trait::is_compatible_prot_pair_v<std::remove_reference_t<Rear>, std::remove_reference_t<T>>&&
                std::is_convertible_v<typename std::remove_reference_t<Rear>::prot_output_type, typename  std::remove_reference_t<T>::prot_input_type>
                )
                inline auto operator>>(T&& rear)&& {
                return pipeline<std::remove_reference_t<decltype(*this)>,
                    std::conditional_t<
                    std::is_lvalue_reference_v<T>,
                    std::add_lvalue_reference_t<std::remove_reference_t<T>>,
                    std::remove_reference_t<T>
                    >, void>(std::move(*this), std::forward<T>(rear));
            }

            //adaptor
            template<typename T>
                requires (
                trait::is_output_prot<std::remove_reference_t<Rear>>::value&&
                trait::is_adaptor<T, typename std::remove_reference_t<Rear>::prot_output_type>::value
                )
                inline auto operator>>(T&& adaptor)&& {
                return pipeline_constructor<std::remove_reference_t<decltype(*this)>, void,
                    std::conditional_t<
                    std::is_lvalue_reference_v<T>,
                    std::add_lvalue_reference_t<std::remove_reference_t<T>>,
                    std::remove_reference_t<T>
                    >>(std::move(*this), std::forward<T>(adaptor));
            }

            consteval static size_t pair_sum() {
                if constexpr (trait::is_pipeline_v<std::remove_reference_t<Front>>) {
                    return Front::pair_sum() + 1;
                }
                else {
                    return 1;
                }
            }

            pipeline(pipeline&) = delete;
            void operator=(pipeline&) = delete;

            pipeline(pipeline&&) = default;
            
        private:
            inline decltype(auto) awaitable() {
                std::array<future*, pair_sum()> futures;
                size_t index = 0;
                await_get(futures, index);
                if constexpr (pair_sum() == 1)
                {
                    future& ref = *futures[0];
                    return ref;
                }
                else
                {
                    return std::apply([](auto&&... args) {
                        return future::race(*args...);
                        }, futures);
                }
            }

            template <typename ErrorHandler>
            inline void process(int which, ErrorHandler errorHandler) {
                if (which == this->pair_sum() - 1) {
                    if constexpr (trait::is_output_prot_gen<std::remove_reference_t<Front>>::await && trait::is_input_prot<std::remove_reference_t<Rear>>::await) {
                        if (turn == 1) {
                            if (front_future.getErr()) {
                                if constexpr (std::is_same_v<ErrorHandler, std::monostate> == false)
                                {
                                    errorHandler(which, true);
                                }
                                turn = 0;
                            }
                            else {
                                if constexpr (!std::is_void_v<Adaptor>) {
                                    auto adapted_data = adaptor(front_future.data);
                                    if (adapted_data) {
                                        rear_future = rear << *adapted_data;
                                        turn = 3;
                                    }
                                    else {
                                        if constexpr (trait::is_pipeline_v<std::remove_reference_t<Front>>) {
                                            front.rear >> front_future;
                                        }
                                        else {
                                            front >> front_future;
                                        }
                                        turn = 1;
                                    }
                                }
                                else {
                                    rear_future = rear << front_future.data;
                                    turn = 3;
                                }
                            }
                        }
                        else if (turn == 3) {
                            if (front_future.getErr()) {
                                if constexpr (std::is_same_v<ErrorHandler, std::monostate> == false)
                                {
                                    errorHandler(which, false);
                                }
                            }
                            turn = 0;
                        }
                        else {
                            assert(!"pipeline ERROR: unexcepted turn clock!");
                        }
                    }
                    else if constexpr (trait::is_output_prot_gen<std::remove_reference_t<Front>>::await) {
                        if (turn == 1) {
                            if (front_future.getErr()) {
                                if constexpr (std::is_same_v<ErrorHandler, std::monostate> == false)
                                {
                                    errorHandler(which, true);
                                }
                                turn = 0;
                            }
                            else {
                                if constexpr (!std::is_void_v<Adaptor>) {
                                    auto adapted_data = adaptor(front_future.data);
                                    if (adapted_data) {
                                        rear << *adapted_data;
                                    }
                                }
                                else {
                                    rear << front_future.data;
                                }
                            }
                            turn = 0;
                        }
                        else {
                            assert(!"pipeline ERROR: unexcepted turn clock!");
                        }
                    }
                    else if constexpr (trait::is_input_prot<std::remove_reference_t<Rear>>::await) {
                        if (turn == 3) {
                            if (rear_future.getErr()) {
                                if constexpr (std::is_same_v<ErrorHandler, std::monostate> == false)
                                {
                                    errorHandler(which, false);
                                }
                            }
                            turn = 2;
                        }
                        else {
                            assert(!"pipeline ERROR: unexcepted turn clock!");
                        }
                    }
                }
                else {
                    if constexpr (trait::is_pipeline_v<std::remove_reference_t<Front>>) {
                        front.process(which, errorHandler);
                    }
                    else {
                        assert(!"pipeline ERROR: unexcepted pipeline num!");
                    }
                }
            }

            template <size_t N>
            inline void await_get(std::array<future*, N>& futures, size_t& index) {
                if constexpr (trait::is_pipeline_v<std::remove_reference_t<Front>>) {
                    front.await_get(futures, index);
                }
                if constexpr (trait::is_output_prot_gen<std::remove_reference_t<Front>>::await && trait::is_input_prot<std::remove_reference_t<Rear>>::await) {
                    if (turn == 0) {
                        if constexpr (trait::is_pipeline_v<std::remove_reference_t<Front>>) {
                            front.rear >> front_future;
                        }
                        else {
                            front >> front_future;
                        }
                        futures[index++] = std::addressof(front_future);
                        turn = 1;
                    }
                    else if (turn == 1) {
                        futures[index++] = std::addressof(front_future);
                    }
                    else if (turn == 3) {
                        futures[index++] = std::addressof(rear_future);
                    }
                    else {
                        assert(!"pipeline ERROR: unexcepted turn clock!");
                    }
                } else if constexpr (trait::is_output_prot_gen<std::remove_reference_t<Front>>::await) {
                    if (turn == 0) {
                        if constexpr (trait::is_pipeline_v<std::remove_reference_t<Front>>) {
                            front.rear >> front_future;
                        }
                        else {
                            front >> front_future;
                        }
                        futures[index++] = std::addressof(front_future);
                        turn = 1;
                    } else if (turn == 1) {
                        futures[index++] = std::addressof(front_future);
                    }
                    else {
                        assert(!"pipeline ERROR: unexcepted turn clock!");
                    }
                } else if constexpr (trait::is_input_prot<std::remove_reference_t<Rear>>::await) {
                    if (turn == 0 || turn == 2) {
                        if constexpr (!std::is_void_v<Adaptor>) {
                            bool adapted = false;
                            while (!adapted) {
                                if constexpr (trait::is_pipeline_v<std::remove_reference_t<Front>>) {
                                    front.rear >> front_future;
                                }
                                else {
                                    front >> front_future;
                                }
                                auto adapted_data = adaptor(front_future);
                                if (adapted_data) {
                                    rear_future = rear << *adapted_data;
                                    adapted = true;
                                }
                            }
                        }
                        else {
                            if constexpr (trait::is_pipeline_v<std::remove_reference_t<Front>>) {
                                front.rear >> front_future;
                            }
                            else {
                                front >> front_future;
                            }
                            rear_future = rear << front_future;
                        }
                        
                        futures[index++] = std::addressof(rear_future);
                        turn = 3;
                    }
                    else if (turn == 3) {
                        futures[index++] = std::addressof(rear_future);
                    }
                    else {
                        assert(!"pipeline ERROR: unexcepted turn clock!");
                    }
                }
                else {
                    static_assert(trait::is_input_prot<std::remove_reference_t<Rear>>::await, "pipeline ERROR: Two Direct protocols (Direct Output Protocol and Direct Input Protocol) cannot be connected to each other in a pipeline segment!");
                }
            }

            // Constructor for pipeline with front and rear protocols (no adaptor)
            inline pipeline(Front&& f, Rear&& r)
            : front(std::forward<Front>(f))
                , rear(std::forward<Rear>(r)) {
            }

            // Constructor for pipeline with front, rear and adaptor
            template <typename U = Adaptor>
            inline pipeline(Front&& f, Rear&& r, std::enable_if<!std::is_void_v<U>, U>::type&& a)
            : front(std::forward<Front>(f))
                , rear(std::forward<Rear>(r))
                , adaptor(std::forward<U>(a)) {
            }

            template <typename U = Adaptor>
            inline pipeline(Front&& f, Rear&& r, int a)
            : front(std::forward<Front>(f))
                , rear(std::forward<Rear>(r)) {
            }

            Front front;
            Rear rear;
            [[no_unique_address]] std::conditional_t<std::is_void_v<Adaptor>, std::monostate, Adaptor> adaptor;
            std::conditional_t<
                trait::is_output_prot_gen<std::remove_reference_t<Front>>::await,
                trait::detail::movable_future_with<typename trait::is_output_prot_gen<std::remove_reference_t<Front>>::prot_output_type>,
                typename trait::is_output_prot_gen<std::remove_reference_t<Front>>::prot_output_type
            > front_future;
            [[no_unique_address]] std::conditional_t<
                trait::is_input_prot<std::remove_reference_t<Rear>>::await,
                future,
                std::monostate
            > rear_future;
            int turn = 0; //0 front before operator<<,1 front after operator<<, 2 rear before operator>>, 3 rear after operator>>
        };

        template <typename Front, typename Rear, typename Adaptor>
        struct pipeline_constructor {
            __IO_INTERNAL_HEADER_PERMISSION;
            using Rear_t = typename Rear;
            using Front_t = typename Front;
            using Adaptor_t = typename Adaptor;
            // Continue construction with adaptor
            template<typename T>
                requires (
            std::is_void_v<Rear>&&
                std::is_void_v<Adaptor>&&
                trait::is_adaptor<T, typename trait::is_output_prot_gen<std::remove_reference_t<Front>>::prot_output_type>::value
                )
                inline auto operator>>(T&& adaptor)&& {
                return pipeline_constructor<Front, void,
                    std::conditional_t<
                    std::is_lvalue_reference_v<T>,
                    std::add_lvalue_reference_t<std::remove_reference_t<T>>,
                    std::remove_reference_t<T>
                    >>(std::forward<Front>(front), std::forward<T>(adaptor));
            }

            // Create final pipeline with input protocol
            template<typename T>
                requires (
                std::is_void_v<Rear>&&
                std::is_void_v<Adaptor>&&
                trait::is_input_prot<typename std::remove_reference_t<T>>::value&&
                trait::is_compatible_prot_pair_v<std::remove_reference_t<Front>, std::remove_reference_t<T>>&&
                std::is_convertible_v<typename trait::is_output_prot_gen<std::remove_reference_t<Front>>::prot_output_type, typename std::remove_reference_t<T>::prot_input_type>
                )
                inline auto operator>>(T&& rear)&& {
                return pipeline<Front,
                    std::conditional_t<
                    std::is_lvalue_reference_v<T>,
                    std::add_lvalue_reference_t<std::remove_reference_t<T>>,
                    std::remove_reference_t<T>
                    >, Adaptor>(std::forward<Front>(front), std::forward<T>(rear));
            }

            // Create final pipeline with input protocol and adaptor
            template<typename T>
                requires (
                std::is_void_v<Rear> &&
                !std::is_void_v<Adaptor>&&
                trait::is_input_prot<std::remove_reference_t<T>>::value&&
                trait::is_compatible_prot_pair_v<std::remove_reference_t<Front>, std::remove_reference_t<T>>&&
                std::is_convertible_v<typename trait::is_adaptor<Adaptor, typename trait::is_output_prot_gen<std::remove_reference_t<Front>>::prot_output_type>::result_type, typename std::remove_reference_t<T>::prot_input_type>
                )
                inline auto operator>>(T&& rear)&& {
                return pipeline<Front,
                    std::conditional_t<
                    std::is_lvalue_reference_v<T>,
                    std::add_lvalue_reference_t<std::remove_reference_t<T>>,
                    std::remove_reference_t<T>
                    >, Adaptor>(std::forward<Front>(front), std::forward<T>(rear), std::forward<Adaptor>(adaptor));
            }

        private:
            inline pipeline_constructor(Front&& f)
                : front(std::forward<Front>(f)) {
            }

            template <typename U = Adaptor>
            inline pipeline_constructor(Front&& f, std::enable_if<!std::is_void_v<U>, U>::type&& a)
                : front(std::forward<Front>(f)), adaptor(std::forward<U>(a)) {
            }

            template <typename U = Adaptor>
            inline pipeline_constructor(Front&& f, int a)   //never use
                : front(std::forward<Front>(f)) {
            }
            Front front;
            [[no_unique_address]] std::conditional_t<std::is_void_v<Adaptor>, std::monostate, Adaptor> adaptor;
        };

        // pipeline_started class - represents a started pipeline that cannot be extended
        template <typename Pipeline, bool individual_coro, typename ErrorHandler>
        class pipeline_started : std::conditional_t<individual_coro, fsm_handle<void>, std::monostate> {
            __IO_INTERNAL_HEADER_PERMISSION;
        public:
            // Constructor that takes ownership of a pipeline
            template <typename Front, typename Rear, typename Adaptor>
            explicit pipeline_started(pipeline<Front, Rear, Adaptor>&& pipe)
                : _pipeline(std::move(pipe)) {
            }

            template <typename Front, typename Rear, typename Adaptor>
            explicit pipeline_started(pipeline<Front, Rear, Adaptor>&& pipe, ErrorHandler&& e)
                : _pipeline(std::move(pipe)), errorHandler(std::forward<ErrorHandler>(e)) {
            }

            // Delete copy constructor and assignment
            pipeline_started(const pipeline_started&) = delete;
            pipeline_started& operator=(const pipeline_started&) = delete;

            // Drive the pipeline with a specific index
            inline void operator<=(int which) requires (individual_coro == false){
                if constexpr (std::is_same_v<ErrorHandler, std::monostate>) {
                    _pipeline.process(which, std::monostate{});
                }
                else {
                    _pipeline.process(which, std::ref(errorHandler));
                }
            }

            // Get the awaitable for the pipeline
            inline decltype(auto) operator+() requires (individual_coro == false) {
                return _pipeline.awaitable();
            }

        private:
            // Delete move constructor and assignment
            pipeline_started(pipeline_started&&) = default;
            pipeline_started& operator=(pipeline_started&&) = default;

            [[no_unique_address]] std::conditional_t<individual_coro, std::monostate, Pipeline> _pipeline;
            [[no_unique_address]] ErrorHandler errorHandler;
        };

        template <>
        struct pipeline<void, void, void> {
            __IO_INTERNAL_HEADER_PERMISSION;
            // Chain operator for starting the pipeline - requires output protocol
            template<typename T>
                requires trait::is_output_prot<std::remove_reference_t<T>>::value
            inline auto operator>>(T&& front) && {
                return pipeline_constructor<std::conditional_t<
                    std::is_lvalue_reference_v<T>,
                    std::add_lvalue_reference_t<std::remove_reference_t<T>>,
                    std::remove_reference_t<T>
                >, void, void>(std::forward<T>(front));
            }
            inline pipeline() {}
        };

        //helper struct of rpc<>::def.
        template <>
        struct rpc<void, void, void> {
            rpc() = delete;
            
            //default struct of rpc
            template <typename Req, typename Resp>
            struct def {
                def(std::function<Resp(Req)> h) : handler(h) {}
                def() = default;
                
                std::function<Resp(Req)> handler;
                
                operator bool() const {
                    return static_cast<bool>(handler);
                }
            };
            
            template <typename F>
            def(F) -> def<typename std::remove_reference_t<typename trait::function_traits<F>::template arg<0>::type>,
                          typename trait::function_traits<F>::result_type>;
        };

        template <typename key = void, typename req = void, typename rsp = void>
        struct rpc {
            __IO_INTERNAL_HEADER_PERMISSION;
            using key_type = key;
            using request_type = req;
            using response_type = rsp;
            using handler_type = std::function<response_type(request_type)>;
            
            template <typename... Args>
            inline rpc(Args&&... args) {
                process_args(std::forward<Args>(args)...);
            }
            
            // Call operator to invoke the appropriate handler
            inline response_type operator()(const std::pair<key_type, request_type>& request) {
                const auto& [key, req] = request;
                auto it = handlers_.find(key);
                if (it != handlers_.end()) {
                    return it->second(req);
                } else if (default_handler_) {
                    return default_handler_(req);
                } else {
                    throw std::runtime_error("No handler found for key and no default handler provided");
                }
            }

        private:
            std::unordered_map<key_type, handler_type> handlers_;
            handler_type default_handler_;
            
            template <typename... Args>
            inline void process_args(const std::pair<key_type, handler_type>& pair, Args&&... args) {
                handlers_[pair.first] = pair.second;
                process_args(std::forward<Args>(args)...);
            }
            
            template <typename Def>
            inline void process_args(const Def& default_handler) {
                if constexpr (std::is_same_v<Def, typename rpc<>::template def<request_type, response_type>> ||
                             std::is_convertible_v<decltype(default_handler.handler), handler_type>) {
                    if (default_handler) {
                        default_handler_ = default_handler.handler;
                    }
                } else {
                    static_assert(std::is_same_v<Def, std::pair<key_type, handler_type>>, 
                        "RPC construct ERROR: def function signature must be response_type(request_type)");
                }
            }
            
            inline void process_args() {}
        };



        // -------------------------------socket/protocol impl--------------------------------

        // hardware/system io socket
        namespace sock {
            struct tcp {
                __IO_INTERNAL_HEADER_PERMISSION;
                friend class std::optional<tcp>;
                using prot_output_type = std::span<char>;
                using prot_input_type = std::span<char>;
                static constexpr size_t default_buffer_size = 1024 * 16;
                template <typename T_FSM>
                inline tcp(fsm<T_FSM>& state_machine) : manager(state_machine.getManager()), asio_sock(manager->io_ctx) {
                    buffer.resize(default_buffer_size);
                }

                inline void setBufSize(size_t size) {
                    buffer.resize(size);
                }

                //receive function
                inline void operator >>(future_with<std::span<char>>& fut) {
                    promise<std::span<char>> prom = manager->make_future(fut, &fut.data);
                    if (!asio_sock.is_open()) {
                        prom.reject_later(std::make_error_code(std::errc::not_connected));
                        return;
                    }

                    asio_sock.async_wait(asio::ip::tcp::socket::wait_read,
                        [this, prom = std::move(prom)](const std::error_code& ec) mutable {
                            if (ec) {
                                prom.reject_later(ec);
                                return;
                            }

                            std::error_code read_ec;
                            size_t bytes_read = asio_sock.read_some(asio::buffer(buffer), read_ec);
                            if (read_ec) {
                                prom.reject_later(read_ec);
                                return;
                            }

                            auto ptr = prom.resolve_later();
                            if (ptr)
                            {
                                *ptr = std::span<char>(buffer.data(), bytes_read);
                            }
                        });
                }

                //send function
                inline future operator <<(const std::span<char>& span) {
                    future fut;
                    promise<> prom = manager->make_future(fut);
                    if (!asio_sock.is_open()) {
                        prom.reject_later(std::make_error_code(std::errc::not_connected));
                        return fut;
                    }

                    // Try immediate non-blocking send first
                    std::error_code write_ec;
                    size_t bytes_written = asio_sock.write_some(asio::buffer(span.data(), span.size()), write_ec);
                    
                    // Check if we wrote everything successfully
                    if (!write_ec && bytes_written == span.size()) {
                        prom.resolve_later();
                        return fut;
                    }
                    
                    // If we got an error other than would_block, reject
                    if (write_ec && write_ec != asio::error::would_block) {
                        prom.reject_later(write_ec);
                        return fut;
                    }
                    
                    // Create a copy of remaining data using unique_ptr
                    auto remaining_data = std::make_unique<std::string>(
                        span.data() + bytes_written, 
                        span.size() - bytes_written
                    );
                    
                    // Use helper function to handle recursive sending
                    async_send_remaining(std::move(remaining_data), std::move(prom));
                    
                    return fut;
                }

                inline future connect(const asio::ip::tcp::endpoint& endpoint) {
                    future fut;
                    promise<> prom = manager->make_future(fut);

                    asio_sock.async_connect(endpoint,
                        [this, prom = std::move(prom)](const std::error_code& ec) mutable {
                            if (ec) {
                                prom.reject_later(ec);
                            }
                            else {
                                // Set non-blocking mode after successful connection
                                std::error_code nb_ec;
                                asio_sock.non_blocking(true, nb_ec);
                                prom.resolve_later();
                            }
                        });

                    return fut;
                }

                inline void close() {
                    if (asio_sock.is_open()) {
                        std::error_code ec;
                        asio_sock.close(ec);
                    }
                }

                IO_MANAGER_FORWARD_FUNC(asio_sock, native_handle);
                IO_MANAGER_FORWARD_FUNC(asio_sock, is_open);
                IO_MANAGER_FORWARD_FUNC(asio_sock, remote_endpoint);
                IO_MANAGER_FORWARD_FUNC(asio_sock, local_endpoint);
                IO_MANAGER_FORWARD_FUNC(asio_sock, available);
                inline tcp(asio::ip::tcp::socket&& sock, size_t size, io::manager* mngr) :manager(mngr), asio_sock(std::move(sock)) {
                    buffer.resize(size);
                }
            private:
                // Helper function to recursively send remaining data
                void async_send_remaining(std::unique_ptr<std::string> data, promise<> prom) {
                    asio_sock.async_write_some(
                        asio::buffer(data->data(), data->size()),
                        [this, data = std::move(data), prom = std::move(prom)]
                        (const std::error_code& ec, size_t bytes_sent) mutable {
                            if (ec) {
                                prom.reject_later(ec);
                                return;
                            }

                            if (bytes_sent < data->size()) {
                                // Still data left to send - update the buffer and recurse
                                data->erase(0, bytes_sent);
                                async_send_remaining(std::move(data), std::move(prom));
                            }
                            else {
                                // All data sent successfully
                                prom.resolve_later();
                            }
                        });
                }
                io::manager* manager = nullptr;
                std::string buffer;
                asio::ip::tcp::socket asio_sock;
            };

            struct tcp_accp {
                __IO_INTERNAL_HEADER_PERMISSION;
                using prot_output_type = std::optional<tcp>;
                template <typename T_FSM>
                inline tcp_accp(fsm<T_FSM>& state_machine) : manager(state_machine.getManager()), acceptor(manager->io_ctx), sock(manager->io_ctx) {}

                inline void setBufSize(size_t size) {
                    buffer_size = size;
                }

                inline bool bind_and_listen(const asio::ip::tcp::endpoint& endpoint, int backlog = 10) {
                    std::error_code ec;
                    acceptor.open(endpoint.protocol(), ec);
                    if (ec) {
                        return false;
                    }

                    acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true), ec);
                    if (ec) {
                        return false;
                    }

                    acceptor.bind(endpoint, ec);
                    if (ec) {
                        return false;
                    }

                    acceptor.listen(backlog, ec);
                    if (ec) {
                        return false;
                    }
                    
                    return !ec;
                }

                inline void operator >>(future_with<std::optional<tcp>>& fut) {
                    promise<std::optional<tcp>> prom = manager->make_future(fut, &fut.data);
                    if (!acceptor.is_open()) {
                        fut.getPromise().reject_later(std::make_error_code(std::errc::not_connected));
                        return;
                    }

                    acceptor.async_accept(sock,
                        [this, prom = std::move(prom)](const std::error_code& ec) mutable {
                            if (ec) {
                                prom.reject_later(ec);
                                return;
                            }

                            // Set the accepted socket to non-blocking
                            std::error_code nb_ec;
                            sock.non_blocking(true, nb_ec);
                            if (nb_ec) {
                                prom.reject_later(nb_ec);
                                return;
                            }

                            auto ptr = prom.resolve_later();
                            if (ptr)
                            {
                                ptr->emplace(std::move(sock), buffer_size, manager);
                            }
                            else
                            {
                                std::error_code ec;
                                sock.close(ec);
                            }
                        });
                }

                inline void close() {
                    if (acceptor.is_open()) {
                        std::error_code ec;
                        acceptor.close(ec);
                    }
                }

                IO_MANAGER_FORWARD_FUNC(acceptor, is_open);
                IO_MANAGER_FORWARD_FUNC(acceptor, local_endpoint);

            private:
                io::manager* manager = nullptr;
                size_t buffer_size = tcp::default_buffer_size;
                asio::ip::tcp::socket sock;
                asio::ip::tcp::acceptor acceptor;
            };

            struct udp {
                __IO_INTERNAL_HEADER_PERMISSION;
                using prot_output_type = std::pair<std::span<char>, asio::ip::udp::endpoint>;
                using prot_input_type = std::pair<std::span<char>, asio::ip::udp::endpoint>;
                static constexpr size_t default_buffer_size = 1024 * 16;
                template <typename T_FSM>
                inline udp(fsm<T_FSM>& state_machine) : manager(state_machine.getManager()), asio_sock(manager->io_ctx) {
                    buffer.resize(default_buffer_size);
                }

                inline void setBufSize(size_t size) {
                    buffer.resize(size);
                }

                //receive function
                inline void operator >>(future_with<std::pair<std::span<char>, asio::ip::udp::endpoint>>& fut) {
                    auto prom = manager->make_future(fut, &fut.data);
                    if (!asio_sock.is_open()) {
                        prom.reject_later(std::make_error_code(std::errc::not_connected));
                        return;
                    }

                    asio_sock.async_receive_from(asio::buffer(buffer), remote_endpoint,
                        [this, prom = std::move(prom)](const std::error_code& ec, size_t bytes_read) mutable {
                            if (ec) {
                                prom.reject_later(ec);
                                return;
                            }

                            auto ptr = prom.resolve_later();
                            if (ptr) {
                                *ptr = std::make_pair(std::span<char>(buffer.data(), bytes_read), remote_endpoint);
                            }
                        });
                }

                //send function
                inline future operator <<(const std::pair<std::span<char>, asio::ip::udp::endpoint>& span) {
                    // Set non-blocking for newly created sockets
                    std::error_code ec;
                    asio_sock.non_blocking(true, ec);

                    future fut;
                    promise<> prom = manager->make_future(fut);
                    if (!asio_sock.is_open()) {
                        prom.reject_later(std::make_error_code(std::errc::not_connected));
                        return fut;
                    }

                    // Try immediate non-blocking send first
                    std::error_code write_ec;
                    size_t bytes_written = asio_sock.send_to(
                        asio::buffer(span.first.data(), span.first.size()), 
                        span.second, 
                        0, // flags
                        write_ec
                    );
                    
                    // For UDP, if we get a would_block, we should use async send
                    // If we get any other error, fail immediately
                    if (!write_ec && bytes_written == span.first.size()) {
                        prom.resolve_later();
                        return fut;
                    }
                    
                    if (write_ec && write_ec != asio::error::would_block) {
                        prom.reject_later(write_ec);
                        return fut;
                    }
                    
                    // Create a copy of data using unique_ptr
                    auto data_copy = std::make_unique<std::string>(
                        span.first.data(), span.first.size()
                    );
                    auto endpoint_copy = span.second;
                    
                    asio_sock.async_wait(asio::ip::udp::socket::wait_write,
                        [this, data = std::move(data_copy), endpoint = endpoint_copy, prom = std::move(prom)]
                        (const std::error_code& ec) mutable {
                            if (ec) {
                                prom.reject_later(ec);
                                return;
                            }

                            std::error_code write_ec;
                            asio_sock.send_to(
                                asio::buffer(data->data(), data->size()), 
                                endpoint, 
                                0, // flags
                                write_ec
                            );
                            
                            if (write_ec) {
                                prom.reject_later(write_ec);
                            } else {
                            prom.resolve_later();
                            }
                        });

                    return fut;
                }

                inline bool bind(const asio::ip::udp::endpoint& endpoint) {
                    std::error_code ec;
                    asio_sock.open(endpoint.protocol(), ec);
                    if (ec) {
                        return false;
                    }

                    asio_sock.bind(endpoint, ec);
                    if (ec) {
                        return false;
                    }
                    return !ec;
                }

                inline void close() {
                    if (asio_sock.is_open()) {
                        std::error_code ec;
                        asio_sock.close(ec);
                    }
                }

                IO_MANAGER_FORWARD_FUNC(asio_sock, native_handle);
                IO_MANAGER_FORWARD_FUNC(asio_sock, is_open);
                IO_MANAGER_FORWARD_FUNC(asio_sock, local_endpoint);

            private:
                io::manager* manager = nullptr;
                std::string buffer;
                asio::ip::udp::socket asio_sock;
                asio::ip::udp::endpoint remote_endpoint;
            };
        };

#include "internal/definitions.h"
    }
}

// software simulated protocols

#if __has_include("protocol/rpc.h")
#include "protocol/rpc.h"
#endif

#if __has_include("protocol/kcp/kcp.h")
#include "protocol/kcp/kcp.h"
#endif

#if __has_include("protocol/http/http.h")
#include "protocol/http/http.h"
#endif