//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2008-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_VARIADIC_TEMPLATES_TOOLS_HPP
#define BOOST_INTERPROCESS_DETAIL_VARIADIC_TEMPLATES_TOOLS_HPP

#if defined(_MSC_VER)
#  pragma once
#endif

#include <boost/container/detail/variadic_templates_tools.hpp>

namespace boost {
namespace interprocess {
namespace ipcdetail {

using boost::container::container_detail::tuple;
using boost::container::container_detail::build_number_seq;
using boost::container::container_detail::index_tuple;
using boost::container::container_detail::get;

}}}   //namespace boost { namespace interprocess { namespace ipcdetail {

#endif   //#ifndef BOOST_INTERPROCESS_DETAIL_VARIADIC_TEMPLATES_TOOLS_HPP
