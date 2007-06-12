//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_MANAGED_EXTERNAL_BUFFER_HPP
#define BOOST_INTERPROCESS_MANAGED_EXTERNAL_BUFFER_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/creation_tags.hpp>
#include <boost/interprocess/detail/managed_memory_impl.hpp>
#include <boost/interprocess/detail/move.hpp>

/*!\file
   Describes a named user memory allocation user class. 
*/

namespace boost {
namespace interprocess {

//!A basic user memory named object creation class. Inherits all 
//!basic functionality from 
//!basic_managed_memory_impl<CharType, AllocationAlgorithm, IndexType>*/
template
      <
         class CharType, 
         class AllocationAlgorithm, 
         template<class IndexConfig> class IndexType
      >
class basic_managed_external_buffer 
   : public detail::basic_managed_memory_impl <CharType, AllocationAlgorithm, IndexType>
{
   /// @cond
   typedef detail::basic_managed_memory_impl 
      <CharType, AllocationAlgorithm, IndexType>    base_t;
   /// @endcond

   public:
   //!Creates and places the segment manager. This can throw
   basic_managed_external_buffer
      (detail::create_only_t, void *addr, std::size_t size)
   {
      if(!base_t::create_impl(addr, size)){
         throw interprocess_exception();
      }
   }

   //!Creates and places the segment manager. This can throw
   basic_managed_external_buffer
      (detail::open_only_t, void *addr, std::size_t size)
   {
      if(!base_t::open_impl(addr, size)){
         throw interprocess_exception();//return false;
      }
   }

   //!Moves the ownership of "moved"'s managed memory to *this. Does not throw
   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   basic_managed_external_buffer
      (detail::moved_object<basic_managed_external_buffer> &moved)
   {  this->swap(moved.get());   }
   #else
   basic_managed_external_buffer
      (basic_managed_external_buffer &&moved)
   {  this->swap(moved);   }
   #endif

   //!Moves the ownership of "moved"'s managed memory to *this. Does not throw
   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   basic_managed_external_buffer &operator=
      (detail::moved_object<basic_managed_external_buffer> &moved)
   {  this->swap(moved.get());   return *this;  }
   #else
   basic_managed_external_buffer &operator=
      (basic_managed_external_buffer &&moved)
   {  this->swap(moved);   return *this;  }
   #endif

   void grow(std::size_t extra_bytes)
   {  base_t::grow(extra_bytes);   }

   //!Swaps the ownership of the managed heap memories managed by *this and other.
   //!Never throws.
   void swap(basic_managed_external_buffer &other)
   {  base_t::swap(other); }

};

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_MANAGED_EXTERNAL_BUFFER_HPP

