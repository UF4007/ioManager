//
// static_thread_pool.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_STATIC_THREAD_POOL_HPP
#define ASIO_STATIC_THREAD_POOL_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "detail/config.hpp"
#include "thread_pool.hpp"

#include "detail/push_options.hpp"

namespace asio {

typedef thread_pool static_thread_pool;

} // namespace asio

#include "detail/pop_options.hpp"

#endif // ASIO_STATIC_THREAD_POOL_HPP
