//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
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
#include <boost/utility/addressof.hpp>
#include <boost/assert.hpp>
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
template<class T, class SegmentManager>
class cached_node_allocator
{
   enum {   NumAlloc = 64  };
 
   typedef typename SegmentManager::void_pointer          void_pointer;
   typedef typename detail::
      pointer_to_other<void_pointer, const void>::type    cvoid_pointer;
   typedef SegmentManager                                 segment_manager;
   typedef typename detail::
      pointer_to_other<void_pointer, char>::type          char_pointer;
   typedef typename SegmentManager::mutex_family::mutex_t mutex_t;
   typedef cached_node_allocator<T, SegmentManager>       self_t;
   enum { DEFAULT_MAX_CACHED_NODES = 64 };

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

   /*!Obtains cached_node_allocator from other cached_node_allocator*/
   template<class T2>
   struct rebind
   {  
      typedef cached_node_allocator<T2, SegmentManager>   other;
   };

 private:

   /*!Not assignable from related cached_node_allocator*/
   template<class T2, class AllogAlgo2>
   cached_node_allocator& operator=
      (const cached_node_allocator<T2, AllogAlgo2>&);

   /*!Not assignable from other cached_node_allocator*/
   cached_node_allocator& operator=(const cached_node_allocator&);

 public:
   /*!Constructor from a segment manager. If not present, constructs
      a node pool. Increments the reference count of the node pool.
      Can throw boost::interprocess::bad_alloc*/
   cached_node_allocator(segment_manager *segment_mngr,
                         std::size_t max_cached_nodes = DEFAULT_MAX_CACHED_NODES) 
      : mp_node_pool(priv_get_or_create(segment_mngr)),
        mp_cached(0),
        m_cached_nodes(0),
        m_max_cached_nodes(max_cached_nodes)
        
        { }

   /*!Copy constructor from other cached_node_allocator. Increments the 
      reference count of the associated node pool. Never throws*/
   cached_node_allocator(const cached_node_allocator &other) 
      : mp_node_pool(other.get_node_pool()),
        mp_cached(0),
        m_cached_nodes(0),
        m_max_cached_nodes(other.get_max_cached_nodes())
   {  
      typedef detail::shared_node_pool
               <SegmentManager, mutex_t, sizeof(T), NumAlloc>   node_pool_t;
      node_pool_t *node_pool = static_cast<node_pool_t*>(other.get_node_pool());
      node_pool->inc_ref_count();   
   }

   /*!Copy constructor from related cached_node_allocator. If not present, constructs
      a node pool. Increments the reference count of the associated node pool.
      Can throw boost::interprocess::bad_alloc*/
   template<class T2>
   cached_node_allocator
      (const cached_node_allocator<T2, SegmentManager> &other)
      : mp_node_pool(priv_get_or_create(other.get_segment_manager())),
        mp_cached(0),
        m_cached_nodes(0), 
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
   void* get_node_pool() const
      {  return detail::get_pointer(mp_node_pool);   }

   /*!Returns the segment manager. Never throws*/
   segment_manager* get_segment_manager()const
   {  
      typedef detail::shared_node_pool
               <SegmentManager, mutex_t, sizeof(T), NumAlloc>   node_pool_t;
      node_pool_t *node_pool = static_cast<node_pool_t*>(detail::get_pointer(mp_node_pool));
      return node_pool->get_segment_manager();  
   }

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
   
   /*!Return address of mutable value. Never throws*/
   pointer address(reference value) const
      {  return pointer(boost::addressof(value));  }

   /*!Return address of non mutable value. Never throws*/
   const_pointer address(const_reference value) const
      {  return const_pointer(boost::addressof(value));  }

   /*!Construct object, calling constructor. 
   Throws if T(const Convertible &) throws*/
   template<class Convertible>
   void construct(const pointer &ptr, const Convertible &value)
      {  new(detail::get_pointer(ptr)) value_type(value);  }

