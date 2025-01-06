//
// detail/impl/throw_error.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_IMPL_THROW_ERROR_IPP
#define ASIO_DETAIL_IMPL_THROW_ERROR_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "../config.hpp"
#include "../throw_error.hpp"
#include "../../system_error.hpp"

#include "../push_options.hpp"

namespace asio {
namespace detail {

void do_throw_error(
    const asio::error_code& err
    ASIO_SOURCE_LOCATION_PARAM)
{
  asio::system_error e(err);
  asio::detail::throw_exception(e ASIO_SOURCE_LOCATION_ARG);
}

void do_throw_error(
    const asio::error_code& err,
    const char* location
    ASIO_SOURCE_LOCATION_PARAM)
{
  asio::system_error e(err, location);
  asio::detail::throw_exception(e ASIO_SOURCE_LOCATION_ARG);
}

} // namespace detail
} // namespace asio

#include "../pop_options.hpp"

#endif // ASIO_DETAIL_IMPL_THROW_ERROR_IPP
