//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_ADAPTIVE_POOL_HPP
#define BOOST_INTERPROCESS_ADAPTIVE_POOL_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/assert.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/type_traits.hpp>
#include <boost/interprocess/allocators/detail/adaptive_node_pool.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <memory>
#include <algorithm>
#include <stdio.h>
#include <cstddef>

//!\file
//!Describes adaptive_pool pooled shared memory STL compatible allocator 

namespace boost {
namespace interprocess {

//!An STL node allocator that uses a segment manager as memory 
//!source. The internal pointer type will of the same type (raw, smart) as
//!"typename SegmentManager::void_pointer" type. This allows
//!placing the allocator in shared memory, memory mapped-files, etc...
//!This node allocator shares a segregated storage between all instances 
//!of adaptive_pool with equal sizeof(T) placed in the same segment 
//!group. NodesPerChunk is the number of nodes allocated at once when the allocator
//!needs runs out of nodes. MaxFreeChunks is the number of free nodes
//!in the adaptive node pool that will trigger the deallocation of
template<class T, class SegmentManager, std::size_t NodesPerChunk, std::size_t MaxFreeChunks>
class adaptive_pool
{
   public:
   typedef typename SegmentManager::void_pointer         void_pointer;
   typedef typename detail::
      pointer_to_other<void_pointer, const void>::type   cvoid_pointer;
   typedef SegmentManager                                segment_manager;
   typedef typename detail::
      pointer_to_other<void_pointer, char>::type         char_pointer;
   typedef typename SegmentManager::
      mutex_family::mutex_type                           mutex_type;
   typedef adaptive_pool
      <T, SegmentManager, NodesPerChunk, MaxFreeChunks>  self_t;
   typedef detail::shared_adaptive_node_pool
      < SegmentManager, mutex_type
      , sizeof(T), NodesPerChunk, MaxFreeChunks>         node_pool_t;
   typedef typename detail::
      pointer_to_other<void_pointer, node_pool_t>::type  node_pool_ptr;

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

   //!Obtains adaptive_pool from 
   //!adaptive_pool
   template<class T2>
   struct rebind
   {  
      typedef adaptive_pool<T2, SegmentManager, NodesPerChunk, MaxFreeChunks>       other;
   };

   /// @cond
   private:
   //!Not assignable from related adaptive_pool
   template<class T2, class SegmentManager2, std::size_t N2, std::size_t F2>
   adaptive_pool& operator=
      (const adaptive_pool<T2, SegmentManager2, N2, F2>&);

   //!Not assignable from other adaptive_pool
   adaptive_pool& operator=(const adaptive_pool&);
   /// @endcond

   public:
   //!Constructor from a segment manager. If not present, constructs a node
   //!pool. Increments the reference count of the associated node pool.
   //!Can throw boost::interprocess::bad_alloc
   adaptive_pool(segment_manager *segment_mngr) 
      : mp_node_pool(priv_get_or_create(segment_mngr)) { }

   //!Copy constructor from other adaptive_pool. Increments the reference 
   //!count of the associated node pool. Never throws
   adaptive_pool(const adaptive_pool &other) 
      : mp_node_pool(other.get_node_pool()) 
   {  
      mp_node_pool->inc_ref_count();   
   }

   //!Copy constructor from related adaptive_pool. If not present, constructs
   //!a node pool. Increments the reference count of the associated node pool.
   //!Can throw boost::interprocess::bad_alloc
   template<class T2>
   adaptive_pool
      (const adaptive_pool<T2, SegmentManager, NodesPerChunk, MaxFreeChunks> &other)
      : mp_node_pool(priv_get_or_create(other.get_segment_manager())) { }

   //!Destructor, removes node_pool_t from memory
   //!if its reference count reaches to zero. Never throws
   ~adaptive_pool() 
      {     priv_destroy_if_last_link();   }

   //!Returns a pointer to the node pool.
   //!Never throws
   node_pool_t* get_node_pool() const
   {  return detail::get_pointer(mp_node_pool);   }

   //!Returns the segment manager.
   //!Never throws
   segment_manager* get_segment_manager()const
   {  return mp_node_pool->get_segment_manager();  }

   //!Returns the number of elements that could be allocated.
   //!Never throws
   size_type max_size() const
   {  return this->get_segment_manager()->get_size()/sizeof(value_type);  }

   //!Allocate memory for an array of count elements. 
   //!Throws boost::interprocess::bad_alloc if there is no enough memory*/
   pointer allocate(size_type count, cvoid_pointer = 0)
   {  
      if(count > ((size_type)-1)/sizeof(value_type))
         throw bad_alloc();
      return pointer(static_cast<T*>(mp_node_pool->allocate(count)));
   }

