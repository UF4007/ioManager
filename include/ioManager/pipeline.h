#pragma once
#include "ioManager.h"

namespace io {
    inline namespace IO_LIB_VERSION___ {

        template <typename Front = void, typename Rear = void, typename Adaptor = void>
        struct pipeline {
            __IO_INTERNAL_HEADER_PERMISSION;
            using Rear_t = Rear;
            using Front_t = Front;
            using Adaptor_t = Adaptor;

            inline decltype(auto) start()&& {
                return pipeline_started<std::remove_reference_t<decltype(*this)>, false,
                    std::monostate>(std::move(*this));
            }

            template <typename ErrorHandler>
                requires trait::PipelineErrorHandler<ErrorHandler>
            inline decltype(auto) start(ErrorHandler&& e)&& {
                return pipeline_started<std::remove_reference_t<decltype(*this)>, false,
                    ErrorHandler>(std::move(*this),
                        std::forward<ErrorHandler>(e));
            }

            template <typename T_FSM> inline decltype(auto) spawn(T_FSM& _fsm)&& {
                pipeline_started<std::remove_reference_t<decltype(*this)>, true,
                    std::monostate>
                    ret = _fsm.spawn_now([](decltype(*this) t) -> fsm_func<void> {
                    pipeline_started<std::remove_reference_t<decltype(*this)>, false,
                    std::monostate>
                    pipeline_s(std::move(t));
                while (1) {
                    pipeline_s <= co_await +pipeline_s;
                }
                        }(*this));
                return ret;
            }

            template <typename T_FSM, typename ErrorHandler>
                requires trait::PipelineErrorHandler<ErrorHandler>
            inline decltype(auto) spawn(T_FSM& _fsm, ErrorHandler&& e)&& {
                pipeline_started<std::remove_reference_t<decltype(*this)>, true,
                    std::monostate>
                    ret = _fsm.spawn_now(
                        [](decltype(*this) t, ErrorHandler&& e) -> fsm_func<void> {
                            pipeline_started<std::remove_reference_t<decltype(*this)>, false,
                            ErrorHandler>
                            pipeline_s(std::move(t), std::forward<ErrorHandler>(e));
                while (1) {
                    pipeline_s <= co_await +pipeline_s;
                }
                        }(*this, std::forward<ErrorHandler>(e)));
                return ret;
            }

            // rear
            template <typename T>
                requires(trait::is_output_prot<std::remove_reference_t<Rear>>::value&&
            trait::is_input_prot<
                std::remove_reference_t<T>,
                typename std::remove_reference_t<Rear>::prot_output_type,
                void>::value&&
                trait::is_compatible_prot_pair_v<std::remove_reference_t<Rear>,
                std::remove_reference_t<T>, void>)
                inline decltype(auto) operator>>(T&& rear)&& {
                return pipeline<std::remove_reference_t<decltype(*this)>,
                    std::conditional_t<
                    std::is_lvalue_reference_v<T>,
                    std::add_lvalue_reference_t<std::remove_reference_t<T>>,
                    std::remove_reference_t<T>>,
                    void>(std::move(*this), std::forward<T>(rear));
            }

            // adaptor
            template <typename T>
                requires(
            trait::is_output_prot<std::remove_reference_t<Rear>>::value&&
                trait::is_adaptor<
                T, typename std::remove_reference_t<Rear>::prot_output_type>::value)
                inline decltype(auto) operator>>(T&& adaptor)&& {
                return pipeline_constructor<
                    std::remove_reference_t<decltype(*this)>, void,
                    std::conditional_t<
                    std::is_lvalue_reference_v<T>,
                    std::add_lvalue_reference_t<std::remove_reference_t<T>>,
                    std::remove_reference_t<T>>>(std::move(*this),
                        std::forward<T>(adaptor));
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
            pipeline& operator=(pipeline&&) = default;

        private:
            inline decltype(auto) awaitable() {
                std::array<future*, pair_sum()> futures;
                size_t index = 0;
                await_get(futures, index);
                if constexpr (pair_sum() == 1) {
                    future& ref = *futures[0];
                    return future::race_index(ref);
                }
                else {
                    return std::apply([](auto &&...args) { return future::race_index(*args...); },
                        futures);
                }
            }

            template <typename ErrorHandler>
            inline void process(int which, ErrorHandler errorHandler) {
                if (which == this->pair_sum() - 1) {
                    if constexpr (trait::is_output_prot_gen<
                        std::remove_reference_t<Front>>::await &&
                        trait::is_input_prot<
                        std::remove_reference_t<Rear>,
                        typename trait::is_output_prot_gen<
                        std::remove_reference_t<Front>>::prot_output_type,
                        Adaptor>::await) {
                        if (turn == 1) {
                            if (front_future.getErr()) {
                                if constexpr (std::is_same_v<ErrorHandler, std::monostate> ==
                                    false) {
                                    errorHandler(which, true, front_future.getErr());
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
                                        if constexpr (trait::is_pipeline_v<
                                            std::remove_reference_t<Front>>) {
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
                            if (rear_future.getErr()) {
                                if constexpr (std::is_same_v<ErrorHandler, std::monostate> ==
                                    false) {
                                    errorHandler(which, false, rear_future.getErr());
                                }
                            }
                            turn = 0;
                        }
                        else {
                            IO_ASSERT(false, "pipeline ERROR: unexcepted turn clock!");
                        }
                    }
                    else if constexpr (trait::is_output_prot_gen<
                        std::remove_reference_t<Front>>::await) {
                        if (turn == 1) {
                            if (front_future.getErr()) {
                                if constexpr (std::is_same_v<ErrorHandler, std::monostate> ==
                                    false) {
                                    errorHandler(which, true, front_future.getErr());
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
                            IO_ASSERT(false, "pipeline ERROR: unexcepted turn clock!");
                        }
                    }
                    else if constexpr (trait::is_input_prot<
                        std::remove_reference_t<Rear>,
                        typename trait::is_output_prot_gen<
                        std::remove_reference_t<Front>>::
                        prot_output_type,
                        Adaptor>::await) {
                        if (turn == 3) {
                            if (rear_future.getErr()) {
                                if constexpr (std::is_same_v<ErrorHandler, std::monostate> ==
                                    false) {
                                    errorHandler(which, false, rear_future.getErr());
                                }
                            }
                            turn = 2;
                        }
                        else {
                            IO_ASSERT(false, "pipeline ERROR: unexcepted turn clock!");
                        }
                    }
                }
                else {
                    if constexpr (trait::is_pipeline_v<std::remove_reference_t<Front>>) {
                        front.process(which, errorHandler);
                    }
                    else {
                        IO_ASSERT(false, "pipeline ERROR: unexcepted pipeline num!");
                    }
                }
            }

            template <size_t N>
            inline void await_get(std::array<future*, N>& futures, size_t& index) {
                if constexpr (trait::is_pipeline_v<std::remove_reference_t<Front>>) {
                    front.await_get(futures, index);
                }
                if constexpr (trait::is_output_prot_gen<
                    std::remove_reference_t<Front>>::await &&
                    trait::is_input_prot<
                    std::remove_reference_t<Rear>,
                    typename trait::is_output_prot_gen<
                    std::remove_reference_t<Front>>::prot_output_type,
                    Adaptor>::await) {
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
                        IO_ASSERT(false, "pipeline ERROR: unexcepted turn clock!");
                    }
                }
                else if constexpr (trait::is_output_prot_gen<
                    std::remove_reference_t<Front>>::await) {
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
                    else {
                        IO_ASSERT(false, "pipeline ERROR: unexcepted turn clock!");
                    }
                }
                else if constexpr (trait::is_input_prot<
                    std::remove_reference_t<Rear>,
                    typename trait::is_output_prot_gen<
                    std::remove_reference_t<Front>>::
                    prot_output_type,
                    Adaptor>::await) {
                    if (turn == 0 || turn == 2) {
                        if constexpr (!std::is_void_v<Adaptor>) {
                            bool adapted = false;
                            while (!adapted) {
                                if constexpr (trait::is_pipeline_v<
                                    std::remove_reference_t<Front>>) {
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
                        IO_ASSERT(false, "pipeline ERROR: unexcepted turn clock!");
                    }
                }
                else {
                    static_assert(trait::is_input_prot<
                        std::remove_reference_t<Rear>,
                        typename trait::is_output_prot_gen<
                        std::remove_reference_t<Front>>::prot_output_type,
                        Adaptor>::await,
                        "pipeline ERROR: Two Direct protocols (Direct Output "
                        "Protocol and Direct Input Protocol) cannot be connected "
                        "to each other in a pipeline segment!");
                }
            }

            // Constructor for pipeline with front and rear protocols (no adaptor)
            template <typename F, typename R>
            inline pipeline(F&& f, R&& r)
                : front(std::forward<F>(f)), rear(std::forward<R>(r)) {
            }

            // inline pipeline(Front&& f, Rear&& r)
            //     : front(std::forward<Front>(f))
            //     , rear(std::forward<Rear>(r)) {
            // }

            // Constructor for pipeline with front, rear and adaptor
            template <typename F, typename R, typename U = Adaptor>
            inline pipeline(F&& f, R&& r, std::enable_if<!std::is_void_v<U>, U>::type&& a)
                : front(std::forward<F>(f)), rear(std::forward<R>(r)),
                adaptor(std::forward<U>(a)) {
            }

            template <typename F, typename R, typename U = Adaptor>
            inline pipeline(F&& f, R&& r, int a)
                : front(std::forward<F>(f)), rear(std::forward<R>(r)) {
            }

            Front front;
            Rear rear;
            [[no_unique_address]] std::conditional_t<std::is_void_v<Adaptor>,
                std::monostate, Adaptor> adaptor;
            std::conditional_t<
                trait::is_output_prot_gen<std::remove_reference_t<Front>>::await,
                trait::detail::movable_future_with<typename trait::is_output_prot_gen<
                std::remove_reference_t<Front>>::prot_output_type>,
                typename trait::is_output_prot_gen<
                std::remove_reference_t<Front>>::prot_output_type>
                front_future;
            [[no_unique_address]] std::conditional_t<
                trait::is_input_prot<
                std::remove_reference_t<Rear>,
                typename trait::is_output_prot_gen<
                std::remove_reference_t<Front>>::prot_output_type,
                Adaptor>::await,
                future, std::monostate> rear_future;
            int turn = 0; // 0 front before operator<<,1 front after operator<<, 2 rear
            // before operator>>, 3 rear after operator>>
        };

        template <typename Front, typename Rear, typename Adaptor>
        struct pipeline_constructor {
            __IO_INTERNAL_HEADER_PERMISSION;
            using Rear_t = Rear;
            using Front_t = Front;
            using Adaptor_t = Adaptor;
            // Continue construction with adaptor
            template <typename T>
                requires(
            std::is_void_v<Rear>&& std::is_void_v<Adaptor>&&
                trait::is_adaptor<
                T, typename trait::is_output_prot_gen<
                std::remove_reference_t<Front>>::prot_output_type>::value)
                inline decltype(auto) operator>>(T&& adaptor)&& {
                return pipeline_constructor<
                    Front, void,
                    std::conditional_t<
                    std::is_lvalue_reference_v<T>,
                    std::add_lvalue_reference_t<std::remove_reference_t<T>>,
                    std::remove_reference_t<T>>>(std::forward<Front>(front),
                        std::forward<T>(adaptor));
            }

            // Create final pipeline with input protocol
            template <typename T>
                requires(
            std::is_void_v<Rear>&& std::is_void_v<Adaptor>&&
                trait::is_input_prot<
                typename std::remove_reference_t<T>,
                typename trait::is_output_prot_gen<
                std::remove_reference_t<Front>>::prot_output_type,
                Adaptor>::value&&
                trait::is_compatible_prot_pair_v<std::remove_reference_t<Front>,
                std::remove_reference_t<T>, Adaptor>)
                inline decltype(auto) operator>>(T&& rear)&& {
                return pipeline<Front,
                    std::conditional_t<
                    std::is_lvalue_reference_v<T>,
                    std::add_lvalue_reference_t<std::remove_reference_t<T>>,
                    std::remove_reference_t<T>>,
                    Adaptor>(std::forward<Front>(front), std::forward<T>(rear));
            }

            // Create final pipeline with input protocol and adaptor
            template <typename T>
                requires(
            std::is_void_v<Rear> && !std::is_void_v<Adaptor>&&
                trait::is_input_prot<
                std::remove_reference_t<T>,
                typename trait::is_output_prot_gen<
                std::remove_reference_t<Front>>::prot_output_type,
                Adaptor>::value&&
                trait::is_compatible_prot_pair_v<std::remove_reference_t<Front>,
                std::remove_reference_t<T>, Adaptor>)
                inline decltype(auto) operator>>(T&& rear)&& {
                return pipeline<Front,
                    std::conditional_t<
                    std::is_lvalue_reference_v<T>,
                    std::add_lvalue_reference_t<std::remove_reference_t<T>>,
                    std::remove_reference_t<T>>,
                    Adaptor>(std::forward<Front>(front), std::forward<T>(rear),
                        std::forward<Adaptor>(adaptor));
            }

        private:
            template <typename F>
            inline pipeline_constructor(F&& f) : front(std::forward<F>(f)) {}

            template <typename F, typename U = Adaptor>
            inline pipeline_constructor(F&& f,
                std::enable_if<!std::is_void_v<U>, U>::type&& a)
                : front(std::forward<F>(f)), adaptor(std::forward<U>(a)) {
            }

            template <typename F, typename U = Adaptor>
            inline pipeline_constructor(F&& f, int a) // never use
                : front(std::forward<F>(f)) {
            }
            Front front;
            [[no_unique_address]] std::conditional_t<std::is_void_v<Adaptor>,
                std::monostate, Adaptor> adaptor;
        };

        // pipeline_started class - represents a started pipeline that cannot be
        // extended
        template <typename Pipeline, bool individual_coro, typename ErrorHandler>
        class pipeline_started
            : std::conditional_t<individual_coro, fsm_handle<void>, std::monostate> {
            __IO_INTERNAL_HEADER_PERMISSION;

        public:
            // Constructor that takes ownership of a pipeline
            template <typename Front, typename Rear, typename Adaptor>
            explicit pipeline_started(pipeline<Front, Rear, Adaptor>&& pipe)
                : _pipeline(std::move(pipe)) {
            }

            template <typename Front, typename Rear, typename Adaptor>
            explicit pipeline_started(pipeline<Front, Rear, Adaptor>&& pipe,
                ErrorHandler&& e)
                : _pipeline(std::move(pipe)),
                errorHandler(std::forward<ErrorHandler>(e)) {
            }

            pipeline_started(fsm_handle<void>&& handle)
                : std::conditional_t<individual_coro, fsm_handle<void>, std::monostate>(
                    std::move(handle)) {
            }

            // Delete copy constructor and assignment
            pipeline_started(const pipeline_started&) = delete;
            pipeline_started& operator=(const pipeline_started&) = delete;

            // Drive the pipeline with a specific index
            inline void operator<=(int which)
                requires(individual_coro == false)
            {
                if constexpr (std::is_same_v<ErrorHandler, std::monostate>) {
                    _pipeline.process(which, std::monostate{});
                }
                else {
                    _pipeline.process(which, std::ref(errorHandler));
                }
            }

            // Get the awaitable for the pipeline
            inline decltype(auto) operator+()
                requires(individual_coro == false)
            {
                return _pipeline.awaitable();
            }

        private:
            // Delete user move constructor and assignment
            pipeline_started(pipeline_started&&) = default;
            pipeline_started& operator=(pipeline_started&&) = default;

            [[no_unique_address]] std::conditional_t<individual_coro, std::monostate,
                Pipeline> _pipeline;
            [[no_unique_address]] ErrorHandler errorHandler;
        };

        template <> struct pipeline<void, void, void> {
            __IO_INTERNAL_HEADER_PERMISSION;
            // Chain operator for starting the pipeline - requires output protocol
            template <typename T>
                requires trait::is_output_prot<std::remove_reference_t<T>>::value
            inline auto operator>>(T&& front)&& {
                return pipeline_constructor<
                    std::conditional_t<
                    std::is_lvalue_reference_v<T>,
                    std::add_lvalue_reference_t<std::remove_reference_t<T>>,
                    std::remove_reference_t<T>>,
                    void, void>(std::forward<T>(front));
            }
            inline pipeline() {}
        };

        constexpr size_t none_side = 0;
        constexpr size_t in_side = 1;
        constexpr size_t out_side = 2;

        template <typename T> struct protocol_lock {
            protocol_lock() {}
            template <typename... Args>
            protocol_lock(Args... args) : temp(std::forward<Args>(args)...) {}
            promise<T> send_prom;
            promise<> recv_prom;
            [[no_unique_address]] std::optional<T> temp;

            template <size_t Side = none_side>
            bool try_send() {
                if (send_prom.valid() && temp.has_value()) {
                    if constexpr (Side == out_side)
                        send_prom.resolve(std::move(*temp));
                    else
                        send_prom.resolve_later(std::move(*temp));

                    if constexpr (Side == in_side)
                        recv_prom.resolve();
                    else
                        recv_prom.resolve_later();

                    temp.reset();
                    return true;
                }
                else {
                    return false;
                }
            }

            template <typename U>
            bool try_send(U&& value) requires (std::is_convertible_v<U, T>) {
                if (send_prom.valid()) {
                    send_prom.resolve_later(std::forward<U>(value));
                    recv_prom.resolve_later();
                    return true;
                }
                else {
                    temp = T(std::forward<U>(value));
                    return false;
                }
            }
        };

        // blackhole adaptor
        template <typename Out, typename In> struct blackhole_adaptor {
            inline std::optional<In> operator()(const Out&) { return std::nullopt; }
        };

        template <typename T> struct blackhole_protocol {
            inline void operator<<(T&) {}
        };
    } // namespace IO_LIB_VERSION___
} // namespace io