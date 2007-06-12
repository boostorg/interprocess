//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2007.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_ALGORITHMS_HPP
#define BOOST_INTERPROCESS_DETAIL_ALGORITHMS_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/get_pointer.hpp>
#include <boost/detail/no_exceptions_support.hpp>

namespace boost {
namespace interprocess { 

template<class FwdIt, class T>
void uninitialized_fill(FwdIt first,  FwdIt last, const T& val)
{
   typedef typename std::iterator_traits<FwdIt>::value_type value_type;
   //Save initial position
   FwdIt init = first;

   BOOST_TRY{
      //Construct objects
      for (; first != last; ++first){
         new(detail::get_pointer(&*first))value_type(val);
      }
   }
   BOOST_CATCH(...){
      //Call destructors
      for (; init != first; ++init){
         detail::get_pointer(&*init)->~value_type();
      }
      BOOST_RETHROW;
   }
   BOOST_CATCH_END
}

template<class FwdIt, class Count, class T>
void uninitialized_fill_n(FwdIt first,  Count count, 
                          const T& val)
{
   typedef typename std::iterator_traits<FwdIt>::value_type value_type;
   //Save initial position
   FwdIt init = first;

   BOOST_TRY{
      //Construct objects
      for (; count--; ++first){
         new(detail::get_pointer(&*first))value_type(val);
      }
   }
   BOOST_CATCH(...){
      //Call destructors
      for (; init != first; ++init){
         detail::get_pointer(&*init)->~value_type();
      }
      BOOST_RETHROW;
   }
   BOOST_CATCH_END
}

template<class InIt, class OutIt>
InIt copy_n(InIt first, typename std::iterator_traits<InIt>::difference_type length, OutIt dest)
{   
   for (; length--; ++dest, ++first)
      *dest = *first;
   return first;
}

template<class InIt, class FwdIt>
typename std::iterator_traits<InIt>::difference_type
   n_uninitialized_copy(InIt first, InIt last, FwdIt dest)
{
   typedef typename std::iterator_traits<FwdIt>::value_type value_type;
   //Save initial destination position
   FwdIt dest_init = dest;
   typename std::iterator_traits<InIt>::difference_type constructed = 0;
   BOOST_TRY{
      //Try to build objects
      for (; first != last; ++dest, ++first, ++constructed){
         new(detail::get_pointer(&*dest))value_type(*first);
      }
   }
   BOOST_CATCH(...){
      //Call destructors
      for (; dest_init != dest; ++dest_init){
         detail::get_pointer(&*dest_init)->~value_type();
      }
      BOOST_RETHROW;
   }
   BOOST_CATCH_END
   return (constructed);
}

template<class InIt, class FwdIt> inline
FwdIt uninitialized_copy(InIt first, InIt last, FwdIt dest)
{
   typedef typename std::iterator_traits<FwdIt>::value_type value_type;
   //Save initial destination position
   FwdIt dest_init = dest;
   typename std::iterator_traits<InIt>::difference_type constructed = 0;
   BOOST_TRY{
      //Try to build objects
      for (; first != last; ++dest, ++first, ++constructed){
         new(detail::get_pointer(&*dest))value_type(*first);
      }
   }
   BOOST_CATCH(...){
      //Call destructors
      for (; dest_init != dest; ++dest_init){
         detail::get_pointer(&*dest_init)->~value_type();
      }
      BOOST_RETHROW;
   }
   BOOST_CATCH_END
   return (dest);
}

template<class InIt, class FwdIt> inline
InIt n_uninitialized_copy_n
   (InIt first, 
    typename std::iterator_traits<InIt>::difference_type count,
    FwdIt dest)
{
   typedef typename std::iterator_traits<FwdIt>::value_type value_type;
   //Save initial destination position
   FwdIt dest_init = dest;
   typename std::iterator_traits<InIt>::difference_type new_count = count+1;

   BOOST_TRY{
      //Try to build objects
      for (; --new_count; ++dest, ++first){
         new(detail::get_pointer(&*dest))value_type(*first);
      }
   }
   BOOST_CATCH(...){
      //Call destructors
      new_count = count - new_count;
      for (; new_count--; ++dest_init){
         detail::get_pointer(&*dest_init)->~value_type();
      }
      BOOST_RETHROW;
   }
   BOOST_CATCH_END
   return first;
}

// uninitialized_copy_copy
// Copies [first1, last1) into [result, result + (last1 - first1)), and
// copies [first2, last2) into
// [result + (last1 - first1), result + (last1 - first1) + (last2 - first2)).
template <class InpIt1, class InpIt2, class FwdIt>
FwdIt uninitialized_copy_copy(InpIt1 first1, InpIt1 last1,
                                 InpIt2 first2, InpIt2 last2,
                                 FwdIt result)
{
   typedef typename std::iterator_traits<FwdIt>::value_type value_type;
   FwdIt mid = boost::interprocess::uninitialized_copy(first1, last1, result);
   BOOST_TRY {
      return boost::interprocess::uninitialized_copy(first2, last2, mid);
   }
   BOOST_CATCH(...){
      for(;result != mid; ++result){
         detail::get_pointer(&*result)->~value_type();
      }
      BOOST_RETHROW
   }
   BOOST_CATCH_END
}

// uninitialized_copy_n_copy_n
// Copies [first1, first1 + n1) into [result, result + n1), and
// copies [first2, first2 + n2) into
// [result + n1, result + n1 + n2).
template <class InpIt1, class InpIt2, class FwdIt>
InpIt2 uninitialized_copy_n_copy_n
  (InpIt1 first1,
   typename std::iterator_traits<InpIt1>::difference_type n1,
   InpIt2 first2, 
   typename std::iterator_traits<InpIt2>::difference_type n2,
   FwdIt result)
{
   typedef typename std::iterator_traits<FwdIt>::value_type value_type;
   typename std::iterator_traits<InpIt1>::difference_type c1 = n1+1;
   typename std::iterator_traits<InpIt2>::difference_type c2 = n2+1;
   FwdIt dest_init = result;

   BOOST_TRY{
      //Try to build objects
      for (; --c1; ++result, ++first1){
         new(detail::get_pointer(&*result))value_type(*first1);
      }
      for (; --c2; ++result, ++first2){
         new(detail::get_pointer(&*result))value_type(*first2);
      }
   }
   BOOST_CATCH(...){
      //Call destructors
   typename std::iterator_traits<FwdIt>::
      difference_type c = (n1 - c1) + (n2 - c2);
      for (; c--; ++dest_init){
         detail::get_pointer(&*dest_init)->~value_type();
      }
      BOOST_RETHROW;
   }
   BOOST_CATCH_END
   return first2;
}

}  //namespace interprocess { 
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_DETAIL_ALGORITHMS_HPP

