// includes and define
#include <variant>
#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <string_view>
#include <cwchar>
#include <queue>
#include <memory>
#include <assert.h>
#include <map>
#include <random>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <thread>
#include <mutex>
#include <stack>
#include <optional>
#include <list>
#include <atomic>
//_MSC_FULL_VER
/**
 * Disable optimization for coroutine header in MSVC to prevent crashes.
 * Issue: MSVC generates misaligned SSE instructions when optimizing coroutine code,
 * which leads to application crashes. No such issues observed with GCC or Clang.
 * Example of the problem can be found in the coro_chan_peak_shaving function in demo.h.
 * No minimal reproduction case available yet.
 * Testing confirms this does not cause performance degradation in coroutines or elsewhere.
 * 
 * MSVC Version: 19.43.34809
 * Microsoft Visual Studio Community 2022 (64 bit)
 */
#ifdef _MSC_VER
#pragma optimize("", off)
#endif
#include <coroutine>
#ifdef _MSC_VER
#pragma optimize("", on)
#endif
#include <span>
#include <semaphore>
#include <algorithm>
#include <type_traits>
#include <functional>
#include "selectMarco.h"

#if IO_USE_ASIO
#define ASIO_HAS_THREADS 1
#define ASIO_HAS_CO_AWAIT 1
//#include "../asio/asio.hpp"
#include "../asio/asio/awaitable.hpp"
#include "../asio/asio/buffer.hpp"
#include "../asio/asio/io_context.hpp"
#include "../asio/asio/io_context_strand.hpp"
#include "../asio/asio/ip/address.hpp"
#include "../asio/asio/ip/address_v4.hpp"
#include "../asio/asio/ip/address_v4_iterator.hpp"
#include "../asio/asio/ip/address_v4_range.hpp"
#include "../asio/asio/ip/address_v6.hpp"
#include "../asio/asio/ip/address_v6_iterator.hpp"
#include "../asio/asio/ip/address_v6_range.hpp"
#include "../asio/asio/ip/network_v4.hpp"
#include "../asio/asio/ip/network_v6.hpp"
#include "../asio/asio/ip/bad_address_cast.hpp"
#include "../asio/asio/ip/basic_endpoint.hpp"
#include "../asio/asio/ip/basic_resolver.hpp"
#include "../asio/asio/ip/basic_resolver_entry.hpp"
#include "../asio/asio/ip/basic_resolver_iterator.hpp"
#include "../asio/asio/ip/basic_resolver_query.hpp"
#include "../asio/asio/ip/host_name.hpp"
#include "../asio/asio/ip/icmp.hpp"
#include "../asio/asio/ip/multicast.hpp"
#include "../asio/asio/ip/resolver_base.hpp"
#include "../asio/asio/ip/resolver_query_base.hpp"
#include "../asio/asio/ip/tcp.hpp"
#include "../asio/asio/ip/udp.hpp"
#include "../asio/asio/co_spawn.hpp"
#include "../asio/asio/detached.hpp"
#include "../asio/asio/awaitable.hpp"
#include "../asio/asio/dispatch.hpp"
#endif

#if IO_USE_OPENSSL
#endif

#if IO_USE_STACKFUL
#define MINICORO_IMPL
namespace io {
    inline namespace IO_LIB_VERSION___ {
        namespace minicoro_detail {
#include "../minicoro/minicoro.h"
        }
    }
}
#endif