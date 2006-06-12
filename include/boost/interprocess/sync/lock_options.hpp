//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_LOCK_OPTIONS_HPP
#define BOOST_INTERPROCESS_LOCK_OPTIONS_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

/*!\file
   Describes the lock options with associated with interprocess_mutex lock constructors.
*/

namespace boost {
namespace interprocess {

namespace detail{
   struct dont_lock_t{};
   struct try_to_lock_t {};
}  //namespace detail{

static const detail::dont_lock_t    dont_lock   = detail::dont_lock_t();
static const detail::try_to_lock_t  try_to_lock = detail::try_to_lock_t();

} // namespace interprocess
} // namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif // BOOST_INTERPROCESS_LOCK_OPTIONS_HPP