   /*!Destroys object. Throws if object's destructor throws*/
   void destroy(const pointer &ptr)
      {  BOOST_ASSERT(ptr != 0); (*ptr).~value_type(); }

   /*!Returns the number of elements that could be allocated. Never throws*/
   size_type max_size() const
      {  return this->get_segment_manager()->get_size();  }

   /*!Allocate memory for an array of count elements. 
      Throws boost::interprocess::bad_alloc if there is no enough memory*/
   pointer allocate(size_type count, cvoid_pointer hint = 0)
   {  
      typedef detail::shared_node_pool
               <SegmentManager, mutex_t, sizeof(T), NumAlloc>   node_pool_t;
      
      void * ret;
      
      if(count == 1){
         //If don't have any cached node, we have to get a null
         //terminated linked list of n nodes from the pool
         if(!mp_cached){
            node_pool_t *node_pool = static_cast<node_pool_t*>(detail::get_pointer(mp_node_pool));
            mp_cached = detail::char_ptr_cast(node_pool->allocate_nodes(m_max_cached_nodes/2));
            m_cached_nodes = m_max_cached_nodes/2;
         }
         //Get the first node
         ret = detail::get_pointer(mp_cached);
         mp_cached = node_pool_t::next_node(detail::get_pointer(mp_cached));
         --m_cached_nodes;
      }
      else{
         node_pool_t *node_pool = static_cast<node_pool_t*>(detail::get_pointer(mp_node_pool));
         ret = node_pool->allocate(count);
      }   
      return pointer(static_cast<T*>(ret));
   }

   /*!Deallocate allocated memory. Never throws*/
   void deallocate(const pointer &ptr, size_type count)
   {
      typedef detail::shared_node_pool
               <SegmentManager, mutex_t, sizeof(T), NumAlloc>   node_pool_t;

      if(count == 1){
         //Check if cache is full
         if(m_cached_nodes >= m_max_cached_nodes){
            //This only occurs if this allocator deallocate memory allocated
            //with other equal allocator. Since the cache is full, and more 
            //deallocations are probably coming, we'll make some room in cache
            //in a single, efficient multi node deallocation.
            priv_deallocate_n_nodes(m_cached_nodes - m_max_cached_nodes/2);
         }
         char *node = detail::char_ptr_cast(detail::get_pointer(ptr));         
         node_pool_t::next_node(detail::get_pointer(node)) = mp_cached;
         mp_cached        = node;
         ++m_cached_nodes;
      }
      else{
         node_pool_t *node_pool = static_cast<node_pool_t*>
            (detail::get_pointer(mp_node_pool));
         node_pool->deallocate(detail::get_pointer(ptr), count);
      }
   }

   /*!Swaps allocators. Does not throw. If each allocator is placed in a
      different shared memory segments, the result is undefined.*/
   friend void swap(self_t &alloc1, self_t &alloc2)
   {
      detail::do_swap(alloc1.mp_node_pool,       alloc2.mp_node_pool);
      detail::do_swap(alloc1.mp_cached,          alloc2.mp_cached);
      detail::do_swap(alloc1.m_cached_nodes,     alloc2.m_cached_nodes);
      detail::do_swap(alloc1.m_max_cached_nodes, alloc2.m_max_cached_nodes);
   }

   void clear_cache()
      {  this->priv_deallocate_all_cached_nodes(); }

 private:

   /*!Object function that creates the node allocator if it is not created and
      increments reference count if it is already created*/
   struct get_or_create_func
   {
      typedef detail::shared_node_pool
               <SegmentManager, mutex_t, sizeof(T), NumAlloc>   node_pool_t;

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
      segment_manager     *mp_named_alloc;
   };

