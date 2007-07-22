//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_CACHED_NODE_ALLOCATOR_HPP
#define BOOST_INTERPROCESS_CACHED_NODE_ALLOCATOR_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/assert.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/interprocess/allocators/detail/node_pool.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <memory>
#include <algorithm>
#include <stdio.h>
#include <cstddef>

/*!\file
   Describes cached_cached_node_allocator pooled shared memory STL compatible allocator 
*/

namespace boost {

namespace interprocess {

/*!An STL node allocator that uses a segment manager as memory 
   source. The internal pointer type will of the same type (raw, smart) as
   "typename SegmentManager::void_pointer" type. This allows
   placing the allocator in shared memory, memory mapped-files, etc...
   This node allocator shares a segregated storage between all instances of 
   cached_node_allocator with equal sizeof(T) placed in the same fixed size 
   memory segment. But also caches some nodes privately to
   avoid some synchronization overhead.*/
template<class T, class SegmentManager, std::size_t NodesPerChunk>
class cached_node_allocator
{
   /// @cond
   typedef typename SegmentManager::void_pointer          void_pointer;
   typedef typename detail::
      pointer_to_other<void_pointer, const void>::type    cvoid_pointer;
   typedef SegmentManager                                 segment_manager;
   typedef typename detail::
      pointer_to_other<void_pointer, char>::type          char_pointer;
   typedef typename SegmentManager::mutex_family::mutex_type mutex_type;
   typedef cached_node_allocator<T, SegmentManager, NodesPerChunk>   self_t;
   enum { DEFAULT_MAX_CACHED_NODES = 64 };
   typedef typename detail::node_slist<void_pointer>::node_t         node_t;
   typedef typename detail::node_slist<void_pointer>::node_slist_t   cached_list_t;
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
   typedef detail::shared_node_pool
      < SegmentManager, mutex_type
      , sizeof(T), NodesPerChunk>                        node_pool_t;
   typedef typename detail::
      pointer_to_other<void_pointer, node_pool_t>::type  node_pool_ptr;

   //!Obtains cached_node_allocator from other cached_node_allocator
   template<class T2>
   struct rebind
   {  
      typedef cached_node_allocator<T2, SegmentManager, NodesPerChunk>   other;
   };

   /// @cond
   private:

   //!Not assignable from related cached_node_allocator
   template<class T2, class SegmentManager2, std::size_t N2>
   cached_node_allocator& operator=
      (const cached_node_allocator<T2, SegmentManager2, N2>&);

   /*!Not assignable from other cached_node_allocator*/
   cached_node_allocator& operator=(const cached_node_allocator&);
   /// @endcond

   public:
   /*!Constructor from a segment manager. If not present, constructs
      a node pool. Increments the reference count of the node pool.
      Can throw boost::interprocess::bad_alloc*/
   cached_node_allocator(segment_manager *segment_mngr,
                         std::size_t max_cached_nodes = DEFAULT_MAX_CACHED_NODES) 
      : mp_node_pool(priv_get_or_create(segment_mngr)),
        m_max_cached_nodes(max_cached_nodes)
   {}

   /*!Copy constructor from other cached_node_allocator. Increments the 
      reference count of the associated node pool. Never throws*/
   cached_node_allocator(const cached_node_allocator &other) 
      : mp_node_pool(other.get_node_pool()),
        m_max_cached_nodes(other.get_max_cached_nodes())
   {  mp_node_pool->inc_ref_count();   }

   /*!Copy constructor from related cached_node_allocator. If not present, constructs
      a node pool. Increments the reference count of the associated node pool.
      Can throw boost::interprocess::bad_alloc*/
   template<class T2>
   cached_node_allocator
      (const cached_node_allocator<T2, SegmentManager, NodesPerChunk> &other)
      : mp_node_pool(priv_get_or_create(other.get_segment_manager())),
        m_max_cached_nodes(other.get_max_cached_nodes())
         { }

   /*!Destructor, removes node_pool_t from memory
      if its reference count reaches to zero. Never throws*/
   ~cached_node_allocator() 
   {     
      priv_deallocate_all_cached_nodes();
      priv_destroy_if_last_link();   
   }

