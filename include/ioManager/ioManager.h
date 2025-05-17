/* C++20 coroutine scheduler, protocol, RPC lib.
 * ------Header-Only------
 * 
 * Pipeline Concurrency, a clear data protocol stream processing solution is provided.
 * 
 * using asio for network support, openssl for encryption support.
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

        //Generic memory pool for a single structure category. 
        // Not Thread safe.
        // Simple benchmark result (50M operations * 5 times, clang): 30% faster in random new/delete, 6% slower when sequence new/delete.
        template <typename T, size_t batch_size = 16>
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
                if (freed.empty()) [[unlikely]]
                {
                    for (size_t i = 0; i < batch_size; ++i)
                    {
                        freed.push(static_cast<T*>(::operator new(sizeof(T))));
                    }
                    return new T(std::forward<Args>(args)...);
                }
                else [[likely]]
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
                if (freed.size() > size_limit) [[unlikely]]
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
            size_t size_limit;
            static constexpr double erase_multiple = 0.5;  // in (0,1) zone
            static constexpr bool debug = false;
        };

        //dual buffer. Thread safe. Reentrancy safe.
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

        // Buffer class that can own or reference memory pointer, move only.
        struct buf
        {
            // Default constructor: empty buffer
            inline buf() : data_ptr(nullptr), data_size(0), data_capacity(0), _owned(nullptr) {}
            
            // Constructor from pointer, size, capacity and ownership
            inline buf(char* ptr, size_t size, size_t capacity, char* take_ownership = nullptr)
                : data_ptr(ptr), data_size(size), data_capacity(capacity), _owned(take_ownership) {}

			// Allocates memory with specified capacity
            explicit inline buf(size_t capacity) : data_ptr((char*)::operator new(capacity)), data_size(0), data_capacity(capacity), _owned(data_ptr) {
                IO_ASSERT(capacity > 0, "buf error: capacity must be larger than 0");
            }
            
            // Explicit constructor from span (copies data)
            explicit inline buf(const std::span<const char>& span, size_t capacity = 0) 
                : data_size(span.size()), data_capacity(capacity == 0 ? span.size() : capacity)
            {
                if (capacity > 0 && capacity < span.size()) {
                    IO_ASSERT(false, "Buf ERROR: Capacity cannot be smaller than span size");
                }
                
                data_ptr = static_cast<char *>(::operator new(data_capacity));
                _owned = data_ptr;

                std::copy(span.begin(), span.end(), data_ptr);
            }

            // Constructor from initializer list
            inline buf(std::initializer_list<char> init_list, size_t capacity = 0)
                : data_ptr((char*)::operator new(init_list.size())),
                data_size(init_list.size()),
                data_capacity(capacity == 0 ? init_list.size() : capacity),
                _owned(data_ptr)
            {
                if (capacity > 0 && capacity < init_list.size()) {
                    IO_ASSERT(false, "Buf ERROR: Capacity cannot be smaller than span size");
                }

                std::copy(init_list.begin(), init_list.end(), data_ptr);
            }
            
            // Disable copy constructor and copy assignment
            buf(const buf&) = delete;
            buf& operator=(const buf&) = delete;
            
            // Move constructor
            inline buf(buf&& other) noexcept 
                : data_ptr(other.data_ptr), 
                  data_size(other.data_size), 
                  data_capacity(other.data_capacity),
                  _owned(other._owned) 
            {
                other.data_ptr = nullptr;
                other.data_size = 0;
                other.data_capacity = 0;
                other._owned = nullptr;
            }
            
            // Move assignment
            inline buf& operator=(buf&& other) noexcept {
                if (this != &other) {
                    // Clean up current resources if _owned
                    if (_owned) {
                        ::operator delete(_owned);
                    }
                    
                    // Move the data from other
                    data_ptr = other.data_ptr;
                    data_size = other.data_size;
                    data_capacity = other.data_capacity;
                    _owned = other._owned;
                    
                    // Reset the other object
                    other.data_ptr = nullptr;
                    other.data_size = 0;
                    other.data_capacity = 0;
                    other._owned = nullptr;
                }
                return *this;
            }
            
            // Destructor - properly destruct objects using std::destroy_n before freeing memory
            inline ~buf() {
                if (_owned) {
                    ::operator delete(_owned);
                }
            }
            
            // Conversion to span
            inline operator std::span<char>() {
                return std::span<char>(data_ptr, data_size);
            }

            // Conversion to const span
            inline operator std::span<const char>() const {
                return std::span<const char>(data_ptr, data_size);
            }
            
            // Get pointer (transfers ownership)
            [[nodiscard]] inline char* transfer() {
                char* ptr = _owned;
                data_ptr = nullptr;
                data_size = 0;
                data_capacity = 0;
                _owned = nullptr;
                return ptr;
            }

            // Get pointer (transfers ownership) with data position
            [[nodiscard]] inline char* transfer(char*& data_pos) {
                char* ptr = _owned;
                data_pos = data_ptr;
                data_ptr = nullptr;
                data_size = 0;
                data_capacity = 0;
                _owned = nullptr;
                return ptr;
            }
            
            // Set pointer (takes ownership if specified)
            inline void reset(char* ptr, size_t size, size_t capacity, char* take_ownership = nullptr) {
                if (_owned) {
                    ::operator delete(_owned);
                }
                
                data_ptr = ptr;
                data_size = size;
                data_capacity = capacity;
                _owned = take_ownership;
            }
            
            // Getters
            inline size_t size() const { return data_size; }
            inline size_t capacity() const { return data_capacity; }
            inline char* owned() const { return _owned; }
            inline char* data() const { return data_ptr; }
            
            // Get span for the unused capacity (between size and capacity)
            inline std::span<char> unused_span() {
                if (data_ptr == nullptr || data_size >= data_capacity) {
                    return {}; // Empty span
                }
                return std::span<char>(data_ptr + data_size, data_capacity - data_size);
            }

			inline void resize(size_t new_size) {
				IO_ASSERT(new_size <= data_capacity, "Buffer overflow: resize would exceed capacity");
				data_size = new_size;
			}

            inline size_t size_increase(size_t size_inc) {
                IO_ASSERT(data_size + size_inc <= data_capacity, "Buffer overflow: size_increase would exceed capacity");
                data_size += size_inc;
                return data_size;
            }

			inline size_t size_decrease(size_t size_dec) {
                IO_ASSERT(size_dec <= data_size, "Buffer underflow: size_decrease would result in negative size");
				data_size -= size_dec;
				return data_size;
			}

			inline void data_increase(size_t data_inc) {
                IO_ASSERT(_owned, "This function must own this buffer memory.");
                IO_ASSERT(data_inc <= data_size, "Buffer overflow: size_increase would exceed capacity");
				data_size -= data_inc;
                data_capacity -= data_inc;
                data_ptr += data_inc;
			}

			inline void data_decrease(size_t data_dec) {
                IO_ASSERT(_owned, "This function must own this buffer memory.");
				data_size += data_dec;
				data_capacity += data_dec;
                data_ptr -= data_dec;
                IO_ASSERT(_owned <= data_ptr, "Buffer underflow: size_decrease would result in negative size");
			}
            
            // Add operator bool to check if the buffer is valid
            inline explicit operator bool() const {
                return data_ptr != nullptr;
            }
            
        private:
            char* data_ptr;          // Pointer to data
            size_t data_size;     // Current size
            size_t data_capacity; // Total capacity
            char* _owned;          // Whether this buffer owns the memory
        };

#include "internal/inplaceVector.h"

        template<typename>
        struct is_future_with : std::false_type {};

        template<typename U>
        struct is_future_with<io::future_with<U>> : std::true_type {};

        template<typename T>
        struct is_future { static constexpr bool value = (io::is_future_with<T>::value || std::is_same_v<T, io::future>); };

        // -------------------------------coroutine core--------------------------------

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
                //IO_ASSERT(coro == nullptr, "Awaitable ERROR: resource leak detected.");
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
                return awaiter->bit_set & (awaiter->set_lock & awaiter->occupy_lock);
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

        //future with data. Not moveable and copyable.
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
            inline future_with(future&& right) noexcept :future(std::move(right)) {}
            inline future_with& operator=(future&& right) noexcept {
                this->future::operator=(std::move(right));
                return *this;
            }
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
                this->lowlevel::promise_base::resolve();
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
            inline bool resolve(auto&& _data)
            {
                T* pdata = data();
                if (pdata)
                {
                    *pdata = std::forward<T>(_data);
                    this->lowlevel::promise_base::resolve();
                    return true;
                }
                else
                {
                    return false;
                }
            }
            inline bool resolve_later(auto&& _data)
            {
                T* pdata = data();
                if (pdata)
                {
                    *pdata = std::forward<T>(_data);
                    this->lowlevel::promise_base::resolve_later();
                    return true;
                }
                else
                {
                    return false;
                }
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
            promise<void> getPromise() = delete;
            void decons() noexcept;
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
            inline bool reject(std::errc ec) { return reject(std::make_error_code(ec)); }
            inline ~async_promise() {
                decons();
            }
            inline bool hasValue() { return awaiter.load(); }
        private:
            std::atomic<lowlevel::awaiter*> awaiter = nullptr;
            void decons(lowlevel::awaiter* exchange_ptr = nullptr) noexcept;
        };

        namespace async {
			using promise = io::async_promise;
            using future = io::async_future;
        };

        struct get_fsm_t {};
        inline static constexpr get_fsm_t get_fsm;
        
        struct yield_t {};
        inline static constexpr yield_t yield;

        template <typename T>
            requires std::invocable<T>
        struct defer_t {
            T func;
            defer_t(T&& func_) :func(func_) {}
            ~defer_t() { func(); }
            defer_t(const defer_t&) = delete;
            defer_t& operator=(const defer_t&) = delete;
            defer_t(defer_t&&) = default;
            defer_t& operator=(defer_t&&) = default;
        };

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
                    IO_ASSERT(x.operator bool() == false, "repeatly co_await in same object!");
                    return { &x.coro, &_fsm.is_awaiting };
                }
                template <typename T_Fut>
                    requires std::is_convertible_v<T_Fut&, io::future&>
                inline lowlevel::awaitable_base<T, lowlevel::selector_status::all, T_Fut> await_transform(T_Fut& x) {
                    IO_ASSERT(static_cast<future&>(x).awaiter->coro == nullptr, "await ERROR: future is not clean: being co_awaited by another coroutine, or not processed by make_future function.");
                    return lowlevel::awaitable_base<T, lowlevel::selector_status::all, T_Fut>(*this, { static_cast<future&>(x).awaiter });
                }
                template <typename T_Fut>
                    requires std::is_convertible_v<T_Fut&, io::future&>
                inline lowlevel::awaitable_base<T, lowlevel::selector_status::all, T_Fut> await_transform(T_Fut&& x) {
                    IO_ASSERT(static_cast<future&&>(x).awaiter->coro == nullptr, "await ERROR: future is not clean: being co_awaited by another coroutine, or not processed by make_future function.");
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
                    return lowlevel::awaitable_base<T, lowlevel::selector_status::race, Args...>(*this, std::move(x.il));
                }
                template <typename ...Args>
                inline lowlevel::awaitable_base<T, lowlevel::selector_status::allsettle, Args...> await_transform(lowlevel::allSettle<Args...>&& x) {
                    return lowlevel::awaitable_base<T, lowlevel::selector_status::allsettle, Args...>(*this, x.il);
                }
                inline lowlevel::awa_initial_suspend<T> initial_suspend() { return lowlevel::awa_initial_suspend<T>(&_fsm); }
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
            inline manager* getManager() {
                return _fsm->_fsm.mngr;
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
                if (is_suspend.test() == false)
                {
#if IO_USE_ASIO
                    io_ctx.run_until(suspend_next);
                    io_ctx.restart();
#else
                    bool _nodiscard = suspend_sem.try_acquire_until(suspend_next);
#endif
                }
                is_suspend.clear();
            }
            
#if IO_USE_ASIO
            // Convert asio::awaitable to io::future
            template <typename T, typename Executor>
            void fromAsio(future& fut, asio::awaitable<T, Executor>&& awaitable_obj) {
                FutureVaild(fut);
                promise<void> prom(fut.awaiter);
                
                // Create a new coroutine to await the asio::awaitable
                asio::co_spawn(
                    io_ctx,
                    [aw = std::move(awaitable_obj), p = std::move(prom)]() mutable -> asio::awaitable<void, Executor> {
                        try {
                            // Await the asio::awaitable
                            co_await std::move(aw);
                            
                            // Resolve the promise when the awaitable completes
                            p.resolve();
                        } catch (const std::exception& e) {
                            // Reject the promise if an exception occurs
                            p.reject(std::make_error_code(std::errc::operation_canceled));
                        }
                    },
                    asio::detached
                );
            }
            
            // Convert asio::awaitable to io::future_with
            template <typename T, typename Executor>
            void fromAsio(future_with<T>& fut, asio::awaitable<T, Executor>&& awaitable_obj) {
                FutureVaild(fut);
                promise<T> prom(fut.awaiter, &fut.data);
                
                // Create a new coroutine to await the asio::awaitable
                asio::co_spawn(
                    io_ctx,
                    [aw = std::move(awaitable_obj), p = std::move(prom)]() mutable -> asio::awaitable<void, Executor> {
                        try {
                            // Await the asio::awaitable and get the result
                            T result = co_await std::move(aw);
                            
                            // Resolve the promise with the result
                            p.resolve(std::move(result));
                        } catch (const std::exception& e) {
                            // Reject the promise if an exception occurs
                            p.reject(std::make_error_code(std::errc::operation_canceled));
                        }
                    },
                    asio::detached
                );
            }

            // Get io_context of this thread
            asio::io_context& getAsioContext() { return io_ctx; }
#endif
            
            // spawn coroutine in same thread, coroutine will be run by the caller manager next turn.
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

                return { new_fsm._data };
            }
            // async co_spawn, coroutine will be detached, and run by the caller manager next turn.
            template <typename T_spawn>
            inline void async_spawn(fsm_func<T_spawn> new_fsm) {
                new_fsm._data->_fsm.is_detached = true;
                new_fsm._data->_fsm.mngr = this;

                while (spinLock_pd.test_and_set(std::memory_order_acquire));
                std::coroutine_handle<fsm_promise<T_spawn>> h;
                h = h.from_promise(*new_fsm._data);
                std::coroutine_handle<> erased_handle;
                erased_handle = h;
                pendingTask.push(erased_handle);
                spinLock_pd.clear(std::memory_order_release);

                this->suspend_release();
            }
            // async wakeup
            void wakeup() {
                suspend_release();
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
            template <typename Func, typename ...Args>
            inline async_future post(manager* executor, Func func, Args&&... args) {
                async_future fut;
                async_promise prom = make_future(fut);
                
                auto func_with_args = [func = std::move(func), 
                                       args_tuple = std::make_tuple(std::forward<Args>(args)...)]() {
                    return std::apply(func, args_tuple);
                };
                
                executor->async_spawn(
                    [](async_promise prom, decltype(func_with_args) func)
                    -> fsm_func<void> {
#if IO_EXCEPTION_ON
                        try {
#endif
                            func();
                            prom.resolve();
#if IO_EXCEPTION_ON
                        }
                        catch (...) {
                            prom.reject(std::make_error_code(std::errc::invalid_argument));
                        }
#endif
                        co_return;
                    }(std::move(prom), std::move(func_with_args)));
                
                return fut;
            }
            template <typename Func, typename ...Args>
            async_future post(pool& thread_pool, Func func, Args&&... args);
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

            std::atomic_flag is_suspend = ATOMIC_FLAG_INIT;

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
                if (is_suspend.test_and_set() == false)
                {
#if IO_USE_ASIO
                    io_ctx.stop();
#else
                    this->suspend_sem.release();
#endif
                }
            }

            io::hive<lowlevel::awaiter> awaiter_hive = io::hive<lowlevel::awaiter>(1000);
        };

        //thread pool. 
        // Launch when constructing.
        struct pool {
			__IO_INTERNAL_HEADER_PERMISSION;
            struct _thread {
				std::thread thread;
                std::atomic_flag stopFlag = ATOMIC_FLAG_INIT;
				manager mngr;
                inline _thread() {
                    thread = std::thread([this]() {
                        while (!stopFlag.test(std::memory_order_acquire)) {
                            mngr.drive();
                        }
                    });
                }
                _thread(const _thread&) = delete;
                _thread& operator=(const _thread&) = delete;
                _thread(_thread&&) = delete;
                _thread& operator=(_thread&&) = delete;
            };
            std::deque<_thread> threadsInPool;
            // Thread distribution counter
            std::atomic<size_t> next_thread = 0;

            pool(const pool&) = delete;
            pool& operator=(const pool&) = delete;
            inline pool(pool&& other) noexcept : threadsInPool(std::move(other.threadsInPool)), next_thread(other.next_thread.load()) {}
            inline pool& operator=(pool&& other) noexcept {
                if (this != &other) {
                    stop();
                    threadsInPool = std::move(other.threadsInPool);
					next_thread = other.next_thread.load();
                }
                return *this;
            }

            /**
             * Posts a function to be executed on a thread in the pool
             * 
             * @return async_future that can be used to await completion
             */
            template <typename Func, typename ...Args>
            inline async_future post(manager* future_carrier, Func func, Args&&... args) {
                if (threadsInPool.empty()) {
                    async_future fut;
                    auto prom = future_carrier->make_future(fut);
                    prom.reject(std::make_error_code(std::errc::not_connected));
                    return fut;
                }
                
                return future_carrier->post(
                    &(threadsInPool[next_thread++ % threadsInPool.size()].mngr),
                    std::move(func),
                    std::forward<Args>(args)...);
            }
            
            /**
             * Stops all threads in the pool and clears the pool
			 * This thread will be join until all threads are finished
             */
            inline void stop() {
                if (threadsInPool.empty()) {
                    return;
                }
                
                for (auto& t : threadsInPool) {
                    t.stopFlag.test_and_set(std::memory_order_release);
                    t.mngr.wakeup();
                }
                
                for (auto& t : threadsInPool) {
                    if (t.thread.joinable()) {
                        t.thread.join();
                    }
                }
                
                threadsInPool.clear();
            }
            
            /**
             * Spawns a coroutine to be executed on a thread in the pool
             */
            template <typename T_spawn>
            inline void async_spawn(fsm_func<T_spawn> new_fsm) {
                if (threadsInPool.empty()) {
                    return;
                }
                
                manager* target_manager = &(threadsInPool[next_thread % threadsInPool.size()].mngr);
                next_thread++;
                
                target_manager->async_spawn(std::move(new_fsm));
            }

            manager* getManager() {
                return &(threadsInPool[next_thread++ % threadsInPool.size()].mngr);
            }
            
            /**
             * Creates a pool with specified thread count
             */
			inline explicit pool(size_t thread_count = 1) {
                threadsInPool.resize(thread_count);
            }
            
            /**
             * Checks if the pool is currently running
             */
            inline bool is_running() const {
                return threadsInPool.size();
            }
            
            inline ~pool() {
                stop();
            }
        };



        // -------------------------------protocol trait-----------------------------------------

        //protocol traits
        namespace trait {
            template <typename F, typename T, typename = void> struct is_adaptor : std::false_type {
                using result_type = void;
            };

            template <typename F, typename T>
            struct is_adaptor<F, T, std::void_t<decltype(std::declval<F>()(std::declval<T&>()))>>
            {
            private:
                using ReturnType = decltype(std::declval<F>()(std::declval<T&>()));

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

            template <typename Front_prot_output_type, typename Adapter>
            struct output_trait {
                using type = std::conditional_t<
                    std::is_void_v<Adapter>,
                    Front_prot_output_type,
                    typename is_adaptor<Adapter, Front_prot_output_type>::result_type
                >;
            };

            namespace detail {
                // Check if T has prot_output_type
                template <typename T, typename = void>
                struct has_prot_output_type : std::false_type {};
                
                template <typename T>
                struct has_prot_output_type<T, std::void_t<typename T::prot_output_type>> : std::true_type {};
                
                //// Check if T has prot_input_type
                //template <typename T, typename = void>
                //struct has_prot_input_type : std::false_type {};
                //
                //template <typename T>
                //struct has_prot_input_type<T, std::void_t<typename T::prot_input_type>> : std::true_type {};
                
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
                    
                // Check if T has U operator<<(prot_input_type&) where U is convertible to io::future
                template <typename T, typename Front_prot_output_type, typename Adapter, typename = void>
                struct has_future_send_op : std::false_type {};
                
                template <typename T, typename Front_prot_output_type, typename Adapter>
				struct has_future_send_op<T, Front_prot_output_type, Adapter,
                                std::void_t<decltype(
                                    std::declval<io::future&>() = std::declval<T>().operator<<(
                                        std::declval<typename output_trait<Front_prot_output_type, Adapter>::type&>()))> >
                    : std::true_type {};
                    
                // Check if T has void operator<<(prot_input_type&)
                template <typename T, typename Front_prot_output_type, typename Adapter, typename = void>
                struct has_void_send_op : std::false_type {};
                
                template <typename T, typename Front_prot_output_type, typename Adapter>
				struct has_void_send_op<T, Front_prot_output_type, Adapter,
                                std::void_t<decltype(std::declval<T>().operator<<(
                                    std::declval<typename output_trait<Front_prot_output_type, Adapter>::type&>()))> >
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
            
            template <typename T, typename Front_prot_output_type, typename Adapter>
            struct is_input_prot {
                // is typename T a send protocol
                static constexpr bool value = 
                    (detail::has_future_send_op<T, Front_prot_output_type, Adapter>::value ||
                     detail::has_void_send_op<T, Front_prot_output_type, Adapter>::value);
                
                // await is true for future-returning operator<< and false for void-returning operator<<
                static constexpr bool await = detail::has_future_send_op<T, Front_prot_output_type, Adapter>::value;
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

            template <typename T1, typename T2, typename Adaptor>
            struct is_compatible_prot_pair {
                static constexpr bool value = 
                    is_output_prot_gen<T1>::value &&
                    is_input_prot<T2, typename is_output_prot_gen<T1>::prot_output_type, Adaptor>::value &&
                    (is_output_prot_gen<T1>::await || is_input_prot<T2, typename is_output_prot_gen<T1>::prot_output_type, Adaptor>::await);
            };

            template <typename T1, typename T2, typename Adaptor>
            inline constexpr bool is_compatible_prot_pair_v = is_compatible_prot_pair<T1, T2, Adaptor>::value;

            // Concept to check if a type is a valid error handler for pipeline
            template <typename T>
            concept PipelineErrorHandler =
                requires(T handler, int which, bool output_or_input, std::error_code ec) {
                    { handler(which, output_or_input, ec) } -> std::same_as<void>;
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

#include "internal/definitions.h"
    }
}