   //!Deallocate allocated memory.
   //!Never throws
   void deallocate(const pointer &ptr, size_type count)
   {  mp_node_pool->deallocate(detail::get_pointer(ptr), count);  }

   //!Deallocates all free chunks of the pool
   void deallocate_free_chunks()
   {  mp_node_pool->deallocate_free_chunks();   }

   //!Swaps allocators. Does not throw. If each allocator is placed in a
   //!different memory segment, the result is undefined.
   friend void swap(self_t &alloc1, self_t &alloc2)
   {  detail::do_swap(alloc1.mp_node_pool, alloc2.mp_node_pool);  }

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
   //!Object function that creates the node allocator if it is not created and
   //!increments reference count if it is already created
   struct get_or_create_func
   {
      typedef detail::shared_adaptive_node_pool
               <SegmentManager, mutex_type, sizeof(T), NodesPerChunk, MaxFreeChunks>   node_pool_t;

      //!This connects or constructs the unique instance of node_pool_t
      //!Can throw boost::interprocess::bad_alloc
      void operator()()
      {
         //Find or create the node_pool_t
         mp_node_pool =    mp_named_alloc->template find_or_construct
                           <node_pool_t>(unique_instance)(mp_named_alloc);
         //If valid, increment link count
         if(mp_node_pool != 0)
            mp_node_pool->inc_ref_count();
      }

      //!Constructor. Initializes function
      //!object parameters
      get_or_create_func(segment_manager *hdr) : mp_named_alloc(hdr){}
      
      node_pool_t      *mp_node_pool;
      segment_manager     *mp_named_alloc;
   };

   //!Initialization function, creates an executes atomically the 
   //!initialization object functions. Can throw boost::interprocess::bad_alloc
   node_pool_t *priv_get_or_create(segment_manager *named_alloc)
   {
      get_or_create_func func(named_alloc);
      named_alloc->atomic_func(func);
      return func.mp_node_pool;
   }

   //!Object function that decrements the reference count. If the count 
   //!reaches to zero destroys the node allocator from memory. 
   //!Never throws
   struct destroy_if_last_link_func
   {
      typedef detail::shared_adaptive_node_pool
               <SegmentManager, mutex_type,sizeof(T), NodesPerChunk, MaxFreeChunks>   node_pool_t;

      //!Decrements reference count and destroys the object if there is no 
      //!more attached allocators. Never throws
      void operator()()
      {
         //If not the last link return
         if(mp_node_pool->dec_ref_count() != 0) return;

         //Last link, let's destroy the segment_manager
         mp_named_alloc->template destroy<node_pool_t>(unique_instance); 
      }  

      //!Constructor. Initializes function
      //!object parameters
      destroy_if_last_link_func(segment_manager    *nhdr,
                                node_pool_t *phdr) 
                            : mp_named_alloc(nhdr), mp_node_pool(phdr){}

      segment_manager   *mp_named_alloc;     
      node_pool_t       *mp_node_pool;
   };

   //!Destruction function, initializes and executes destruction function 
   //!object. Never throws
   void priv_destroy_if_last_link()
   {
      typedef detail::shared_adaptive_node_pool
               <SegmentManager, mutex_type,sizeof(T), NodesPerChunk, MaxFreeChunks>   node_pool_t;
      //Get segment manager
      segment_manager *named_segment_mngr = this->get_segment_manager();
      //Execute destruction functor atomically
      destroy_if_last_link_func func(named_segment_mngr, detail::get_pointer(mp_node_pool));
      named_segment_mngr->atomic_func(func);
   }

   private:
   node_pool_ptr   mp_node_pool;
   /// @endcond
};

//!Equality test for same type
//!of adaptive_pool
template<class T, class S, std::size_t NodesPerChunk, std::size_t F> inline
bool operator==(const adaptive_pool<T, S, NodesPerChunk, F> &alloc1, 
                const adaptive_pool<T, S, NodesPerChunk, F> &alloc2)
   {  return alloc1.get_node_pool() == alloc2.get_node_pool(); }

//!Inequality test for same type
//!of adaptive_pool
template<class T, class S, std::size_t NodesPerChunk, std::size_t F> inline
bool operator!=(const adaptive_pool<T, S, NodesPerChunk, F> &alloc1, 
                const adaptive_pool<T, S, NodesPerChunk, F> &alloc2)
   {  return alloc1.get_node_pool() != alloc2.get_node_pool(); }


}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_ADAPTIVE_POOL_HPP

