#define __IO_INTERNAL_HEADER_PERMISSION																friend class lowlevel;\
                                                    template <typename _T2>   						friend struct coPromiseStack;\
													template <typename _T2>   						friend class coPromise;\
													template <typename _T2>   						friend struct coAsync;\
																									friend class coSelector;\
																									friend struct ioManager;\

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
template <typename _T> class coPromise;
template <typename _T> struct coPromiseStack;
template <typename _T> struct coAsync;
struct ioManager;
class coSelector;
template <typename OutT, typename InT> struct ioChannel;