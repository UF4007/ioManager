#pragma once
#include "../ioManager.h"

namespace io {
    inline namespace IO_LIB_VERSION___ {
        struct chan_err : public std::error_category {
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
                thread_local chan_err instance;
                return instance;
            }
        };

        //channel, use it within a single manager(thread) 
		// Guaranteed the order of the elements.
        // Implements by circular buffer. Not any dynamic memory allocates inside.
        // Not Thread safe.
        template <typename T>
            requires std::is_move_constructible_v<T>
        struct chan {
            // Input protocol implemention. 
            // The memory's lifetime that is pointed by the std::span must be longer than the future returned.
            // Inside, we use the move operator= instead of the copy.
            inline future operator<<(const std::span<T>& in) {
                std::span<T> data_in = in;
                chan_base* base = this->getPtr();
                if (isClosed()) return getClosedFuture(base);
                
                // Try to push data directly into buffer
                if (base->status != chan_base::status_t::send_block) {
                    // First try to resolve any waiting receivers
                    while (base->waiting.size() && data_in.size() > 0) {
                        auto& [prom, span] = *(base->waiting.begin());
                        if (prom.valid() == false) {
                            base->waiting.erase(base->waiting.begin());
                            continue;
                        }
                        
                        // Try to satisfy the waiting receiver
                        if (data_in.size() >= span.size()) {
                            // We have enough data to fully satisfy this receiver
                            std::copy(
                                std::make_move_iterator(data_in.begin()),
                                std::make_move_iterator(data_in.begin() + span.size()),
                                span.begin()
                            );
                            data_in = data_in.subspan(span.size());
                            prom.resolve_later();
                            base->waiting.erase(base->waiting.begin());
                            if (data_in.size() == 0)
                                return getResolvedFuture(base);
                        } else {
                            // Partial data - receiver still needs more
                            std::copy(
                                std::make_move_iterator(data_in.begin()),
                                std::make_move_iterator(data_in.end()),
                                span.begin()
                            );
                            span = span.subspan(data_in.size());
                            data_in = {};
                            return getResolvedFuture(base);
                        }
                    }
                    
                    // If we have data left, push it to the buffer
                    size_t original_size = data_in.size();
                    size_t pushed = base->push_from_span(data_in);
                    if (pushed == original_size) {
                        // All data pushed successfully
                        // or empty
                        base->status = chan_base::status_t::normal;
                        return getResolvedFuture(base);
                    }
                } else {
                    base->erase_invalid();
                }

                // We still have data left and couldn't process all of it - need to block
                base->status = chan_base::status_t::send_block;
                future ret;
                promise<> prom = base->mngr->make_future(ret);
                base->waiting.emplace_back(std::move(prom), data_in);
                return ret;
            }

            // Future will not resolve until the span gets enough data.
            // The memory's lifetime that is pointed by the std::span must be longer than the future returned.
            inline future get_and_copy(const std::span<T>& out) {
                std::span<T> data_out = out;
                chan_base* base = this->getPtr();
                if (isClosed()) return getClosedFuture(base);
                
                // First try to get data from buffer
                if (base->status != chan_base::status_t::recv_block) {
                    // Try to pop data from buffer to output span
                    if (data_out.size() > 0) {
                        base->pop_to_span(data_out);
                    }
                    
                    // If we still need data and there are waiting senders,
                    // process them in order
                    while (base->waiting.size() && data_out.size() > 0) {
                        auto& [prom, span] = *(base->waiting.begin());
                        if (!prom.valid()) {
                            base->waiting.erase(base->waiting.begin());
                            continue;
                        }

                        // Process this sender's data
                        if (span.size() <= data_out.size()) {
                            // We can take all of this sender's data
                            std::copy(
                                std::make_move_iterator(span.begin()),
                                std::make_move_iterator(span.end()),
                                data_out.begin()
                            );
                            data_out = data_out.subspan(span.size());
                            prom.resolve_later();
                            base->waiting.erase(base->waiting.begin());

                            // If we got all we needed, we're done
                            if (data_out.size() == 0) {
                                break;
                            }
                        }
                        else {
                            // We can only take part of this sender's data
                            std::copy(
                                std::make_move_iterator(span.begin()),
                                std::make_move_iterator(span.begin() + data_out.size()),
                                data_out.begin()
                            );
                            span = span.subspan(data_out.size());
                            data_out = {};

                            // We're now full, but this sender still has data
                            // Keep the sender waiting and return
                            break;
                        }
                    }

                    // Process any waiting senders to put their data into the buffer
                    while (base->waiting.size()) {
                        auto& [prom, span] = *(base->waiting.begin());
                        if (!prom.valid()) {
                            base->waiting.erase(base->waiting.begin());
                            continue;
                        }

                        size_t original_span_size = span.size();
                        size_t pushed = base->push_from_span(span);

                        if (pushed == original_span_size) {
                            // All data pushed to buffer
                            prom.resolve_later();
                            base->waiting.erase(base->waiting.begin());
                        }
                        else {
                            // Buffer is full, can't push any more
                            break;
                        }
                    }

                    // Update status based on waiting queue
                    if (base->waiting.size() == 0) {
                        base->status = chan_base::status_t::normal;
                    }
                }
                
                // If there's still data we need, we have to block
                if (data_out.size() > 0) {
                    base->status = chan_base::status_t::recv_block;
                    future ret;
                    promise<> prom = base->mngr->make_future(ret);
                    base->waiting.emplace_back(std::move(prom), data_out);
                    return ret;
                }
                
                return getResolvedFuture(base);
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
                    return (base->capacity - base->read_pos) + base->write_pos;
            }
            inline size_t capacity() {
                return this->getPtr()->capacity;
            }
            //close, this will deconstruct all element in buffer, and resume all coroutines.
            inline void close() {
                return this->getPtr()->close();
            }

