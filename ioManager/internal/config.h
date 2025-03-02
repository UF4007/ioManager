#ifndef IO_EXCEPTION_ON
#if (defined(__EXCEPTIONS) || defined(__cpp_exceptions) || \
    (defined(_HAS_EXCEPTIONS) && _HAS_EXCEPTIONS))
#define IO_EXCEPTION_ON 1
#else
#define IO_EXCEPTION_ON 0
#endif
#endif