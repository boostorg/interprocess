//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
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

//Predeclaration of pointer casts
namespace boost
{
   template<class T, class U>
   T static_pointer_cast(const U &r);
   template<class T, class U>
   T const_pointer_cast(const U &r);
   template<class T, class U>
   T dynamic_pointer_cast(const U &r);
   template<class T, class U>
   T reinterpret_pointer_cast(const U &r);
}  //namespace boost

namespace boost { namespace interprocess { namespace detail {

template<class T, class U> 
inline T do_static_cast_impl(const U &r, const boost::false_type)
{  
   return static_pointer_cast<typename T::value_type>(r);  
}

template<class T, class U> 
inline T do_static_cast_impl(const U &r, const boost::true_type)
{  
   return static_cast<T>(r);  
}

template<class T, class U> 
inline T do_dynamic_cast_impl(const U &r, const boost::false_type)
{  
   return dynamic_pointer_cast<typename T::value_type>(r);  
}

template<class T, class U> 
inline T do_dynamic_cast_impl(const U &r, const boost::true_type)
{  
   return dynamic_cast<T>(r);  
}

template<class T, class U> 
inline T do_reinterpret_cast_impl(const U &r, const boost::false_type)
{  
   return reinterpret_pointer_cast<typename T::value_type>(r);  
}

template<class T, class U> 
inline T do_reinterpret_cast_impl(const U &r, const boost::true_type)
{  
   return reinterpret_cast<T>(r);  
}

template<class T, class U> 
inline T do_const_cast_impl(const U &r, const boost::false_type)
{  
   return const_pointer_cast<typename T::value_type>(r);  
}

template<class T, class U> 
inline T do_const_cast_impl(const U &r, const boost::true_type)
{  
   return const_cast<T>(r);  
}

}  //namespace detail {

template<class T, class U> 
inline T do_static_cast(const U &r)
{  
   return detail::do_static_cast_impl<T>(r, typename boost::is_pointer<U>::type());  
}

template<class T, class U> 
inline T do_dynamic_cast(const U &r)
{  
   return detail::do_dynamic_cast_impl<T>(r, typename boost::is_pointer<U>::type());  
}

template<class T, class U> 
inline T do_reinterpret_cast(const U &r)
{  
   return detail::do_reinterpret_cast_impl<T>(r, typename boost::is_pointer<U>::type());  
}

template<class T, class U> 
inline T do_const_cast(const U &r)
{  
   return detail::do_const_cast_impl<T>(r, typename boost::is_pointer<U>::type());  
}


}}   //namespace boost { namespace interprocess { 

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_GENERIC_CAST_HPP

