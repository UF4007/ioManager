#pragma once
namespace io {
    inline namespace IO_LIB_VERSION___ {
		//async channel, transfers data between managers (threads).
		// Guaranteed the order of the elements only when SCSP.
		// MCMP safe, rapid CAS spining lock only.
        // Dynamic memory allocates inside.
		// Communication functions are Thread Safe.
        namespace async {
			template <typename T>
				requires std::is_move_constructible_v<T>
			struct chan
			{
                // The memory pointed to by the std::span will be moved into the channel immediately.
                // Inside, we use the move operator= instead of the copy.
                inline async_future operator<<(const std::span<T>& in) {
                    if (in.empty()) {
                        return getResolvedFuture();
                    }
                    
                    if (this->getPtr()->closed.test()) {
                        return getClosedFuture();
                    }
                    
                    // Allocate memory for segment and data
                    size_t dataSize = sizeof(segment) + sizeof(T) * in.size();
                    void* memory = ::operator new(dataSize);
                    segment* seg = new (memory) segment();
                    
                    // Initialize segment
                    seg->next = nullptr;
                    seg->length = in.size();
                    
                    // Create a promise for this operation
                    async_future fut;
                    async_promise prom = this_executor->make_future(fut);
                    
                    // Move construct T objects from span to segment
                    T* contentPtr = seg->getPtrContent();
                    std::uninitialized_copy(
                        std::make_move_iterator(in.begin()),
                        std::make_move_iterator(in.end()),
                        contentPtr
                    );
                    // Try to acquire spin lock and check closed state
                    chan_base* base = this->getPtr();
                    while (base->spinLock.test_and_set(std::memory_order_acquire)) {
                        // Spin until lock is acquired
                        if (base->closed.test()) {
                            // Channel is closed, clean up and return error
                            std::destroy_n(contentPtr, seg->length);
                            ::operator delete(memory);
                            return getClosedFuture();
                        }
                    }
                    
                    // Now we have the lock, add segment to the chain
                    if (base->send_end == nullptr) {
                        base->send_start = base->send_end = seg;
                    } else {
                        base->send_end->next = seg;
                        base->send_end = seg;
                    }
                    
                    // Update size
                    base->size += in.size();

                    // If we're under capacity, resolve immediately
                    if (base->size > base->capacity) {
                        seg->prom = std::move(prom);
                    }
                    
                    // If there are waiting receivers, wake up one
                    if (!base->recv_queue.empty()) {
                        async_promise recv_prom = std::move(base->recv_queue.front());
                        base->recv_queue.pop_front();
                        base->spinLock.clear(std::memory_order_release);
                        recv_prom.resolve();
                    } else {
                        base->spinLock.clear(std::memory_order_release);
                    }

                    prom.resolve();
                    
                    return fut;
                }

                // Listen until the channel is readable or closed.
                inline async_future listen() {
                    chan_base* base = this->getPtr();

                    if (this->getPtr()->closed.test()) {
                        return getClosedFuture();
                    }
                    
                    // Try to acquire lock
                    while (base->spinLock.test_and_set(std::memory_order_acquire)) {
                        // Spin until lock is acquired
                        if (base->closed.test()) {
                            return getClosedFuture();
                        }
                    }
                    
                    // Recheck with lock held
                    if (base->send_start != nullptr) {
                        base->spinLock.clear(std::memory_order_release);
                        return getResolvedFuture();
                    }
                    
                    // No data and not closed, add to waiting queue
                    async_future fut;
                    async_promise prom = this_executor->make_future(fut);
                    base->recv_queue.push_back(std::move(prom));
                    
                    base->spinLock.clear(std::memory_order_release);
                    return fut;
                }

                // Try to read specific size of datas from the channel.
				inline size_t accept(const std::span<T>& out) {
                    if (out.empty()) {
                        return 0;
                    }
                    
                    size_t totalRead = 0;
                    
                    // Outer loop, try to consume one segment each iteration
                    while (totalRead < out.size()) {
                        segment* current = nullptr;
                        size_t toRead = 0;
                        
                        // Acquire lock to get the first segment from the list
                        chan_base* base = this->getPtr();
                        while (base->spinLock.test_and_set(std::memory_order_acquire)) {
                            if (base->closed.test()) {
                                return totalRead; // Channel closed and no data
                            }
                        }
                        
                        // Check if there's data to read
                        if (base->send_start == nullptr) {
                            base->spinLock.clear(std::memory_order_release);
                            return totalRead; // No data available
                        }
                        
                        // Get the first segment, but don't remove it from the list yet
                        current = base->send_start;
                        
                        // Calculate how much data to read from this segment
                        // Considering both the remaining space in out and available data in segment
                        size_t availableInSegment = current->length - current->consumed;
                        toRead = std::min(availableInSegment, out.size() - totalRead);
                        
                        // Update channel size
                        base->size -= toRead;
                        
                        // Remove it from the list
                        base->send_start = current->next;
                        if (base->send_start == nullptr) {
                            base->send_end = nullptr;
                        }
                        
                        // Release lock
                        base->spinLock.clear(std::memory_order_release);
                        
                        // Process data outside of lock - allows other threads to access the channel while we process data
                        if (toRead > 0) {
                            T* contentPtr = current->getPtrContent();
                            
                            // Move data to output span
                            std::copy(
                                std::make_move_iterator(contentPtr),
                                std::make_move_iterator(contentPtr + toRead),
                                out.data() + totalRead
                            );
                            
                            // Destroy original objects after moving
                            std::destroy_n(contentPtr, toRead);
                            
                            totalRead += toRead;
                            
                            // If we fully consumed the segment, free memory and resolve promise
                            if (toRead == availableInSegment) {
                                current->prom.resolve();
                                ::operator delete(current);
                            }
                            else {
                                // Put the remaining segment return to the channel.
                                availableInSegment = current->length - current->consumed;
                                while (base->spinLock.test_and_set(std::memory_order_acquire)) {
                                    if (base->closed.test()) {
                                        contentPtr = current->getPtrContent();
                                        current->prom.resolve();
                                        std::destroy_n(contentPtr, availableInSegment);
                                        ::operator delete(current);
                                        return totalRead; // Channel closed and no data
                                    }
                                }
                                current->next = base->send_start;
                                base->send_start = current;
                                if (base->send_end == nullptr) {
                                    base->send_end = current;
                                }
                                base->spinLock.clear(std::memory_order_release);
                            }
                        }
                        
                        // If no more data was read, break the loop
                        if (toRead == 0) {
                            break;
                        }
                    }
                    
                    return totalRead;
				}

                inline bool isClosed() {
                    return this->getPtr()->closed.test();
                }
                inline bool isFull() {
                    return this->size() == this->capacity();
                }
                inline size_t size() {
                    chan_base* base = this->getPtr();
                    size_t result = 0;
                    
                    // Try to get spin lock
                    while (base->spinLock.test_and_set(std::memory_order_acquire)) {
                        // Spin until lock is acquired
                        if (base->closed.test()) {
                            return 0;
                        }
                    }
                    
                    result = base->size;
                    base->spinLock.clear(std::memory_order_release);
                    return result;
                }
                inline size_t capacity() {
                    return this->getPtr()->capacity;
                }
                //close, this will deconstruct all element in buffer, and resume all coroutines.
                inline void close() {
                    return this->getPtr()->close();
                }

                inline auto getManager() { return this_executor; }
                inline void setManager(io::manager* mngr) { this_executor = mngr; }
            private:
                struct segment {
                    async_promise prom;
                    segment* next;
                    size_t length;
                    size_t consumed = 0;
                    inline T* getPtrContent() { return (T*)(this + 1) + consumed; }
                    inline std::span<T> getSpan() { return std::span<T>{getPtrContent(), length - consumed}; }
                };
                struct chan_base {
                    std::deque<async_promise> recv_queue;   //locked zone.
                    segment* send_start = nullptr;          //locked zone.
                    segment* send_end = nullptr;            //locked zone.
                    size_t size = 0;                        //locked zone.
                    std::atomic_flag spinLock = ATOMIC_FLAG_INIT;

                    std::atomic_flag closed = ATOMIC_FLAG_INIT;

                    //TODO: memory pool (hive) of segment. Not now yet.

                    size_t capacity;

                    inline void close() {
                        // Avoid closing twice
                        if (closed.test_and_set()) {
                            return;
                        }
                        
                        // Acquire the lock
                        while (spinLock.test_and_set(std::memory_order_acquire));
                        
                        // Notify all waiting receivers
                        while (!recv_queue.empty()) {
                            async_promise prom = std::move(recv_queue.front());
                            recv_queue.pop_front();
                            prom.reject(std::error_code(1, chan_err::global()));
                        }
                        
                        // Clean up all segments but allow senders to resolve first
                        segment* current = send_start;
                        send_start = send_end = nullptr;
                        size = 0;
                        
                        // Release the lock
                        spinLock.clear(std::memory_order_release);
                        
                        // Destroy all segments
                        while (current != nullptr) {
                            segment* next = current->next;
                            
                            // Destruct all T objects in this segment
                            std::span<T> contentPtr = current->getSpan();
                            std::destroy(contentPtr.begin(), contentPtr.end());

                            current->prom.reject(std::error_code(1, chan_err::global()));
                            
                            // Free memory
                            ::operator delete(current);
                            current = next;
                        }
                    }
                    inline ~chan_base() {
                        close();
                    }
                };
                io::manager* this_executor;
                std::shared_ptr<chan_base> _base;    //we assert that this shared_ptr cannot be null.

                inline async_future getResolvedFuture() {
                    future ret;
                    promise prom = this_executor->make_future(ret);
                    prom.resolve();
                    return ret;
                }
                inline async_future getClosedFuture() {
                    future ret;
                    promise prom = this_executor->make_future(ret);
                    prom.reject(std::error_code(1, chan_err::global()));
                    return ret;
                }
                inline chan_base* getPtr() { return (chan_base*)_base.get(); }

            public:
                template <typename T_FSM>
                inline chan(fsm<T_FSM>& _fsm, size_t capacity, std::initializer_list<T> init_list = {}) :chan(_fsm.getManager(), capacity, init_list) {}
                inline chan(manager* executor, size_t capacity, std::initializer_list<T> init_list = {}) {
                    this_executor = executor;
                    _base = std::make_shared<chan_base>();
                    _base->capacity = capacity;
                    assert(init_list.size() <= capacity || !"make_chan ERROR: Too many initialize args!");
                    
                    // Initialize with values from init_list if any
                    if (init_list.size()) {
                        std::span<T> span((T*)(init_list.begin()), init_list.size());
                        this->operator<<(span);
                    }
                }

                inline chan(const chan& other, manager* async_executor) {
                    this_executor = async_executor;
                    _base = other._base;
                }
                inline chan(chan&& other, manager* async_executor) {
                    this_executor = async_executor;
                    _base = std::move(other._base);
                    other.this_executor = nullptr;
                }

                //operator= default
            };

            template <typename T>
                requires std::is_move_constructible_v<T>
            struct chan_r :chan<T> {
                template <typename ...Args>
                chan_r(Args&&... c) : chan<T>(std::forward<Args>(c)...) {}

                async_future operator<<(const std::span<T>& in) = delete;
            };

            template <typename T>
                requires std::is_move_constructible_v<T>
            struct chan_s :chan<T> {
                template <typename ...Args>
                chan_s(Args&&... c) : chan<T>(std::forward<Args>(c)...) {}

                async_future get_and_copy(const std::span<T>& out) = delete;
            };
        }

        namespace prot {
            /**
             * @brief Output protocol implementation for the channel.
             *
             * Provides an actual storage buffer for data output from the channel.
             *    While std::span is just a view without owning memory, chan_prot defines
             *    a fixed-size array (std::array) to store data, solving the buffer ownership
             *    problem in pipeline design.
             *
             * Controls batch size for data reading through the Out_buf_size template parameter,
             *    determining how many items will be read from the channel at once.
             *
             * @tparam Out_buf_size Size of the output buffer, controls batch size per read operation.
             *                      Must be greater than 0.
             * @tparam Is_Async If true, the protocol will be asynchronous between threads.
             *
             */
            template <typename T, size_t Out_buf_size = 1, bool Is_Async = false>
                requires (Out_buf_size != 0)
            struct chan : io::chan<T> {
                // Replace variadic template constructor with specific constructors
                chan(const io::chan<T>& c) : io::chan<T>(c) {}
                
                chan(io::chan<T>&& c) : io::chan<T>(std::move(c)) {}

                // For explicit buffer size specification
                template <size_t BufferSize = Out_buf_size>
                    requires (BufferSize != 0)
                chan(const io::chan<T>& c, std::integral_constant<size_t, BufferSize>) : io::chan<T>(c) {
                    static_assert(BufferSize == Out_buf_size, "Buffer size mismatch in template parameter");
                }

                template <size_t BufferSize = Out_buf_size>
                    requires (BufferSize != 0)
                chan(io::chan<T>&& c, std::integral_constant<size_t, BufferSize>) : io::chan<T>(std::move(c)) {
                    static_assert(BufferSize == Out_buf_size, "Buffer size mismatch in template parameter");
                }

                using prot_output_type =
                    std::conditional_t<
                    Out_buf_size == 1,
                    T,
                    std::array<T, Out_buf_size>
                    >;

                // Output protocol implementation
                inline void operator>>(future_with<prot_output_type>& out_future) {
                    if constexpr (Out_buf_size == 1)
                    {
                        out_future = this->get_and_copy(std::span(&out_future.data, 1));
                    }
                    else
                    {
                        out_future = this->get_and_copy(std::span(out_future.data.data(), Out_buf_size));
                    }
                }

            private:
                [[no_unique_address]] std::conditional_t<
                    Out_buf_size == 1,
                    T,
                    std::monostate
                > temp;

            public:
                // Input protocol implementation
                inline future operator<<(T& in) requires (Out_buf_size == 1) {
                    temp = std::move(in);
                    return this->io::chan<T>::operator<<(std::span(&temp, 1));
                }
            };

            // The struct itself is not thread safe at all, only the communication is thread safe.
            template <typename T, size_t Out_buf_size>
                requires (Out_buf_size != 0)
            struct chan<T, Out_buf_size, true> : io::async::chan<T> {
                using prot_output_type =
                    std::conditional_t<
                    Out_buf_size == 1,
                    T,
                    std::array<T, Out_buf_size>
                    >;

            private:
                struct rec_fsm_comm {
                    io::future receive_activate;
                    io::promise<prot_output_type> out_promise;
                };
                fsm_handle<rec_fsm_comm> receive_fsm;

                void init_async_fsm() {
                    io::manager* mngr = this->getManager();
                    receive_fsm = mngr->spawn_later([](io::async::chan<T> ch)->fsm_func<rec_fsm_comm> {
                        io::fsm<rec_fsm_comm>& fsm_ = co_await io::get_fsm;
                        while (1)
                        {
                            co_await fsm_->receive_activate;

                            //working
                            if constexpr (Out_buf_size == 1)
                            {
                                do {
                                    auto listen_fut = ch.listen();
                                    co_await listen_fut;

                                    if (listen_fut.getErr()) {
                                        fsm_->out_promise.reject_later(listen_fut.getErr());
                                        break;
                                    }

                                    T* data_ptr = fsm_->out_promise.data();
                                    if (data_ptr) {
                                        std::span<T> span(data_ptr, 1);
                                        size_t read = ch.accept(span);

                                        if (read == 1) {
                                            fsm_->out_promise.resolve_later();
                                        }
                                        else {
                                            continue;
                                        }
                                    }
                                } while (0);
                            }
                            else
                            {
                                size_t total_read = 0;
                                size_t remaining = Out_buf_size;
                                do {
                                    auto listen_fut = ch.listen();
                                    co_await listen_fut;

                                    if (listen_fut.getErr()) {
                                        fsm_->out_promise.reject_later(listen_fut.getErr());
                                        break;
                                    }

                                    T* data_ptr = fsm_->out_promise.data();
                                    if (data_ptr) {
                                        std::span<T> span(data_ptr + total_read, remaining);
                                        size_t read = ch.accept(span);
                                        total_read += read;
                                        remaining -= read;
                                        if (total_read >= Out_buf_size) {
                                            fsm_->out_promise.resolve_later();
                                        }
                                        else {
                                            continue;
                                        }
                                    }
                                } while (0);
                            }
                            fsm_.make_future(fsm_->receive_activate);
                        }
                    }(*this));
                    mngr->make_future(receive_fsm->receive_activate);
                }

            public:
                // Replace variadic template constructor with specific constructors
                chan(const io::async::chan<T>& c) : io::async::chan<T>(c) {
                    init_async_fsm();
                }

                chan(io::async::chan<T>&& c) : io::async::chan<T>(std::move(c)) {
                    init_async_fsm();
                }

                // For explicit buffer size specification
                template <size_t BufferSize = Out_buf_size>
                    requires (BufferSize != 0)
                chan(const io::async::chan<T>& c, std::integral_constant<size_t, BufferSize>) : io::async::chan<T>(c) {
                    static_assert(BufferSize == Out_buf_size, "Buffer size mismatch in template parameter");
                    init_async_fsm();
                }

                template <size_t BufferSize = Out_buf_size>
                    requires (BufferSize != 0)
                chan(io::async::chan<T>&& c, std::integral_constant<size_t, BufferSize>) : io::async::chan<T>(std::move(c)) {
                    static_assert(BufferSize == Out_buf_size, "Buffer size mismatch in template parameter");
                    init_async_fsm();
                }

                // Output protocol implementation
                inline void operator>>(future_with<prot_output_type>& out_future) {
                    io::manager* mngr = this->getManager();
                    receive_fsm->receive_activate.getPromise().resolve_later();
                    receive_fsm->out_promise = mngr->make_future(out_future, &out_future.data);
                }

                // Input protocol implementation
                inline future operator<<(T& in) requires (Out_buf_size == 1) {
                    return this->io::async::chan<T>::operator<<(std::span(&in, 1));
                }

                // Input protocol implementation
                inline future operator<<(const std::span<T>& in) {
                    return (future)this->operator<<(in);
                }
            };

            template <typename T>
            chan(const io::async::chan<T>&) -> chan<T, 1, true>;

            template <typename T>
            chan(io::async::chan<T>&&) -> chan<T, 1, true>;

            template <typename T, size_t BufferSize>
            chan(const io::async::chan<T>&, std::integral_constant<size_t, BufferSize>) -> chan<T, BufferSize, true>;

            template <typename T, size_t BufferSize>
            chan(io::async::chan<T>&&, std::integral_constant<size_t, BufferSize>) -> chan<T, BufferSize, true>;

            template <typename T>
            chan(const io::chan<T>&) -> chan<T, 1, false>;

            template <typename T>
            chan(io::chan<T>&&) -> chan<T, 1, false>;

            template <typename T, size_t BufferSize>
            chan(const io::chan<T>&, std::integral_constant<size_t, BufferSize>) -> chan<T, BufferSize, false>;

            template <typename T, size_t BufferSize>
            chan(io::chan<T>&&, std::integral_constant<size_t, BufferSize>) -> chan<T, BufferSize, false>;
        };
    };
};