            template <typename T_FSM>
            inline chan(fsm<T_FSM>& _fsm, size_t capacity, std::initializer_list<T> init_list = {}) :chan(_fsm.getManager(), capacity, init_list) {}
            inline chan(manager* mngr, size_t capacity, std::initializer_list<T> init_list = {}) {
                _base = std::shared_ptr<void>(::operator new(sizeof(chan_base) + capacity * sizeof(T)), chan_base::deleter);
                assert(init_list.size() <= capacity || !"make_chan ERROR: Too many initialize args!");
                chan_base* base = getPtr();
                new(base)chan_base();
                base->mngr = mngr;
                if (init_list.size() == 0)
                {
                    base->status = chan_base::status_t::recv_block;
                }
                else if (init_list.size() == capacity)
                {
                    base->status = chan_base::status_t::send_block;
                }
                else
                {
                    base->status = chan_base::status_t::normal;
                }
                base->is_full = (init_list.size() == capacity);
                base->capacity = capacity;
                base->write_pos = init_list.size();
                T* buffer = getPtrContent();
                std::uninitialized_copy(
                    init_list.begin(),
                    init_list.end(),
                    buffer
                );
            }
         
        private:
            std::shared_ptr<void> _base;    //type erasure ringbuf. we assert that this shared_ptr cannot be null.
            struct chan_base {
                struct waiting_t {
                    promise<> prom;
                    std::span<T> span;
                };
                std::deque<waiting_t> waiting;
                manager* mngr;
                enum class status_t :char {
                    normal = 0,
                    send_block = 1,
                    recv_block = 2
                }status;
                bool closed = false;

                char align[2];

                bool is_full;
                size_t capacity;
                size_t write_pos = 0;
                size_t read_pos = 0;
                inline void erase_invalid() {
                    if (waiting.size() == 0)
                        return;
                    for (auto iter = waiting.begin(); iter != waiting.end(); iter++)
                    {
                        auto& [prom, span] = *iter;
                        if (prom.valid() == false)
                        {
                            iter = waiting.erase(iter);
                            if (iter == waiting.end())
                                break;
                        }
                    }
                }

                // Push data from input span to chan_base buffer
                // Modifies the input span to reflect the remaining data
                inline size_t push_from_span(std::span<T>& data_in) {
                    if (this->is_full || data_in.size() == 0) {
                        return 0;
                    }
                    
                    size_t total_pushed = 0;
                    
                    // First chunk
                    std::span<T> span_buf = this->get_in(data_in.size());
                    if (span_buf.size() > 0) {
                        std::uninitialized_copy(
                            std::make_move_iterator(data_in.begin()),
                            std::make_move_iterator(data_in.begin() + span_buf.size()),
                            span_buf.begin()
                        );
                        
                        total_pushed += span_buf.size();
                        data_in = data_in.subspan(span_buf.size());
                        
                        // If there's still data and space, try a second chunk
                        // (might be needed due to ring buffer wrap-around)
                        if (data_in.size() > 0 && !this->is_full) {
                            span_buf = this->get_in(data_in.size());
                            if (span_buf.size() > 0) {
                                std::uninitialized_copy(
                                    std::make_move_iterator(data_in.begin()),
                                    std::make_move_iterator(data_in.begin() + span_buf.size()),
                                    span_buf.begin()
                                );
                                
                                total_pushed += span_buf.size();
                                data_in = data_in.subspan(span_buf.size());
                            }
                        }
                    }
                    
                    return total_pushed;
                }
                
