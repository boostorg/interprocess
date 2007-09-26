//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_PRIVATE_NODE_ALLOCATOR_HPP
#define BOOST_INTERPROCESS_PRIVATE_NODE_ALLOCATOR_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/assert.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/interprocess/allocators/detail/node_pool.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <memory>
#include <algorithm>

#include <stdio.h>
#include <cstddef>

//!\file
//!Describes private_node_allocator pooled shared memory STL compatible allocator 

namespace boost {
namespace interprocess {

//!An STL node allocator that uses a segment manager as memory 
//!source. The internal pointer type will of the same type (raw, smart) as
//!"typename SegmentManager::void_pointer" type. This allows
//!placing the allocator in shared memory, memory mapped-files, etc...
//!This allocator has its own node pool. NodesPerChunk is the number of nodes allocated 
//!at once when the allocator needs runs out of nodes
template<class T, class SegmentManager, std::size_t NodesPerChunk>
class private_node_allocator
{
   /// @cond
   private:
   typedef typename SegmentManager::void_pointer         void_pointer;
   typedef typename detail::
      pointer_to_other<void_pointer, const void>::type   cvoid_pointer;
   typedef SegmentManager                                segment_manager;
   typedef typename detail::
      pointer_to_other<void_pointer, char>::type         char_pointer;
   typedef typename detail::pointer_to_other
      <void_pointer, segment_manager>::type              segment_mngr_ptr_t;
   typedef typename SegmentManager::
      mutex_family::mutex_type                           mutex_type;
   typedef private_node_allocator
      <T, SegmentManager, NodesPerChunk>                 self_t;
   typedef detail::private_node_pool
      <SegmentManager, sizeof(T), NodesPerChunk>         priv_node_pool_t;
   /// @endcond

   public:
   //-------
   typedef typename detail::
      pointer_to_other<void_pointer, T>::type            pointer;
   typedef typename detail::
      pointer_to_other<void_pointer, const T>::type      const_pointer;
   typedef T                                             value_type;
   typedef typename detail::add_reference
                     <value_type>::type                  reference;
   typedef typename detail::add_reference
                     <const value_type>::type            const_reference;
   typedef std::size_t                                   size_type;
   typedef std::ptrdiff_t                                difference_type;

   //!Obtains node_allocator from other node_allocator
   template<class T2>
   struct rebind
   {  
      typedef private_node_allocator<T2, SegmentManager, NodesPerChunk>   other;
   };

   /// @cond
   private:
   //!Not assignable from related private_node_allocator
   template<class T2, class MemoryAlgorithm2, std::size_t N2>
   private_node_allocator& operator=
      (const private_node_allocator<T2, MemoryAlgorithm2, N2>&);

   //!Not assignable from other private_node_allocator
   private_node_allocator& operator=(const private_node_allocator&);
   /// @endcond

   public:

   //!Constructor from a segment manager
   private_node_allocator(segment_manager *segment_mngr)
      : m_node_pool(segment_mngr){}

   //!Copy constructor from other private_node_allocator. Never throws
   private_node_allocator(const private_node_allocator &other)
      : m_node_pool(other.get_segment_manager()){}

   //!Copy constructor from related private_node_allocator. Never throws.
   template<class T2>
   private_node_allocator
      (const private_node_allocator<T2, SegmentManager, NodesPerChunk> &other)
      : m_node_pool(other.get_segment_manager())
   {}

   //!Destructor, frees all used memory. Never throws
   ~private_node_allocator() 
   {}

   //!Returns the segment manager. Never throws
   segment_manager* get_segment_manager()const
   {  return m_node_pool.get_segment_manager(); }

   //!Returns the number of elements that could be allocated. Never throws
   size_type max_size() const
   {  return this->get_segment_manager()->get_size()/sizeof(value_type);  }

   //!Allocate memory for an array of count elements. 
   //!Throws boost::interprocess::bad_alloc if there is no enough memory
   pointer allocate(size_type count, cvoid_pointer hint = 0)
   {
      (void)hint;
      if(count > ((size_type)-1)/sizeof(value_type))
         throw bad_alloc();
      return pointer(static_cast<value_type*>(m_node_pool.allocate(count)));
   }

   //!Deallocate allocated memory. Never throws
   void deallocate(const pointer &ptr, size_type count)
   {  m_node_pool.deallocate(detail::get_pointer(ptr), count);   }

   //!Deallocates all free chunks of the pool
   void deallocate_free_chunks()
   {  m_node_pool.deallocate_free_chunks(); }

   //!Swaps allocators. Does not throw. If each allocator is placed in a
   //!different shared memory segments, the result is undefined.*/
   friend void swap(self_t &alloc1,self_t &alloc2)
   {  alloc1.m_node_pool.swap(alloc2.m_node_pool);  }

   //These functions are obsolete. These are here to conserve
   //backwards compatibility with containers using them...

   //!Returns address of mutable object.
   //!Never throws
   pointer address(reference value) const
   {  return pointer(boost::addressof(value));  }

   //!Returns address of non mutable object.
   //!Never throws
   const_pointer address(const_reference value) const
   {  return const_pointer(boost::addressof(value));  }

   //!Default construct an object. 
   //!Throws if T's default constructor throws*/
   void construct(const pointer &ptr)
   {  new(detail::get_pointer(ptr)) value_type;  }

   //!Destroys object. Throws if object's
   //!destructor throws
   void destroy(const pointer &ptr)
   {  BOOST_ASSERT(ptr != 0); (*ptr).~value_type();  }

   /// @cond
   private:
   priv_node_pool_t m_node_pool;
   /// @endcond
};

//!Equality test for same type of private_node_allocator
template<class T, class S, std::size_t NodesPerChunk> inline
bool operator==(const private_node_allocator<T, S, NodesPerChunk> &alloc1, 
                const private_node_allocator<T, S, NodesPerChunk> &alloc2)
{  return &alloc1 == &alloc2; }

//!Inequality test for same type of private_node_allocator
template<class T, class S, std::size_t NodesPerChunk> inline
bool operator!=(const private_node_allocator<T, S, NodesPerChunk> &alloc1, 
                const private_node_allocator<T, S, NodesPerChunk> &alloc2)
{  
   return &alloc1 != &alloc2;
}

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_PRIVATE_NODE_ALLOCATOR_HPP

