// includes and define
#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
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
#include <coroutine>
#include <span>
#include <semaphore>
#include <algorithm>
#include <type_traits>
#include <functional>
#include "selectMarco.h"

#if IO_USE_ASIO
//#include "../asio/asio.hpp"
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
#endif