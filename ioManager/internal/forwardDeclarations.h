#define __IO_INTERNAL_HEADER_PERMISSION																friend class lowlevel;\
													template <typename _TypeA>						friend class coPromise;\
																									friend class coMultiplex;\
																									friend class ioManager;\
																									friend class volunteerDriver;\
																									friend class netif;\
																									friend class icmp_client_socket;\
																									friend class tcp_client_socket;\
																									friend class tcp_server_socket;\
																									friend class udp_socket;\

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
template <typename _T> class coPromise;
class ioManager;
struct protCtl;
template <typename _T> struct ioChannel;
template <typename _T> class ioSelector;
class tcp_client_socket;