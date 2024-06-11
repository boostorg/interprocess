//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_MANAGED_ROBUST_SHARED_MEMORY_HPP
#define BOOST_INTERPROCESS_MANAGED_ROBUST_SHARED_MEMORY_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/detail/managed_memory_impl.hpp>
#include <boost/interprocess/detail/managed_open_or_create_impl.hpp>
#include <boost/interprocess/nonpersistent_shared_memory_object.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/permissions.hpp>
//These includes needed to fulfill default template parameters of
//predeclarations in interprocess_fwd.hpp
#include <boost/interprocess/mem_algo/rbtree_best_fit.hpp>
#include <boost/interprocess/sync/mutex_family.hpp>

namespace boost {
namespace interprocess {

namespace ipcdetail {

template<class AllocationAlgorithm>
struct nonpersistentmem_open_or_create
{
   typedef  ipcdetail::managed_open_or_create_impl
      < nonpersistent_shared_memory_object, AllocationAlgorithm::Alignment, true, true> type;
};

}  //namespace ipcdetail {

//!A basic non-persistent shared memory named object creation class.
//!It emulates non-persistent shared memory with temporary files on
//!filesystems that support flock-style shared and exclusive locks.
//!When the shared memory is created, and the temporary file still exists
//!but is not accessed by any other process, its contents are cleared.
//!
//!Inherits all basic functionality from
//!basic_managed_memory_impl<CharType, AllocationAlgorithm, IndexType>*/
template
      <
         class CharType,
         class AllocationAlgorithm,
         template<class IndexConfig> class IndexType
      >
class basic_managed_nonpersistent_shared_memory
   : public ipcdetail::basic_managed_memory_impl
      <CharType, AllocationAlgorithm, IndexType
      ,ipcdetail::nonpersistentmem_open_or_create<AllocationAlgorithm>::type::ManagedOpenOrCreateUserOffset>
   , private ipcdetail::nonpersistentmem_open_or_create<AllocationAlgorithm>::type
{
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   typedef ipcdetail::basic_managed_memory_impl
      <CharType, AllocationAlgorithm, IndexType,
      ipcdetail::nonpersistentmem_open_or_create<AllocationAlgorithm>::type::ManagedOpenOrCreateUserOffset>   base_t;
   typedef typename ipcdetail::nonpersistentmem_open_or_create<AllocationAlgorithm>::type                     base2_t;

    template<class BasicManagedMemoryImpl, class Func>
	class create_open_func_callback : public ipcdetail::create_open_func<BasicManagedMemoryImpl> {
		using base_t = ipcdetail::create_open_func<BasicManagedMemoryImpl>;
    public:

        create_open_func_callback(BasicManagedMemoryImpl * const frontend, ipcdetail::create_enum_t type, Func&& fn)
        : base_t(frontend, type), m_func(std::forward<Func>(fn)) {}

        bool operator()(void *addr, std::size_t size, bool created) const {
			if(!base_t::operator()(addr, size, created)) return false; // return on failure
			
            if(created) {
                m_func();
            }
			return true;
        } 
    private:
        typename std::decay<Func>::type m_func;
    };

   basic_managed_nonpersistent_shared_memory *get_this_pointer()
   {  return this;   }

   public:
   typedef nonpersistent_shared_memory_object                    device_type;
   typedef typename base_t::size_type              size_type;

   private:
   typedef typename base_t::char_ptr_holder_t   char_ptr_holder_t;
   BOOST_MOVABLE_BUT_NOT_COPYABLE(basic_managed_nonpersistent_shared_memory)
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   public: //functions

   //!Destroys *this and indicates that the calling process is finished using
   //!the resource. The destructor function will deallocate
   //!any system resources allocated by the system for use by this process for
   //!this resource. The resource can still be opened again calling
   //!the open constructor overload. To erase the resource from the system
   //!use remove().
   ~basic_managed_nonpersistent_shared_memory()
   {}

   //!Default constructor. Does nothing.
   //!Useful in combination with move semantics
   basic_managed_nonpersistent_shared_memory()
   {}

   //!Creates shared memory and creates and places the segment manager if
   //!segment was not created, or if no process is accessing the shared memory. 
   // When the segment was already created, it connects to the segment.
   //!This can throw.
   template<typename Func>
   basic_managed_nonpersistent_shared_memory (open_or_create_t,
                              const char *name, 
                              size_type size,
                              Func&& OnCreation,
                              const void *addr = 0, const permissions& perm = permissions())
      : base_t()
      , base2_t(open_or_create, name, size, read_write, addr,
                create_open_func_callback<base_t, Func>(get_this_pointer(), ipcdetail::DoOpenOrCreate, std::forward<Func>(OnCreation)),
                perm)
   {}

   //!Moves the ownership of "moved"'s managed memory to *this.
   //!Does not throw
   basic_managed_nonpersistent_shared_memory(BOOST_RV_REF(basic_managed_nonpersistent_shared_memory) moved)
   {
      basic_managed_nonpersistent_shared_memory tmp;
      this->swap(moved);
      tmp.swap(moved);
   }

   //!Moves the ownership of "moved"'s managed memory to *this.
   //!Does not throw
   basic_managed_nonpersistent_shared_memory &operator=(BOOST_RV_REF(basic_managed_nonpersistent_shared_memory) moved)
   {
      basic_managed_nonpersistent_shared_memory tmp(boost::move(moved));
      this->swap(tmp);
      return *this;
   }

   //!Swaps the ownership of the managed shared memories managed by *this and other.
   //!Never throws.
   void swap(basic_managed_nonpersistent_shared_memory &other)
   {
      base_t::swap(other);
      base2_t::swap(other);
   }

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //!Tries to find a previous named allocation address. Returns a memory
   //!buffer and the object count. If not found returned pointer is 0.
   //!Never throws.
   template <class T>
   std::pair<T*, size_type> find  (char_ptr_holder_t name)
   {
      if(base2_t::get_mapped_region().get_mode() == read_only){
         return base_t::template find_no_lock<T>(name);
      }
      else{
         return base_t::template find<T>(name);
      }
   }

   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};

#ifdef BOOST_INTERPROCESS_DOXYGEN_INVOKED

//!Typedef for a default basic_managed_shared_memory
//!of narrow characters
typedef basic_managed_nonpersistent_shared_memory
   <char
   ,rbtree_best_fit<mutex_family>
   ,iset_index>
managed_nonpersistent_shared_memory;

//!Typedef for a default basic_managed_shared_memory
//!of wide characters
typedef basic_managed_nonpersistent_shared_memory
   <wchar_t
   ,rbtree_best_fit<mutex_family>
   ,iset_index>
wmanaged_shared_memory;

//!Typedef for a default basic_managed_shared_memory
//!of narrow characters to be placed in a fixed address
typedef basic_managed_nonpersistent_shared_memory
   <char
   ,rbtree_best_fit<mutex_family, void*>
   ,iset_index>
fixed_managed_shared_memory;

//!Typedef for a default basic_managed_shared_memory
//!of narrow characters to be placed in a fixed address
typedef basic_managed_nonpersistent_shared_memory
   <wchar_t
   ,rbtree_best_fit<mutex_family, void*>
   ,iset_index>
wfixed_managed_shared_memory;


#endif   //#ifdef BOOST_INTERPROCESS_DOXYGEN_INVOKED

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_MANAGED_ROBUST_SHARED_MEMORY_HPP