   /*!Returns a pointer to the node pool. Never throws*/
   node_pool_t* get_node_pool() const
   {  return detail::get_pointer(mp_node_pool);   }

   /*!Returns the segment manager. Never throws*/
   segment_manager* get_segment_manager()const
   {  return mp_node_pool->get_segment_manager();  }

   /*!Sets the new max cached nodes value. This can provoke deallocations
      if "newmax" is less than current cached nodes. Never throws*/
   void set_max_cached_nodes(std::size_t newmax)
   {
      m_max_cached_nodes = newmax;
      priv_deallocate_remaining_nodes();
   }

   /*!Returns the max cached nodes parameter. Never throws*/
   std::size_t get_max_cached_nodes() const
      {  return m_max_cached_nodes;  }

   //!Returns the number of elements that could be allocated. Never throws
   size_type max_size() const
      {  return this->get_segment_manager()->get_size()/sizeof(value_type);  }

   /*!Allocate memory for an array of count elements. 
      Throws boost::interprocess::bad_alloc if there is no enough memory*/
   pointer allocate(size_type count, cvoid_pointer hint = 0)
   {
      (void)hint;
      if(count > ((size_type)-1)/sizeof(value_type))
         throw bad_alloc();
      typedef detail::shared_node_pool
               <SegmentManager, mutex_type, sizeof(T), NodesPerChunk>   node_pool_t;
      
      void * ret;
      
      if(count == 1){
         //If don't have any cached node, we have to get a new list of free nodes from the pool
         if(m_cached_nodes.empty()){
            mp_node_pool->allocate_nodes(m_max_cached_nodes/2, m_cached_nodes);
         }
         ret = &m_cached_nodes.front();
         m_cached_nodes.pop_front();
      }
      else{
         ret = mp_node_pool->allocate(count);
      }   
      return pointer(static_cast<T*>(ret));
   }

   /*!Deallocate allocated memory. Never throws*/
   void deallocate(const pointer &ptr, size_type count)
   {
      typedef detail::shared_node_pool
               <SegmentManager, mutex_type, sizeof(T), NodesPerChunk>   node_pool_t;

      if(count == 1){
         //Check if cache is full
         if(m_cached_nodes.size() >= m_max_cached_nodes){
            //This only occurs if this allocator deallocate memory allocated
            //with other equal allocator. Since the cache is full, and more 
            //deallocations are probably coming, we'll make some room in cache
            //in a single, efficient multi node deallocation.
            priv_deallocate_n_nodes(m_cached_nodes.size() - m_max_cached_nodes/2);
         }
         m_cached_nodes.push_front(*(node_t*)detail::char_ptr_cast(detail::get_pointer(ptr)));
      }
      else{
         mp_node_pool->deallocate(detail::get_pointer(ptr), count);
      }
   }

   /*!Swaps allocators. Does not throw. If each allocator is placed in a
      different shared memory segments, the result is undefined.*/
   friend void swap(self_t &alloc1, self_t &alloc2)
   {
      detail::do_swap(alloc1.mp_node_pool,       alloc2.mp_node_pool);
      alloc1.m_cached_nodes.swap(alloc2.m_cached_nodes);
      detail::do_swap(alloc1.m_max_cached_nodes, alloc2.m_max_cached_nodes);
   }

   //!Returns the cached nodes to the shared pool
   void deallocate_cache()
   {  this->priv_deallocate_all_cached_nodes(); }

   //!Deallocates all free chunks of the pool
   void deallocate_free_chunks()
   {  mp_node_pool->deallocate_free_chunks();   }

   //These functions are obsolete. These are here to conserve
   //backwards compatibility with containers using them...

   //!Returns address of mutable object.
   //!Never throws
   pointer address(reference value) const
   {  return pointer(boost::addressof(value));  }

   /*!Returns address of non mutable object. Never throws*/
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

   /*!Object function that creates the node allocator if it is not created and
      increments reference count if it is already created*/
   struct get_or_create_func
   {
      typedef detail::shared_node_pool
               <SegmentManager, mutex_type, sizeof(T), NodesPerChunk>   node_pool_t;

