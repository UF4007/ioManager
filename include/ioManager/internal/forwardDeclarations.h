#define __IO_INTERNAL_HEADER_PERMISSION								template <typename T2, size_t capacity2> friend struct io::forward_fifo;\
																	template <typename _Struc2> friend class io::dualbuf;\
																	friend class io::lowlevel;\
																	friend struct io::future;\
																	friend struct io::async_future;\
																	friend struct io::async_promise;\
																	template <typename T2>requires (!std::is_same_v<T2, void>)friend struct io::future_with;\
																	friend struct io::clock;\
																	template <typename T2>friend struct io::promise;\
																	template <typename T2>friend struct io::fsm;\
																	template <typename T2>requires (std::is_same_v<T2, void> || std::is_default_constructible_v<T2>)friend struct io::fsm_func;\
																	template <typename T2>friend struct io::fsm_handle;\
																	friend struct io::manager;\
																	friend struct io::sock::tcp;\
																	friend struct io::sock::tcp_accp;\
																	friend struct io::sock::udp;\
																	template <typename Rear2, typename Front2, typename Adaptor2>friend struct io::pipeline_constructor;\
																	template <typename Rear2, typename Front2, typename Adaptor2>friend struct io::pipeline;\
																	template <typename Pipeline2, bool individual_coro2, typename ErrorHandler2>friend class pipeline_started;\
																	template <typename FSM_Index2, typename FSM_In2, typename FSM_Out2>friend struct io::rpc;\

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
template <typename T, size_t batch_size>struct hive;
template <typename T>struct skip_table;
template <typename T, size_t capacity> struct forward_fifo;
template <typename _Struc> class dualbuf;
template<typename T>
	requires (!std::is_same_v<T, void>)
struct future_with;
struct awaitable;
struct future;
struct clock;
template <typename T>struct promise;
struct async_future;
struct async_promise;
template <typename T>struct fsm;
template <typename T>
	requires (std::is_same_v<T, void> || std::is_default_constructible_v<T>)
struct fsm_func;
template <typename T>struct fsm_handle;
struct manager;
struct pool;
struct yield_t;
template <typename Front, typename Rear, typename Adaptor>struct pipeline_constructor;
template <typename Front, typename Rear, typename Adaptor>struct pipeline;
template <typename Pipeline, bool individual_coro, typename ErrorHandler>class pipeline_started;
template <typename key, typename req, typename rsp>struct rpc;
namespace sock {
	struct tcp;
	struct tcp_accp;
	struct udp;
};

#define IO_MANAGER_FORWARD_FUNC(___obj___,___func___) template <typename ...Args> auto ___func___(Args&&...args) { return ___obj___.___func___(std::forward<Args>(args)...); }

#define IO_MANAGER_BAN_COPY(___obj___)     \
    ___obj___(const ___obj___ &) = delete; \
    ___obj___ &operator=(const ___obj___ &) = delete;

#define IO_MANAGER_BAN_MOVE(___obj___)  \
    ___obj___(___obj___ &&) = delete;   \
    ___obj___ &operator=(___obj___ &&) = delete;

#define ___DEFER_CONCAT_IMPL(x, y) x##y
#define ___DEFER_CONCAT(x, y) ___DEFER_CONCAT_IMPL(x, y)
#define IO_DEFER io::defer_t ___DEFER_CONCAT(defer_obj_, __LINE__)