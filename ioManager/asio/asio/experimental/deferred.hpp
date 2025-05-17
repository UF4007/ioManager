//
// experimental/deferred.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXPERIMENTAL_DEFERRED_HPP
#define ASIO_EXPERIMENTAL_DEFERRED_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "../detail/config.hpp"
#include "../deferred.hpp"

#include "../detail/push_options.hpp"

namespace asio {
namespace experimental {

#if !defined(ASIO_NO_DEPRECATED)
using asio::deferred_t;
using asio::deferred;
#endif // !defined(ASIO_NO_DEPRECATED)

} // namespace experimental
} // namespace asio

#include "../detail/pop_options.hpp"

#endif // ASIO_EXPERIMENTAL_DEFERRED_HPP
