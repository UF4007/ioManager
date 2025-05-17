#ifndef IO_EXCEPTION_ON
#if (defined(__EXCEPTIONS) || defined(__cpp_exceptions) || \
    (defined(_HAS_EXCEPTIONS) && _HAS_EXCEPTIONS))
#define IO_EXCEPTION_ON 1
#else
#define IO_EXCEPTION_ON 0
#endif
#endif

#if (IO_EXCEPTION_ON)
#define IO_THROW(content) throw content;
#else 
#define IO_THROW(content) abort();
#endif

#ifndef IO_USE_ASIO
#define IO_USE_ASIO 1
#endif

#ifndef IO_USE_OPENSSL
#define IO_USE_OPENSSL 0
#endif