   /*!Frees all cached nodes. Never throws*/
   void priv_deallocate_all_cached_nodes()
   {
      typedef detail::shared_node_pool
               <SegmentManager, mutex_t, sizeof(T), NumAlloc>   node_pool_t;
      node_pool_t *node_pool = static_cast<node_pool_t*>
         (detail::get_pointer(mp_node_pool));
      node_pool->deallocate_nodes(detail::get_pointer(mp_cached));
      mp_cached = 0;
   }

   /*!Frees all cached nodes at once. Never throws*/
   void priv_deallocate_remaining_nodes()
   {
      if(m_cached_nodes > m_max_cached_nodes){
         priv_deallocate_n_nodes(m_cached_nodes);
      }
   }

   /*!Frees n cached nodes at once. Never throws*/
   void priv_deallocate_n_nodes(std::size_t n)
   {
      //Check n
      if(n==0) return;   
      typedef detail::shared_node_pool
               <SegmentManager, mutex_t, sizeof(T), NumAlloc>   node_pool_t;
      node_pool_t *node_pool = static_cast<node_pool_t*>
         (detail::get_pointer(mp_node_pool));

      //Create a new linked list of n nodes ending in null
      //taking the first n bytes of cached nodes list
      m_cached_nodes -= n;
      char_pointer head = mp_cached, prev;
      while(n--){ 
         prev = mp_cached;   
         mp_cached = node_pool_t::next_node(detail::get_pointer(mp_cached));
      }

      node_pool_t::next_node(detail::get_pointer(prev)) = 0;      

      //Deallocate all new linked list at once
      node_pool->deallocate_nodes(detail::get_pointer(head));
   }   

   /*!Initialization function, creates an executes atomically the 
      initialization object functions. Can throw boost::interprocess::bad_alloc*/
   void *priv_get_or_create(segment_manager *named_alloc)
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
               <SegmentManager, mutex_t,sizeof(T), NumAlloc>   node_pool_t;

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
               <SegmentManager, mutex_t,sizeof(T), NumAlloc>   node_pool_t;
      //Get segment manager
      segment_manager *segment_mngr = this->get_segment_manager();
      //Get pool pointer
      node_pool_t  *node_pool = static_cast<node_pool_t*>
         (detail::get_pointer(mp_node_pool));
      //Execute destruction functor atomically
      destroy_if_last_link_func func(segment_mngr, node_pool);
      segment_mngr->atomic_func(func);
   }

 private:
   // We can't instantiate a pointer like this:
   // detail::shared_node_pool<SegmentManager, mutex_t, 
   //                             sizeof(T), NumAlloc> *mp_node_pool;
   // since it can provoke an early instantiation of T, that could be 
   // incomplete at that moment (for example, a node of a node-based container)
   // This provokes errors on some node based container implementations using
   // this pooled allocator as this allocator type.
   // 
   // Because of this, we will use a void offset pointer and we'll do some 
   //(ugly )casts when needed.
   void_pointer   mp_node_pool;
   char_pointer   mp_cached;       //Ptr to the first cached node
   std::size_t    m_cached_nodes;
   std::size_t    m_max_cached_nodes;
};

/*!Equality test for same type of cached_node_allocator*/
template<class T, class A> inline
bool operator==(const cached_node_allocator<T, A> &alloc1, 
                const cached_node_allocator<T, A> &alloc2)
   {  return alloc1.get_node_pool() == alloc2.get_node_pool(); }

/*!Inequality test for same type of cached_node_allocator*/
template<class T, class A> inline
bool operator!=(const cached_node_allocator<T, A> &alloc1, 
                const cached_node_allocator<T, A> &alloc2)
   {  return alloc1.get_node_pool() != alloc2.get_node_pool(); }

/*!This specialization indicates that the construct function allows
   convertible types to construct the value type. This allows
   storing less allocator instances in containers.*/
template<class T, class SegmentManager>
struct has_convertible_construct
   <boost::interprocess::cached_node_allocator<T, SegmentManager> >
{
   enum {   value = true };
};

}  //namespace interprocess {

}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_CACHED_NODE_ALLOCATOR_HPP