                // Pop data from chan_base buffer to output span
                // Modifies the input span to reflect the remaining space
                inline size_t pop_to_span(std::span<T>& where_copy) {
                    if ((this->write_pos == this->read_pos && !this->is_full) || where_copy.size() == 0) {
                        return 0;
                    }

                    size_t total_popped = 0;
                    
                    // First chunk
                    std::span<T> span_buf = this->get_out(where_copy.size());
                    if (span_buf.size() > 0) {
                        // Copy elements from buffer to the output span
                        std::copy(
                            std::make_move_iterator(span_buf.begin()),
                            std::make_move_iterator(span_buf.end()),
                            where_copy.begin()
                        );
                        
                        // Destroy the original objects after moving
                        std::destroy(span_buf.begin(), span_buf.end());
                        
                        total_popped += span_buf.size();
                        where_copy = where_copy.subspan(span_buf.size());
                        
                        // If there's still space and data, try a second chunk
                        // (might be needed due to ring buffer wrap-around)
                        if (where_copy.size() > 0 && (this->write_pos != this->read_pos || this->is_full)) {
                            span_buf = this->get_out(where_copy.size());
                            if (span_buf.size() > 0) {
                                // Copy elements from buffer to the output span
                                std::copy(
                                    std::make_move_iterator(span_buf.begin()),
                                    std::make_move_iterator(span_buf.end()),
                                    where_copy.begin()
                                );
                                
                                // Destroy the original objects after moving
                                std::destroy(span_buf.begin(), span_buf.end());
                                
                                total_popped += span_buf.size();
                                where_copy = where_copy.subspan(span_buf.size());
                            }
                        }
                    }
                    
                    return total_popped;
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
                    if (this->closed)
                        return;
                    this->closed = true;

                    T* buffer = (T*)(this + 1);
                    
                    // Calculate the number of elements that need to be destroyed
                    size_t elements_to_destroy = 0;
                    if (this->is_full) {
                        // Buffer is completely full
                        elements_to_destroy = capacity;
                    } else if (write_pos >= read_pos) {
                        // Elements are in a continuous segment
                        elements_to_destroy = write_pos - read_pos;
                    } else {
                        // Elements wrap around the buffer
                        elements_to_destroy = (capacity - read_pos) + write_pos;
                    }
                    
                    // Destroy only the constructed elements
                    if (elements_to_destroy > 0) {
                        if (write_pos > read_pos || (write_pos == read_pos && is_full)) {
                            // Continuous segment or full buffer
                            std::destroy_n(buffer + read_pos, elements_to_destroy);
                        } else {
                            // Wrapped around - need to destroy in two parts
                            std::destroy_n(buffer + read_pos, capacity - read_pos);
                            std::destroy_n(buffer, write_pos);
                        }
                    }
                    
                    // Reject any waiting promises
                    while (waiting.size())
                    {
                        auto& [prom, span] = *(waiting.begin());
                        prom.reject_later(std::error_code(1, chan_err::global()));
                        waiting.erase(waiting.begin());
                    }
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
                prom.reject_later(std::error_code(1, chan_err::global()));
                return ret;
            }
            inline chan_base* getPtr() { return (chan_base*)_base.get(); }
            inline T* getPtrContent() { return (T*)((chan_base*)_base.get() + 1); }
        };

        template <typename T>
            requires std::is_move_constructible_v<T>
        struct chan_r :chan<T> {
            chan_r(const chan<T>& c) : chan<T>(c) {}
            chan_r(chan<T>&& c) : chan<T>(std::move(c)) {}
            
            future operator<<(const std::span<T>& in) = delete;
        };

        template <typename T>
            requires std::is_move_constructible_v<T>
        struct chan_s :chan<T> {
            chan_s(const chan<T>& c) : chan<T>(c) {}
            chan_s(chan<T>&& c) : chan<T>(std::move(c)) {}
            
            future get_and_copy(const std::span<T>& out) = delete;
        };
    };
};