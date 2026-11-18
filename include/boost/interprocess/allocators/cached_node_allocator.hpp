//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_CACHED_NODE_ALLOCATOR_HPP
#define BOOST_INTERPROCESS_CACHED_NODE_ALLOCATOR_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>

#include <boost/interprocess/allocators/detail/node_pool.hpp>
#include <boost/interprocess/allocators/detail/allocator_common.hpp>
#include <boost/interprocess/allocators/detail/node_tools.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/containers/version_type.hpp>

#include <boost/container/uses_allocator_construction.hpp>

#include <cstddef>

//!\file
//!Describes cached_cached_node_allocator pooled shared memory STL compatible allocator

namespace boost {
namespace interprocess {


#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

namespace ipcdetail {

template < class T
         , class SegmentManager
         , std::size_t NodesPerBlock = 64
         >
class cached_node_allocator_v1
   :  public ipcdetail::cached_allocator_impl
         < T
         , ipcdetail::shared_node_pool
            < SegmentManager
            , sizeof_value<T>::value
            , NodesPerBlock
            , alignof_value<T>::value
            >
         , 1>
{
   BOOST_COPYABLE_AND_MOVABLE_ALT(cached_node_allocator_v1)
   public:
   typedef ipcdetail::cached_allocator_impl
         < T
         , ipcdetail::shared_node_pool
            < SegmentManager
            , sizeof_value<T>::value
            , NodesPerBlock
            , alignof_value<T>::value
            >
         , 1> base_t;
   typedef uses_segment_manager<SegmentManager> uses_segment_manager_t;

   template<class T2>
   struct rebind
   {
      typedef cached_node_allocator_v1
         <T2, SegmentManager, NodesPerBlock>  other;
   };
               
   typedef typename base_t::size_type size_type;

   cached_node_allocator_v1(SegmentManager *segment_mngr,
                         size_type max_cached_nodes = base_t::DEFAULT_MAX_CACHED_NODES)
      : base_t(segment_mngr, max_cached_nodes)
   {}

   cached_node_allocator_v1(uses_segment_manager_t usm,
                            size_type max_cached_nodes = base_t::DEFAULT_MAX_CACHED_NODES)
      : base_t(usm.get_segment_manager(), max_cached_nodes)
   {}

   template<class T2>
   cached_node_allocator_v1
      (const cached_node_allocator_v1
         <T2, SegmentManager, NodesPerBlock> &other)
      : base_t(other)
   {}

   cached_node_allocator_v1(const cached_node_allocator_v1 &other)
      : base_t(other)
   {}

   cached_node_allocator_v1(BOOST_RV_REF(cached_node_allocator_v1) other)
      : base_t(BOOST_MOVE_BASE(base_t, other))
   {}
};

}  //namespace ipcdetail{

#endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

//!An STL node allocator that uses a segment manager as memory
//!source. The internal pointer type will of the same type (raw, smart) as
//!"typename SegmentManager::void_pointer" type. This allows
//!placing the allocator in shared memory, memory mapped-files, etc...
//!
//!This node allocator shares a segregated storage between all instances of
//!cached_node_allocator with equal sizeof(T) placed in the same
//!memory segment. But also caches some nodes privately to
//!avoid some synchronization overhead.
//!
//!NodesPerBlock is the number of nodes allocated at once when the allocator
//!runs out of nodes.
template < class T
         , class SegmentManager
         , std::size_t NodesPerBlock
         >
class cached_node_allocator
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   :  public ipcdetail::cached_allocator_impl
         < T
         , ipcdetail::shared_node_pool
            < SegmentManager
            , sizeof_value<T>::value
            , NodesPerBlock
            , alignof_value<T>::value
            >
         , 2>
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
{

   #ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
   public:
   typedef ipcdetail::cached_allocator_impl
         < T
         , ipcdetail::shared_node_pool
            < SegmentManager
            , sizeof_value<T>::value
            , NodesPerBlock
            , alignof_value<T>::value
            >
         , 2> base_t;

   BOOST_COPYABLE_AND_MOVABLE_ALT(cached_node_allocator)

   public:
   typedef boost::interprocess::version_type<cached_node_allocator, 2>  version;
   typedef typename base_t::size_type                                   size_type;
   typedef uses_segment_manager<SegmentManager>                         uses_segment_manager_t;

   template<class T2>
   struct rebind
   {
      typedef cached_node_allocator<T2, SegmentManager, NodesPerBlock>  other;
   };

   cached_node_allocator(SegmentManager *segment_mngr,
                         size_type max_cached_nodes = base_t::DEFAULT_MAX_CACHED_NODES)
      : base_t(segment_mngr, max_cached_nodes)
   {}

   cached_node_allocator(uses_segment_manager_t usm,
                         size_type max_cached_nodes = base_t::DEFAULT_MAX_CACHED_NODES)
      : base_t(usm.get_segment_manager(), max_cached_nodes)
   {}

   template<class T2>
   cached_node_allocator
      (const cached_node_allocator<T2, SegmentManager, NodesPerBlock> &other)
      : base_t(other)
   {}

   cached_node_allocator(const cached_node_allocator & other)
      : base_t(other)
   {}

   cached_node_allocator(BOOST_RV_REF(cached_node_allocator) other)
      : base_t(BOOST_MOVE_BASE(base_t, other))
   {}

   #else
   public:
   typedef implementation_defined::segment_manager       segment_manager;
   typedef segment_manager::void_pointer                 void_pointer;
   typedef implementation_defined::pointer               pointer;
   typedef implementation_defined::const_pointer         const_pointer;
   typedef T                                             value_type;
   typedef typename ipcdetail::add_reference
                     <value_type>::type                  reference;
   typedef typename ipcdetail::add_reference
                     <const value_type>::type            const_reference;
   typedef typename SegmentManager::size_type            size_type;
   typedef typename SegmentManager::difference_type      difference_type;

   //!Obtains cached_node_allocator from
   //!cached_node_allocator
   template<class T2>
   struct rebind
   {
      typedef cached_node_allocator<T2, SegmentManager> other;
   };

   private:
   //!Not assignable from
   //!related cached_node_allocator
   template<class T2, class SegmentManager2, std::size_t N2>
   cached_node_allocator& operator=
      (const cached_node_allocator<T2, SegmentManager2, N2>&);

   //!Not assignable from
   //!other cached_node_allocator
   cached_node_allocator& operator=(const cached_node_allocator&);

   public:
   //!Constructor from a segment manager. If not present, constructs a node
   //!pool. Increments the reference count of the associated node pool.
   //!Can throw boost::interprocess::bad_alloc
   cached_node_allocator(segment_manager *segment_mngr);

   //!Copy constructor from other cached_node_allocator. Increments the reference
   //!count of the associated node pool. Never throws
   cached_node_allocator(const cached_node_allocator &other);

   //!Move constructor from other. Increments the reference
   //!count of the associated node pool and captures the cache. Never throws
   cached_node_allocator(cached_node_allocator &&other);

   //!Copy constructor from related cached_node_allocator. If not present, constructs
   //!a node pool. Increments the reference count of the associated node pool.
   //!Can throw boost::interprocess::bad_alloc
   template<class T2>
   cached_node_allocator
      (const cached_node_allocator<T2, SegmentManager, NodesPerBlock> &other);

   //!Destructor, removes node_pool_t from memory
   //!if its reference count reaches to zero. Never throws
   ~cached_node_allocator();

   //!Returns a pointer to the node pool.
   //!Never throws
   node_pool_t* get_node_pool() const;

   //!Returns the segment manager.
   //!Never throws
   segment_manager* get_segment_manager()const;

   //!Returns the number of elements that could be allocated.
   //!Never throws
   size_type max_size() const;

   //!Allocate memory for an array of count elements.
   //!Throws boost::interprocess::bad_alloc if there is no enough memory
   pointer allocate(size_type count, cvoid_pointer hint = 0);

   //!Deallocate allocated memory.
   //!Never throws
   void deallocate(const pointer &ptr, size_type count);

   //!Deallocates all free blocks
   //!of the pool
   void deallocate_free_blocks();

   //!Swaps allocators. Does not throw. If each allocator is placed in a
   //!different memory segment, the result is undefined.
   friend void swap(self_t &alloc1, self_t &alloc2);

   //!Returns address of mutable object.
   //!Never throws
   //!This function is deprecated and will be removed in the future
   pointer address(reference value) const;

   //!Returns address of non mutable object.
   //!Never throws
   //!This function is deprecated and will be removed in the future
   const_pointer address(const_reference value) const;

   //! <b>Requires</b>: Uses-allocator construction of T with allocator argument
   //!   `uses_segment_manager_t` and additional constructor arguments `std::forward<Args>(args)...`
   //!   is well-formed. [Note: uses-allocator construction is always well formed for
   //!   types that do not use allocators. - end note]
   //!
   //! <b>Effects</b>: Construct a T object at p by uses-allocator construction with allocator
   //!   argument constructible from `segment_manager*`
   //!  and constructor arguments `std::forward<Args>(args)...`.
   //!
   //! <b>Throws</b>: Nothing unless the constructor for T throws.
   template <typename U, class ...Args>
   void construct(U* p, Args&& ...args);

   //!Destroys object. Throws if object's
   //!destructor throws
   void destroy(const pointer &ptr);

   //!Returns maximum the number of objects the previously allocated memory
   //!pointed by p can hold. This size only works for memory allocated with
   //!allocate, allocation_command and allocate_many.
   //!This function is deprecated and will be removed in the future
   size_type size(const pointer &p) const;

   //!Allocates num_elements buffers. Each buffer has room for elem_size
   //!objects of type T. The buffers are linked in chain.
   //!Deallocate them with deallocate_many(...).
   //!Throws boost::interprocess::bad_alloc if there is not enough memory.
   //!This function is deprecated and will be removed in the future
   void allocate_many(size_type elem_size, size_type num_elements, multiallocation_chain &chain);

   //!Allocates n_elements buffers. Buffer i has room for elem_sizes[i]
   //!objects of type T. The buffers are linked in chain.
   //!Deallocate them with deallocate_many(...).
   //!Throws boost::interprocess::bad_alloc if there is not enough memory.
   //!This function is deprecated and will be removed in the future
   void allocate_many(const size_type *elem_sizes, size_type n_elements, multiallocation_chain &chain);

   //!Deallocates all the buffers linked in chain. The buffers must be
   //!allocated with allocate_many(...). Never throws.
   //!This function is deprecated and will be removed in the future
   void deallocate_many(multiallocation_chain &chain);

   //!Allocates just one object. Memory allocated with this function
   //!must be deallocated only with deallocate_one().
   //!Throws boost::interprocess::bad_alloc if there is no enough memory
   pointer allocate_one();

   //!Allocates num_elements objects of type T and links them in chain.
   //!Deallocate them with deallocate_one() or deallocate_individual().
   //!Throws boost::interprocess::bad_alloc if there is not enough memory.
   void allocate_individual(size_type num_elements, multiallocation_chain &chain);

   //!Deallocates memory previously allocated with allocate_one().
   //!You should never use deallocate_one to deallocate memory allocated
   //!with other functions different from allocate_one(). Never throws
   void deallocate_one(const pointer &p);

   //!Deallocates all the objects linked in chain. The objects must be
   //!allocated with allocate_one() or allocate_individual(). Never throws.
   void deallocate_individual(multiallocation_chain &chain);
   //!Sets the new max cached nodes value. This can provoke deallocations
   //!if "newmax" is less than current cached nodes. Never throws
   void set_max_cached_nodes(size_type newmax);

   //!Returns the max cached nodes parameter.
   //!Never throws
   size_type get_max_cached_nodes() const;
   #endif
};

#ifdef BOOST_INTERPROCESS_DOXYGEN_INVOKED

//!Equality test for same type
//!of cached_node_allocator
template<class T, class S, std::size_t NPC> inline
bool operator==(const cached_node_allocator<T, S, NPC> &alloc1,
                const cached_node_allocator<T, S, NPC> &alloc2);

//!Inequality test for same type
//!of cached_node_allocator
template<class T, class S, std::size_t NPC> inline
bool operator!=(const cached_node_allocator<T, S, NPC> &alloc1,
                const cached_node_allocator<T, S, NPC> &alloc2);

#endif

//!Equality test for cached_node_allocator with different value types.
//!Allocators are equal if the segment managers are equal.
template<class T1, class T2, class S, std::size_t NPC> inline
bool operator==(const cached_node_allocator<T1, S, NPC> &alloc1,
                const cached_node_allocator<T2, S, NPC> &alloc2)
   {  return alloc1.get_segment_manager() == alloc2.get_segment_manager(); }

//!Inequality test for cached_node_allocator with different value types.
//!Allocators are unequal if the segment managers are unequal.
template<class T1, class T2, class S, std::size_t NPC> inline
bool operator!=(const cached_node_allocator<T1, S, NPC> &alloc1,
                const cached_node_allocator<T2, S, NPC> &alloc2)
   {  return alloc1.get_segment_manager() != alloc2.get_segment_manager(); }

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_CACHED_NODE_ALLOCATOR_HPP