      /*!This connects or constructs the unique instance of node_pool_t
         Can throw boost::interprocess::bad_alloc*/
      void operator()()
      {
         //Find or create the node_pool_t
         mp_node_pool =    mp_named_alloc->template find_or_construct
                           <node_pool_t>(unique_instance)(mp_named_alloc);
         //If valid, increment link count
         if(mp_node_pool != 0)
            mp_node_pool->inc_ref_count();
      }

      /*!Constructor. Initializes function object parameters*/
      get_or_create_func(segment_manager *hdr) : mp_named_alloc(hdr){}
      
      node_pool_t      *mp_node_pool;
      segment_manager  *mp_named_alloc;
   };

   /*!Frees all cached nodes. Never throws*/
   void priv_deallocate_all_cached_nodes()
   {  mp_node_pool->deallocate_nodes(m_cached_nodes); }

   /*!Frees all cached nodes at once. Never throws*/
   void priv_deallocate_remaining_nodes()
   {
      if(m_cached_nodes.size() > m_max_cached_nodes){
         priv_deallocate_n_nodes(m_cached_nodes.size()-m_max_cached_nodes);
      }
   }

   /*!Frees n cached nodes at once. Never throws*/
   void priv_deallocate_n_nodes(std::size_t n)
   {  mp_node_pool->deallocate_nodes(m_cached_nodes, n); }   

   /*!Initialization function, creates an executes atomically the 
      initialization object functions. Can throw boost::interprocess::bad_alloc*/
   node_pool_t *priv_get_or_create(segment_manager *named_alloc)
   {
      get_or_create_func func(named_alloc);
      named_alloc->atomic_func(func);
      return func.mp_node_pool;
   }

   /*!Object function that decrements the reference count. If the count 
      reaches to zero destroys the node allocator from memory. 
      Never throws*/
   struct destroy_if_last_link_func
   {
      typedef detail::shared_node_pool
               <SegmentManager, mutex_type,sizeof(T), NodesPerChunk>   node_pool_t;

      /*!Decrements reference count and destroys the object if there is no 
         more attached allocators. Never throws*/
      void operator()()
      {
         //If not the last link return
         if(mp_node_pool->dec_ref_count() != 0) return;

         //Last link, let's destroy the segment_manager
         mp_named_alloc->template destroy<node_pool_t>(unique_instance); 
      }  

      /*!Constructor. Initializes function object parameters*/
      destroy_if_last_link_func(segment_manager    *nhdr,
                                node_pool_t *phdr) 
                            : mp_named_alloc(nhdr), mp_node_pool(phdr){}

      segment_manager     *mp_named_alloc;     
      node_pool_t      *mp_node_pool;
   };

   /*!Destruction function, initializes and executes destruction function 
      object. Never throws*/
   void priv_destroy_if_last_link()
   {
      typedef detail::shared_node_pool
               <SegmentManager, mutex_type,sizeof(T), NodesPerChunk>   node_pool_t;
      //Get segment manager
      segment_manager *segment_mngr = this->get_segment_manager();
      //Execute destruction functor atomically
      destroy_if_last_link_func func(segment_mngr, detail::get_pointer(mp_node_pool));
      segment_mngr->atomic_func(func);
   }

   private:
   node_pool_ptr  mp_node_pool;
   cached_list_t  m_cached_nodes;
   std::size_t    m_max_cached_nodes;
   /// @endcond
};

/*!Equality test for same type of cached_node_allocator*/
template<class T, class S, std::size_t NodesPerChunk> inline
bool operator==(const cached_node_allocator<T, S, NodesPerChunk> &alloc1, 
                const cached_node_allocator<T, S, NodesPerChunk> &alloc2)
   {  return alloc1.get_node_pool() == alloc2.get_node_pool(); }

/*!Inequality test for same type of cached_node_allocator*/
template<class T, class S, std::size_t NodesPerChunk> inline
bool operator!=(const cached_node_allocator<T, S, NodesPerChunk> &alloc1, 
                const cached_node_allocator<T, S, NodesPerChunk> &alloc2)
   {  return alloc1.get_node_pool() != alloc2.get_node_pool(); }

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_CACHED_NODE_ALLOCATOR_HPP

