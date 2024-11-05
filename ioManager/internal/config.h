#ifndef IO_SOCKET_BUFFER_SIZE
#define IO_SOCKET_BUFFER_SIZE 8192
#endif

#define IO_USE_SELECT 1

#ifndef IO_USE_SELECT
	#ifdef _WIN32
		#define IO_USE_IOCP 1
	#elif defined(__linux__)
		#define IO_USE_EPOLL 1
	#else
		#define IO_USE_SELECT 1
	#endif
#endif