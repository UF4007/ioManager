#define __IO_INTERNAL_HEADER_PERMISSION																friend class lowlevel;\
																									friend struct awaitable;\
                                                    template <typename _T2>   						friend struct coPromiseStack;\
													template <typename _T2>   						friend class coPromise;\
													template <typename _T2>   						friend struct coAsync;\
																									friend class coSelector;\
																									friend struct ioManager;\
																									friend struct tcp::acceptor;\

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct awaitable;
template <typename _T> class coPromise;
template <typename _T> struct coPromiseStack;
template <typename _T> struct coAsync;
struct ioManager;
class coSelector;
namespace tcp {
	struct acceptor;
};