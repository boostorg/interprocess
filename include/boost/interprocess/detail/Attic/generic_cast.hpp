//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_GENERIC_CAST_HPP
#define BOOST_INTERPROCESS_GENERIC_CAST_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/type_traits/is_pointer.hpp>


namespace boost{
namespace interprocess{

template<class T>
class cast_to;

template<class T>
class cast_to<T*>
{
   public:
   template<class S>
   static T* using_static_cast(S *s)
   {  return static_cast<T*>(s); }

   template<class S>
   static T* using_dynamic_cast(S *s)
   {  return dynamic_cast<T*>(s); }

   template<class S>
   static T* using_const_cast(S *s)
   {  return const_cast<T*>(s); }

   template<class S>
   static T* using_reinterpret_cast(S *s)
   {  return reinterpret_cast<T*>(s); }
};

}  //namespace interprocess{
}  //namespace boost{

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_GENERIC_CAST_HPP

