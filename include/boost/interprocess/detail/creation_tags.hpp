//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_CREATION_TAGS_HPP
#define BOOST_INTERPROCESS_CREATION_TAGS_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

namespace boost { namespace interprocess { namespace detail {

struct create_only_t {};
struct open_only_t {};
struct open_or_create_t {};

}  //namespace detail {

static const detail::create_only_t     create_only    = detail::create_only_t();
static const detail::open_or_create_t  open_or_create = detail::open_or_create_t();
static const detail::open_only_t       open_only      = detail::open_only_t();

}}  //namespace boost { namespace interprocess {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_CREATION_TAGS_HPP

