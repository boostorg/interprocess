//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_SMART_PTR_SEGMENT_DELETER_HPP
#define BOOST_INTERPROCESS_SMART_PTR_SEGMENT_DELETER_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#  pragma warning (disable : 4503)
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/utilities.hpp>

//!\file
//!Describes a deleter for smart pointers that deletes the
//!object using the segment manager

namespace boost {
namespace interprocess {

template < class SegmentManager
         , class VoidPointer = typename SegmentManager::void_pointer>
struct segment_deleter
{
   typedef typename detail::pointer_to_other
      <VoidPointer, SegmentManager>::type SegmentManagerPtr;

   segment_deleter(SegmentManagerPtr mngr)
      :  m_mngr(mngr)
   {}

   template<class Ptr>
   void operator() (const Ptr &ptr) const
   {
      m_mngr->destroy_ptr(detail::get_pointer(ptr));
   }

   /// @cond
   private:
   SegmentManagerPtr m_mngr;
   /// @endcond
};


}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_SMART_PTR_SEGMENT_DELETER_HPP
