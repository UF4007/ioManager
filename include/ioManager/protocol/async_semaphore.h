#pragma once
#include "../ioManager.h"
namespace io {
    inline namespace IO_LIB_VERSION___ {
        // Async semaphore implementation - thread-safe counting semaphore with async wait
        // Similar to std::counting_semaphore but works with the io::async framework
        namespace async {
            struct semaphore {
                // Create a semaphore with the specified initial count
                inline semaphore(io::manager* mngr, size_t initial_count = 0) 
                    : ptr(std::make_shared<sem_base>()), this_executor(mngr) {
                    getPtr()->current_tokens = getPtr()->initial_tokens = initial_count;
                }

                // Acquire n tokens from the semaphore
                // Returns a future that completes when the tokens are acquired
                // or when the semaphore is closed
                inline io::async_future acquire(size_t n = 1) {
                    if (n == 0) {
                        return getResolvedFuture();
                    }

                    if (this->getPtr()->closed.test()) {
                        return getClosedFuture();
                    }

                    sem_base* base = this->getPtr();
                    
                    // Try to acquire spin lock
                    while (base->spinLock.test_and_set(std::memory_order_acquire)) {
                        // Spin until lock is acquired
                        if (base->closed.test()) {
                            return getClosedFuture();
                        }
                    }
                    
                    // Check if we have enough tokens available
                    if (base->current_tokens >= n) {
                        base->current_tokens -= n;
                        base->spinLock.clear(std::memory_order_release);
                        return getResolvedFuture();
                    }
                    
                    // Not enough tokens, add to waiting queue
                    io::async_future fut;
                    io::async_promise prom = this_executor->make_future(fut);
                    
                    // Create a waiter and add to queue
                    waiter w;
                    w.tokens_needed = n;
                    w.prom = std::move(prom);
                    base->waiters.push_back(std::move(w));
                    
                    base->spinLock.clear(std::memory_order_release);
                    return fut;
                }

                // Release n tokens back to the semaphore
                // This may wake up waiters if they can now acquire their tokens
                inline void release(size_t n = 1) {
                    if (n == 0) {
                        return;
                    }

                    if (this->getPtr()->closed.test()) {
                        return;
                    }

                    sem_base* base = this->getPtr();
                    
                    // Try to acquire spin lock
                    while (base->spinLock.test_and_set(std::memory_order_acquire)) {
                        // Spin until lock is acquired
                        if (base->closed.test()) {
                            return;
                        }
                    }
                    
                    // Add tokens back
                    base->current_tokens += n;
                    
                    // Try to satisfy waiters
                    auto it = base->waiters.begin();
                    std::vector<io::async_promise> to_resolve;
                    
                    while (it != base->waiters.end() && base->current_tokens > 0) {
                        if (it->tokens_needed <= base->current_tokens) {
                            // This waiter can be satisfied
                            base->current_tokens -= it->tokens_needed;
                            to_resolve.push_back(std::move(it->prom));
                            it = base->waiters.erase(it);
                        } else {
                            // Can't satisfy this waiter yet
                            ++it;
                        }
                    }
                    
                    base->spinLock.clear(std::memory_order_release);
                    
                    // Resolve promises outside of lock
                    for (auto& prom : to_resolve) {
                        prom.resolve();
                    }
                }

                // Try to acquire n tokens without waiting
                // Returns true if successful, false otherwise
                inline bool try_acquire(size_t n = 1) {
                    if (n == 0) {
                        return true;
                    }

                    if (this->getPtr()->closed.test()) {
                        return false;
                    }

                    sem_base* base = this->getPtr();
                    
                    // Try to acquire spin lock
                    while (base->spinLock.test_and_set(std::memory_order_acquire)) {
                        // Spin until lock is acquired
                        if (base->closed.test()) {
                            return false;
                        }
                    }
                    
                    // Check if we have enough tokens available
                    bool success = false;
                    if (base->current_tokens >= n) {
                        base->current_tokens -= n;
                        success = true;
                    }
                    
                    base->spinLock.clear(std::memory_order_release);
                    return success;
                }

                // Close the semaphore and wake up all waiters with error
                inline void close() {
                    this->getPtr()->close();
                }

                // Reset semaphore to initial state with initial_count tokens
                inline void reset() {
                    sem_base* base = this->getPtr();
                    
                    // Try to acquire spin lock
                    while (base->spinLock.test_and_set(std::memory_order_acquire));
                    
                    base->current_tokens = base->initial_tokens;
                    
                    // Clear all waiters
                    std::vector<io::async_promise> to_reject;
                    for (auto& w : base->waiters) {
                        to_reject.push_back(std::move(w.prom));
                    }
                    base->waiters.clear();
                    
                    base->spinLock.clear(std::memory_order_release);
                    
                    // Reject all waiters with operation aborted
                    for (auto& prom : to_reject) {
                        prom.reject(std::make_error_code(std::errc::operation_canceled));
                    }
                }

                // Get current available tokens (for debugging)
                inline size_t available() {
                    sem_base* base = this->getPtr();
                    
                    // Try to acquire spin lock
                    while (base->spinLock.test_and_set(std::memory_order_acquire)) {
                        // Spin until lock is acquired
                        if (base->closed.test()) {
                            return 0;
                        }
                    }
                    
                    size_t result = base->current_tokens;
                    base->spinLock.clear(std::memory_order_release);
                    return result;
                }

                // Get the number of waiters (for debugging)
                inline size_t waiting_count() {
                    sem_base* base = this->getPtr();
                    
                    // Try to acquire spin lock
                    while (base->spinLock.test_and_set(std::memory_order_acquire)) {
                        // Spin until lock is acquired
                        if (base->closed.test()) {
                            return 0;
                        }
                    }
                    
                    size_t result = base->waiters.size();
                    base->spinLock.clear(std::memory_order_release);
                    return result;
                }

                inline bool is_closed() {
                    return this->getPtr()->closed.test();
                }

                inline io::manager* getManager() { return this_executor; }
                inline void setManager(io::manager* mngr) { this_executor = mngr; }

            private:
                // Structure to track waiting acquirers
                struct waiter {
                    size_t tokens_needed;
                    io::async_promise prom;
                };

                // Core semaphore data structure
                struct sem_base {
                    std::vector<waiter> waiters;
                    size_t initial_tokens;
                    size_t current_tokens;
                    std::atomic_flag spinLock = ATOMIC_FLAG_INIT;
                    std::atomic_flag closed = ATOMIC_FLAG_INIT;

                    inline void close() {
                        // Avoid closing twice
                        if (closed.test_and_set()) {
                            return;
                        }
                        
                        // Acquire the lock
                        while (spinLock.test_and_set(std::memory_order_acquire));
                        
                        // Wake up all waiters with error
                        std::vector<io::async_promise> to_reject;
                        for (auto& w : waiters) {
                            to_reject.push_back(std::move(w.prom));
                        }
                        waiters.clear();
                        
                        spinLock.clear(std::memory_order_release);
                        
                        // Reject all waiters with operation aborted
                        for (auto& prom : to_reject) {
                            prom.reject(std::make_error_code(std::errc::operation_canceled));
                        }
                    }
                };

                // Return shared_ptr to the base semaphore structure
                inline sem_base* getPtr() {
                    return ptr.get();
                }

                // Helper method to get a resolved future
                inline io::async_future getResolvedFuture() {
                    io::async_future fut;
                    io::async_promise prom = this_executor->make_future(fut);
                    prom.resolve();
                    return fut;
                }

                // Helper method to get a closed future
                inline io::async_future getClosedFuture() {
                    io::async_future fut;
                    io::async_promise prom = this_executor->make_future(fut);
                    prom.reject(std::make_error_code(std::errc::operation_canceled));
                    return fut;
                }

                std::shared_ptr<sem_base> ptr;
                io::manager* this_executor;
            };
        }
    }
} 