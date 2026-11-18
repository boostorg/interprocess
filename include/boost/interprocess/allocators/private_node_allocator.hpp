//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_PRIVATE_NODE_ALLOCATOR_HPP
#define BOOST_INTERPROCESS_PRIVATE_NODE_ALLOCATOR_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/intrusive/pointer_traits.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/assert.hpp>
#include <boost/container/detail/addressof.hpp>
#include <boost/interprocess/allocators/detail/node_pool.hpp>
#include <boost/interprocess/containers/version_type.hpp>
#include <boost/container/detail/multiallocation_chain.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/mpl.hpp>

#include <boost/container/uses_allocator_construction.hpp>

#include <boost/move/adl_move_swap.hpp>
#include <boost/move/utility_core.hpp>
#include <cstddef>

//!\file
//!Describes private_node_allocator_base pooled shared memory STL compatible allocator

namespace boost {
namespace interprocess {

#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

namespace ipcdetail {

template < unsigned int Version
         , class T
         , class SegmentManager
         , std::size_t NodesPerBlock
         >
class private_node_allocator_base
   : public node_pool_allocation_impl
   < private_node_allocator_base < Version, T, SegmentManager, NodesPerBlock>
   , Version
   , T
   , SegmentManager
   >
{
   BOOST_COPYABLE_AND_MOVABLE_ALT(private_node_allocator_base)
   public:
   //Segment manager
   typedef SegmentManager                                segment_manager;
   typedef typename SegmentManager::void_pointer         void_pointer;
   typedef uses_segment_manager<SegmentManager>          uses_segment_manager_t;

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:
   typedef private_node_allocator_base
      < Version, T, SegmentManager, NodesPerBlock>       self_t;
   typedef ipcdetail::private_node_pool
         < SegmentManager
         , sizeof_value<T>::value
         , NodesPerBlock
         , alignof_value<T>::value
      > node_pool_t;

   BOOST_INTERPROCESS_STATIC_ASSERT((Version <=2));

   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   public:

   typedef typename boost::intrusive::
      pointer_traits<void_pointer>::template
         rebind_pointer<T>::type                         pointer;
   typedef typename boost::intrusive::
      pointer_traits<void_pointer>::template
         rebind_pointer<const T>::type                   const_pointer;
   typedef T                                             value_type;
   typedef typename ipcdetail::add_reference
                     <value_type>::type                  reference;
   typedef typename ipcdetail::add_reference
                     <const value_type>::type            const_reference;
   typedef typename segment_manager::size_type           size_type;
   typedef typename segment_manager::difference_type     difference_type;
   typedef boost::interprocess::version_type
      <private_node_allocator_base, Version>              version;
   typedef boost::container::dtl::transform_multiallocation_chain
      <typename SegmentManager::multiallocation_chain, T>multiallocation_chain;
   //!Each copy of the allocator has its own pool, and the nodes must be deallocated
   //!with the pool that allocated them. So the allocator moves with the nodes
   //!when containers are move assigned or swapped.
   typedef ipcdetail::false_type                         propagate_on_container_copy_assignment;
   typedef ipcdetail::true_type                          propagate_on_container_move_assignment;
   typedef ipcdetail::true_type                          propagate_on_container_swap;
   typedef ipcdetail::false_type                         is_always_equal;

   //!Obtains node_allocator from other node_allocator
   template<class T2>
   struct rebind
   {
      typedef private_node_allocator_base
         <Version, T2, SegmentManager, NodesPerBlock>   other;
   };

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   template <int dummy>
   struct node_pool
   {
      typedef ipcdetail::private_node_pool
         < SegmentManager
         , sizeof_value<T>::value
         , NodesPerBlock
         , alignof_value<T>::value
      > type;

      static type *get(void *p)
      {  return static_cast<type*>(p);  }
   };

   private:
   //!Not assignable from related private_node_allocator_base
   template<unsigned int Version2, class T2, class MemoryAlgorithm2, std::size_t N2>
   private_node_allocator_base& operator=
      (const private_node_allocator_base<Version2, T2, MemoryAlgorithm2, N2>&);

   //!Not assignable from other private_node_allocator_base
   private_node_allocator_base& operator=(const private_node_allocator_base&);
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   public:
   //!Constructor from a segment manager
   private_node_allocator_base(segment_manager *segment_mngr)
      : m_node_pool(segment_mngr)
   {}

   //!Copy constructor from other private_node_allocator_base. Never throws
   private_node_allocator_base(const private_node_allocator_base &other)
      : m_node_pool(other.get_segment_manager())
   {}

   //!Move constructor from other private_node_allocator_base. Never throws
   private_node_allocator_base(BOOST_RV_REF(private_node_allocator_base) other)
      : m_node_pool(boost::move(BOOST_MOVE_TO_LV(other).m_node_pool))
   {}

   //!Move assignment: *this takes the pool of other. The nodes of the pool of *this
   //!must be deallocated before (containers do it before they move assign the
   //!allocator). Never throws
   private_node_allocator_base& operator=(BOOST_RV_REF(private_node_allocator_base) other)
   {
      m_node_pool = boost::move(BOOST_MOVE_TO_LV(other).m_node_pool);
      return *this;
   }

   //!Copy constructor from related private_node_allocator_base. Never throws.
   template<class T2>
   private_node_allocator_base
      (const private_node_allocator_base
         <Version, T2, SegmentManager, NodesPerBlock> &other)
      : m_node_pool(other.get_segment_manager())
   {}

   //!Destructor, frees all used memory. Never throws
   ~private_node_allocator_base()
   {}

   //!Returns the segment manager. Never throws
   segment_manager* get_segment_manager()const
   {  return m_node_pool.get_segment_manager(); }

   #if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) || defined(BOOST_CONTAINER_DOXYGEN_INVOKED)
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
   template < typename U, class ...Args>
   inline void construct(U* p, Args&& ...args)
   {
      boost::container::uninitialized_construct_using_allocator
         (p, uses_segment_manager_t(this->get_segment_manager()), ::boost::forward<Args>(args)...);
   }

   #else // #if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) || defined(BOOST_CONTAINER_DOXYGEN_INVOKED)

   #define BOOST_CONTAINER_ALLOCATORS_PRIVATE_NODE_ALLOCATOR_CONSTRUCT_CODE(N) \
   template < typename U BOOST_MOVE_I##N BOOST_MOVE_CLASSQ##N >\
   void construct(U* p BOOST_MOVE_I##N BOOST_MOVE_UREFQ##N)\
   {\
      boost::container::uninitialized_construct_using_allocator\
         (p, uses_segment_manager_t(this->get_segment_manager()) BOOST_MOVE_I##N BOOST_MOVE_FWDQ##N);\
   }\
   //
   BOOST_MOVE_ITERATE_0TO9(BOOST_CONTAINER_ALLOCATORS_PRIVATE_NODE_ALLOCATOR_CONSTRUCT_CODE)
   #undef BOOST_CONTAINER_ALLOCATORS_PRIVATE_NODE_ALLOCATOR_CONSTRUCT_CODE

   #endif   //#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) || defined(BOOST_CONTAINER_DOXYGEN_INVOKED)

   //!Returns the internal node pool. Never throws
   node_pool_t* get_node_pool() const
   {  return const_cast<node_pool_t*>(&m_node_pool); }

   //!Swaps allocators. Does not throw. If each allocator is placed in a
   //!different shared memory segments, the result is undefined.
   friend void swap(self_t &alloc1,self_t &alloc2)
   {  alloc1.m_node_pool.swap(alloc2.m_node_pool);  }

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:
   node_pool_t m_node_pool;
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};

//!Equality test for same type of private_node_allocator_base
template<unsigned int V, class T, class S, std::size_t NPC> inline
bool operator==(const private_node_allocator_base<V, T, S, NPC> &alloc1,
                const private_node_allocator_base<V, T, S, NPC> &alloc2)
{  return &alloc1 == &alloc2; }

//!Inequality test for same type of private_node_allocator_base
template<unsigned int V, class T, class S, std::size_t NPC> inline
bool operator!=(const private_node_allocator_base<V, T, S, NPC> &alloc1,
                const private_node_allocator_base<V, T, S, NPC> &alloc2)
{  return &alloc1 != &alloc2; }

//!Equality test for private_node_allocator_base with different value types.
//!Always false: a rebound copy creates a new private pool, so memory
//!allocated by one allocator can not be deallocated by the other.
template<unsigned int V, class T1, class T2, class S, std::size_t NPC> inline
bool operator==(const private_node_allocator_base<V, T1, S, NPC> &,
                const private_node_allocator_base<V, T2, S, NPC> &)
{  return false; }

//!Inequality test for private_node_allocator_base with different value types.
//!Always true.
template<unsigned int V, class T1, class T2, class S, std::size_t NPC> inline
bool operator!=(const private_node_allocator_base<V, T1, S, NPC> &,
                const private_node_allocator_base<V, T2, S, NPC> &)
{  return true; }

template < class T
         , class SegmentManager
         , std::size_t NodesPerBlock = 64
         >
class private_node_allocator_v1
   :  public private_node_allocator_base
         < 1
         , T
         , SegmentManager
         , NodesPerBlock
         >
{
   BOOST_COPYABLE_AND_MOVABLE_ALT(private_node_allocator_v1)
   public:
   typedef ipcdetail::private_node_allocator_base
         < 1, T, SegmentManager, NodesPerBlock> base_t;
   typedef uses_segment_manager<SegmentManager> uses_segment_manager_t;

   template<class T2>
   struct rebind
   {
      typedef private_node_allocator_v1<T2, SegmentManager, NodesPerBlock>  other;
   };

   private_node_allocator_v1(SegmentManager *segment_mngr)
      : base_t(segment_mngr)
   {}

   private_node_allocator_v1(uses_segment_manager_t usm)
      : base_t(usm.get_segment_manager())
   {}

   template<class T2>
   private_node_allocator_v1
      (const private_node_allocator_v1<T2, SegmentManager, NodesPerBlock> &other)
      : base_t(other)
   {}

   private_node_allocator_v1(const private_node_allocator_v1 &other)
      : base_t(other)
   {}

   private_node_allocator_v1(BOOST_RV_REF(private_node_allocator_v1) other)
      : base_t(BOOST_MOVE_BASE(base_t, other))
   {}

   //!Move assignment: *this takes the pool of other. The nodes of the pool of *this
   //!must be deallocated before (containers do it before they move assign the
   //!allocator). Never throws
   private_node_allocator_v1& operator=(BOOST_RV_REF(private_node_allocator_v1) other)
   {
      base_t::operator=(BOOST_MOVE_BASE(base_t, other));
      return *this;
   }

   //!Swaps the pools. Never throws
   friend void swap(private_node_allocator_v1 &alloc1, private_node_allocator_v1 &alloc2)
   {  swap(static_cast<base_t&>(alloc1), static_cast<base_t&>(alloc2));  }
};

}  //namespace ipcdetail {

#endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

//!An STL node allocator that uses a segment manager as memory
//!source. The internal pointer type will of the same type (raw, smart) as
//!"typename SegmentManager::void_pointer" type. This allows
//!placing the allocator in shared memory, memory mapped-files, etc...
//!This allocator has its own node pool. NodesPerBlock is the number of nodes allocated
//!at once when the allocator needs runs out of nodes
template < class T
         , class SegmentManager
         , std::size_t NodesPerBlock
         >
class private_node_allocator
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   :  public ipcdetail::private_node_allocator_base
         < 2
         , T
         , SegmentManager
         , NodesPerBlock
         >
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
{

   #ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
   typedef ipcdetail::private_node_allocator_base
         < 2, T, SegmentManager, NodesPerBlock> base_t;

   BOOST_COPYABLE_AND_MOVABLE_ALT(private_node_allocator)
   public:
   typedef boost::interprocess::version_type<private_node_allocator, 2> version;
   typedef uses_segment_manager<SegmentManager>                         uses_segment_manager_t;


   template<class T2>
   struct rebind
   {
      typedef private_node_allocator
         <T2, SegmentManager, NodesPerBlock>  other;
   };

   private_node_allocator(SegmentManager *segment_mngr)
      : base_t(segment_mngr)
   {}

   private_node_allocator(uses_segment_manager_t usm)
      : base_t(usm.get_segment_manager())
   {}

   template<class T2>
   private_node_allocator
      (const private_node_allocator<T2, SegmentManager, NodesPerBlock> &other)
      : base_t(other)
   {}

   private_node_allocator(const private_node_allocator &other)
      : base_t(other)
   {}

   private_node_allocator(BOOST_RV_REF(private_node_allocator) other)
      : base_t(BOOST_MOVE_BASE(base_t, other))
   {}

   //!Move assignment: *this takes the pool of other. The nodes of the pool of *this
   //!must be deallocated before (containers do it before they move assign the
   //!allocator). Never throws
   private_node_allocator& operator=(BOOST_RV_REF(private_node_allocator) other)
   {
      base_t::operator=(BOOST_MOVE_BASE(base_t, other));
      return *this;
   }

   //!Swaps the pools. Never throws
   friend void swap(private_node_allocator &alloc1, private_node_allocator &alloc2)
   {  swap(static_cast<base_t&>(alloc1), static_cast<base_t&>(alloc2));  }

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
   typedef typename segment_manager::size_type           size_type;
   typedef typename segment_manage::difference_type      difference_type;

   //!Obtains private_node_allocator from
   //!private_node_allocator
   template<class T2>
   struct rebind
   {
      typedef private_node_allocator
         <T2, SegmentManager, NodesPerBlock> other;
   };

   private:
   //!Not assignable from
   //!related private_node_allocator
   template<class T2, class SegmentManager2, std::size_t N2>
   private_node_allocator& operator=
      (const private_node_allocator<T2, SegmentManager2, N2>&);

   //!Not assignable from
   //!other private_node_allocator
   private_node_allocator& operator=(const private_node_allocator&);

   public:
   //!Constructor from a segment manager. Constructs a private node pool
   //!that obtains memory from segment_mngr.
   private_node_allocator(segment_manager *segment_mngr);

   //!Copy constructor from other private_node_allocator. Constructs a new, empty
   //!private node pool that uses the segment manager of other. Never throws
   private_node_allocator(const private_node_allocator &other);

   //!Move constructor from other. *this takes the private node pool of other.
   //!Never throws
   private_node_allocator(private_node_allocator &&other);

   //!Copy constructor from related private_node_allocator. Constructs a new, empty
   //!private node pool that uses the segment manager of other. Never throws
   template<class T2>
   private_node_allocator
      (const private_node_allocator<T2, SegmentManager, NodesPerBlock> &other);

   //!Destructor, destroys the private node pool and returns its
   //!memory to the segment manager. Never throws
   ~private_node_allocator();

   //!Move assignment: *this takes the private node pool of other. The nodes of
   //!the pool of *this must be deallocated before (containers do it before
   //!they move assign the allocator). Never throws
   private_node_allocator& operator=(private_node_allocator &&other);

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
   #endif
};

#ifdef BOOST_INTERPROCESS_DOXYGEN_INVOKED

//!Equality test for same type
//!of private_node_allocator
template<class T, class S, std::size_t NodesPerBlock> inline
bool operator==(const private_node_allocator<T, S, NodesPerBlock> &alloc1,
                const private_node_allocator<T, S, NodesPerBlock> &alloc2);

//!Inequality test for same type
//!of private_node_allocator
template<class T, class S, std::size_t NodesPerBlock> inline
bool operator!=(const private_node_allocator<T, S, NodesPerBlock> &alloc1,
                const private_node_allocator<T, S, NodesPerBlock> &alloc2);

//!Equality test for private_node_allocator with different value types.
//!Always false: a rebound copy creates a new private pool.
template<class T1, class T2, class S, std::size_t NodesPerBlock> inline
bool operator==(const private_node_allocator<T1, S, NodesPerBlock> &alloc1,
                const private_node_allocator<T2, S, NodesPerBlock> &alloc2);

//!Inequality test for private_node_allocator with different value types.
//!Always true.
template<class T1, class T2, class S, std::size_t NodesPerBlock> inline
bool operator!=(const private_node_allocator<T1, S, NodesPerBlock> &alloc1,
                const private_node_allocator<T2, S, NodesPerBlock> &alloc2);

#endif

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_PRIVATE_NODE_ALLOCATOR_HPP

