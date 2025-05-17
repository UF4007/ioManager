#pragma once
#include "ioManager.h"

namespace io {
    inline namespace IO_LIB_VERSION___ {

        // Repeatly Timer and Counter
        // Not thread safe
        namespace timer {
            struct counter {
                size_t stop_count;
                size_t count_sum = 0;
                inline counter(size_t stop_count) : stop_count(stop_count) {}
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
                inline void reset(size_t count_sum_ = 0) { count_sum = count_sum_; }
                inline bool isReach() { return count_sum >= stop_count; }
                template <typename T_FSM> inline future& onReach(T_FSM& _fsm) {
                    _fsm.make_future(_on_stop);
                    return _on_stop;
                }

            private:
                future _on_stop;
            };
            // compensated countdown timer, when it being await, current time will be
            // compared with (start_timepoint + count_sum * count_duration), rather
            // (previous_timepoint + count_duration)
            struct down : private counter {
                inline down(size_t stop_count) : counter(stop_count) {}
                inline void start(std::chrono::steady_clock::duration _duration) {
                    this->reset();
                    duration = _duration;
                    return;
                }
                template <typename T_FSM> inline clock& await_tm(T_FSM& _fsm) {
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
                inline void reset(size_t count_sum_ = 0) {
                    counter::reset(count_sum_);
                    start_tp = std::chrono::steady_clock::now();
                }
                inline std::chrono::steady_clock::duration getDuration() const {
                    return duration;
                }
                inline bool isReach() { return ((counter*)this)->isReach(); }
                template <typename T_FSM> inline future& onReach(T_FSM& _fsm) {
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
                inline auto elapsed() {
                    return std::chrono::steady_clock::now() - previous_tp;
                }
                inline void reset() { previous_tp = {}; }

            private:
                std::chrono::steady_clock::time_point previous_tp;
            };
        }; // namespace timer
    } // namespace IO_LIB_VERSION___
} // namespace io
