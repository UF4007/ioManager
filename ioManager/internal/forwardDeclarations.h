#define __IO_INTERNAL_HEADER_PERMISSION																friend class lowlevel;\
                                                    template <typename _T2>                         friend struct coPromiseStack;\
													template <typename _TypeA>						friend class coPromise;\
																									friend class coMultiplex;\
																									friend struct ioManager;\
																									friend class volunteerDriver;\
																									friend class netif;\
																									friend class icmp_client_socket;\
																									friend class tcp_client_socket;\
																									friend class tcp_server_socket;\
																									friend class udp_socket;\

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
template <typename _T> class coPromise;
template <typename _T> struct coPromiseStack;
struct ioManager;
struct protCtl;
template <typename _T> struct ioChannel;
template <typename _T> class ioSelector;
class tcp_client_socket;