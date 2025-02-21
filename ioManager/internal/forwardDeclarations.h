#define __IO_INTERNAL_HEADER_PERMISSION								template <typename T2, size_t capacity2> friend struct io::forward_fifo;\
																	template <typename _Struc2> friend class io::dualbuf;\
																	friend class io::lowlevel;\
																	friend struct io::future;\
																	friend struct io::clock;\
																	friend struct io::timer;\
																	template <typename T2>friend struct io::promise;\
																	friend struct io::async_future;\
																	friend struct io::async_promise;\
																	template <typename T2>requires std::is_move_constructible_v<T2>friend struct io::chan;\
																	template <typename T2>requires std::is_move_constructible_v<T2>friend struct io::chan_r;\
																	template <typename T2>requires std::is_move_constructible_v<T2>friend struct io::chan_s;\
																	template <typename T2>friend struct io::fsm;\
																	template <typename T2>requires (std::is_same_v<T2, void> || std::is_default_constructible_v<T2>)friend struct io::fsm_func;\
																	template <typename T2>friend struct io::fsm_handle;\
																	friend struct io::manager;\
																	friend struct io::tcp::socket;\
																	friend struct io::tcp::acceptor;\
																	friend struct io::udp::socket;\

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
template <typename T>struct hive;
template <typename T>struct skip_table;
template <typename T, size_t capacity> struct forward_fifo;
template <typename _Struc> class dualbuf;
struct awaitable;
struct future;
struct clock;
struct timer;
template <typename T>struct promise;
struct async_future;
struct async_promise;
template <typename T>
	requires std::is_move_constructible_v<T>
struct chan;
template <typename T>
	requires std::is_move_constructible_v<T>
struct chan_r;
template <typename T>
	requires std::is_move_constructible_v<T>
struct chan_s;
template <typename T>struct fsm;
template <typename T>
	requires (std::is_same_v<T, void> || std::is_default_constructible_v<T>)
struct fsm_func;
template <typename T>struct fsm_handle;
struct manager;
namespace tcp {
	struct socket;
	struct acceptor;
};
namespace udp {
	struct socket;
};

#define IO_MANAGER_FORWARD_FUNC(___obj___,___func___) template <typename ...Args> auto ___func___(Args&&...args) { return ___obj___.___func___(std::forward<Args>(args)...); }

#define IO_MANAGER_BAN_COPY(___obj___)     \
    ___obj___(const ___obj___ &) = delete; \
    ___obj___ &operator=(const ___obj___ &) = delete;

#define IO_MANAGER_BAN_MOVE(___obj___)  \
    ___obj___(___obj___ &&) = delete;   \
    ___obj___ &operator=(___obj___ &&) = delete;