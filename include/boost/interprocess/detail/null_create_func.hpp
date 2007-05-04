//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_NULL_CREATION_FUNCTOR_HPP
#define BOOST_INTERPROCESS_DETAIL_NULL_CREATION_FUNCTOR_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

namespace boost {
namespace interprocess {
namespace detail{

/*!No-op functor*/
struct null_func_t
{
   bool operator()(const segment_info_t *, bool) const
      {   return true;   }
};

}  //namespace detail{
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_DETAIL_NULL_CREATION_FUNCTOR_HPP
