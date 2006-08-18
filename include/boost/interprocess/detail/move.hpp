//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_MOVE_HPP
#define BOOST_INTERPROCESS_MOVE_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

/*!\file
   Describes a function and a type to emulate move semantics.
*/
namespace boost {
namespace interprocess {
namespace detail {

/*!An object that represents a moved object.*/
template<class Object>
struct moved_object
{  
   moved_object(const Object &obj)
      :  m_obj(const_cast<Object *>(&obj))
   {}

   Object &get()
   {  return *m_obj;  }

   private:
   Object *m_obj; 
};

}  //namespace detail {

/*!A function that converts an object to a moved object so that 
   it can match a function taking a detail::moved_object object.*/
template<class Object>
detail::moved_object<Object> move
   (/*const */Object &lock)
{  return detail::moved_object<Object>(lock);   }

}  //namespace interprocess {
}  //namespace boost {


#include <boost/interprocess/detail/config_end.hpp>

#endif


