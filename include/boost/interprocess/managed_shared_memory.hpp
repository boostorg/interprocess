//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_MANAGED_SHARED_MEMORY_HPP
#define BOOST_INTERPROCESS_MANAGED_SHARED_MEMORY_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/detail/managed_memory_impl.hpp>
#include <boost/interprocess/detail/managed_open_or_create_impl.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/detail/creation_tags.hpp>

/*!\file
   Describes a named shared memory object allocation user class. 
*/

namespace boost {

namespace interprocess {

/*!A basic shared memory named object creation class. Initializes the 
   shared memory segment. Inherits all basic functionality from 
   basic_managed_memory_impl<CharType, AllocationAlgorithm, IndexType>*/
template
      <
         class CharType, 
         class AllocationAlgorithm, 
         template<class IndexConfig> class IndexType
      >
class basic_managed_shared_memory 
   : public detail::basic_managed_memory_impl <CharType, AllocationAlgorithm, IndexType>
{
   /// @cond
   typedef detail::basic_managed_memory_impl 
      <CharType, AllocationAlgorithm, IndexType>   base_t;

   typedef detail::create_open_func<base_t>        create_open_func_t;

   basic_managed_shared_memory();

   basic_managed_shared_memory *get_this_pointer()
   {  return this;   }
   /// @endcond

   public: //functions
   //!Destructor. Calls close. Never throws.
   ~basic_managed_shared_memory()
   {}

   //!Creates shared memory and creates and places the segment manager. 
   //!This can throw.
   basic_managed_shared_memory(detail::create_only_t create_only, const char *name,
                             std::size_t size, const void *addr = 0)
      : m_shmem(create_only, name, size, read_write, addr, 
                create_open_func_t(get_this_pointer(), DoCreate))
   {}

   //!Creates shared memory and creates and places the segment manager if
   //!segment was not created. If segment was created it connects to the
   //!segment.
   //!This can throw.
   basic_managed_shared_memory (detail::open_or_create_t open_or_create,
                              const char *name, std::size_t size, 
                              const void *addr = 0)
      : m_shmem(open_or_create, name, size, read_write, addr, 
                create_open_func_t(get_this_pointer(), 
                DoCreateOrOpen))
   {}

   //!Connects to a created shared memory and it's the segment manager.
   //!Never throws.
   basic_managed_shared_memory (detail::open_only_t open_only, const char* name, 
                              const void *addr = 0)
      : m_shmem(open_only, name, read_write, addr, 
                create_open_func_t(get_this_pointer(), 
                DoOpen))
   {}

   //!Moves the ownership of "moved"'s managed memory to *this. Does not throw
   basic_managed_shared_memory
      (detail::moved_object<basic_managed_shared_memory> &moved)
   {  this->swap(moved.get());   }

   //!Moves the ownership of "moved"'s managed memory to *this. Does not throw
   basic_managed_shared_memory &operator=
      (detail::moved_object<basic_managed_shared_memory> &moved)
   {  this->swap(moved.get());   return *this;  }

   //!Swaps the ownership of the managed shared memories managed by *this and other.
   //!Never throws.
   void swap(basic_managed_shared_memory &other)
   {
      base_t::swap(other);
      m_shmem.swap(other.m_shmem);
   }

   /// @cond
   private:
   detail::managed_open_or_create_impl<shared_memory_object> m_shmem;
   /// @endcond
};

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_MANAGED_SHARED_MEMORY_HPP

