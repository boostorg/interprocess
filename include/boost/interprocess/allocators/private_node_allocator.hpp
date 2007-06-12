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
#include <boost/utility/addressof.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/config.hpp>
#include <boost/assert.hpp>
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
   /// @endcond

   public:
   //-------
   typedef typename detail::
      pointer_to_other<void_pointer, T>::type            pointer;
   typedef typename detail::
      pointer_to_other<void_pointer, const T>::type      const_pointer;
   typedef T                                             value_type;
   typedef typename workaround::random_it
                     <value_type>::reference             reference;
   typedef typename workaround::random_it
                     <value_type>::const_reference       const_reference;
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

   void priv_initialize()
   {
      typedef detail::private_node_pool
               <SegmentManager, sizeof(T), NodesPerChunk>   priv_node_pool_t;
      void * ptr = mp_segment_mngr->allocate(sizeof(priv_node_pool_t));
      //This does not throw
      new(ptr)priv_node_pool_t(detail::get_pointer(mp_segment_mngr));
      //Construction ok, don't free memory
      mp_node_pool = ptr;
   }

   void priv_free()
   {
      //-------------------------------------------------------------
      typedef detail::private_node_pool
                  <SegmentManager, sizeof(T), NodesPerChunk>   priv_node_pool_t;
      //-------------------------------------------------------------
      priv_node_pool_t *pnode_pool     = static_cast<priv_node_pool_t*>
                                          (detail::get_pointer(mp_node_pool));
      segment_manager  &segment_mngr   = *mp_segment_mngr;
      //This never throws
      pnode_pool->~priv_node_pool_t();
      segment_mngr.deallocate(pnode_pool);
   }
   /// @endcond

   public:

   //!Constructor from a segment manager
   private_node_allocator(segment_manager *segment_mngr)
      : mp_segment_mngr(segment_mngr), mp_node_pool(0){}

   //!Copy constructor from other private_node_allocator. Never throws
   private_node_allocator(const private_node_allocator &other)
      : mp_segment_mngr(other.get_segment_manager()), mp_node_pool(0){}

   //!Copy constructor from related private_node_allocator. Never throws.
   template<class T2>
   private_node_allocator
      (const private_node_allocator<T2, SegmentManager, NodesPerChunk> &other)
      : mp_segment_mngr(other.get_segment_manager()), mp_node_pool(0){}

   //!Destructor, frees all used memory. Never throws
   ~private_node_allocator() 
      {  if(mp_node_pool)  priv_free();  }

   //!Returns the segment manager. Never throws
   segment_manager* get_segment_manager()const
      {  return detail::get_pointer(mp_segment_mngr);  }

   //!Return address of mutable value. Never throws
   pointer address(reference value) const
      {  return pointer(boost::addressof(value));  }

   //!Return address of non mutable value. Never throws
   const_pointer address(const_reference value) const
      {  return const_pointer(boost::addressof(value));  }

   //!Construct object, calling constructor. 
   //!Throws if T(const Convertible &) throws
   template<class Convertible>
   void construct(const pointer &ptr, const Convertible &value)
      {  new(detail::get_pointer(ptr)) value_type(value);  }

   //!Destroys object. Throws if object's destructor throws
   void destroy(const pointer &ptr)
      {  BOOST_ASSERT(ptr != 0); (*ptr).~value_type();  }

   //!Returns the number of elements that could be allocated. Never throws
   size_type max_size() const
      {  return mp_segment_mngr->get_size()/sizeof(value_type);  }

   //!Allocate memory for an array of count elements. 
   //!Throws boost::interprocess::bad_alloc if there is no enough memory
   pointer allocate(size_type count, cvoid_pointer hint = 0)
   {
      (void)hint;
      if(count > ((size_type)-1)/sizeof(value_type))
         throw bad_alloc();
      //----------------------------------------------------------
      typedef detail::private_node_pool
                  <SegmentManager, sizeof(T), NodesPerChunk>   priv_node_pool_t;
      //----------------------------------------------------------
      if(!mp_node_pool)   priv_initialize();
      priv_node_pool_t *node_pool  = static_cast<priv_node_pool_t*>
                                       (detail::get_pointer(mp_node_pool));
      return pointer(static_cast<value_type*>(node_pool->allocate(count)));
   }

   //!Deallocate allocated memory. Never throws
   void deallocate(const pointer &ptr, size_type count)
   {
      //----------------------------------------------------------
      typedef detail::private_node_pool
                  <SegmentManager, sizeof(T), NodesPerChunk>   priv_node_pool_t;
      //----------------------------------------------------------
      if(!mp_node_pool)   priv_initialize();
      priv_node_pool_t *node_pool = static_cast<priv_node_pool_t*>(
                                       detail::get_pointer(mp_node_pool));
      node_pool->deallocate(detail::get_pointer(ptr), count);
   }

   //!Swaps allocators. Does not throw. If each allocator is placed in a
   //!different shared memory segments, the result is undefined.*/
   friend void swap(self_t &alloc1,self_t &alloc2)
   {
      using namespace std;
      swap (alloc1.mp_segment_mngr,   alloc2.mp_segment_mngr);
      swap (alloc1.mp_node_pool,    alloc2.mp_node_pool);
   }

   /// @cond
   private:
   // We can't instantiate a pointer like this:
   // detail::private_node_pool<SegmentManager, 
   //                             sizeof(T),  NodesPerChunk> *mp_node_pool;
   // since it can provoke an early instantiation of T, that is not 
   // complete at the moment (for example, a node of a node-based pointer)
   // This provokes errors on some node based container implementations using
   // this pooled allocator as this allocator type.
   // 
   // Because of this, we will use a void smart pointer and we'll do some casts
   // when needed.
   segment_mngr_ptr_t      mp_segment_mngr;
   void_pointer            mp_node_pool;
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

