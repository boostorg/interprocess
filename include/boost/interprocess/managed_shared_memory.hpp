//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
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
   ////////////////////////////////////////////////////////////////////////
   //
   //               Some internal helper structs/functors
   //
   ////////////////////////////////////////////////////////////////////////

   /*!This class defines an operator() that creates a shared memory
      of the requested size. The rest of the parameters are
      passed in the constructor. The class a template parameter
      to be used with create_from_file/create_from_istream functions
      of basic_named_object classes*/
/*
   class segment_creator
   {
      public:
      segment_creator(shared_memory &shmem,
                      const char *mem_name, 
                      const void *addr)
      : m_shmem(shmem), m_mem_name(mem_name), m_addr(addr){}

      void *operator()(std::size_t size)
      {
         if(!m_shmem.create(m_mem_name, size, m_addr))
            return 0;
         return m_shmem.get_address(); 
      }      
      private:
      shared_memory &m_shmem;
      const char *m_mem_name;
      const void *m_addr;
   };
*/
   /*!Functor to execute atomically when opening or creating a shared memory
      segment.*/
   struct create_open_func
   {
      enum type_t {   DoCreate, DoOpen, DoCreateOrOpen  };

      create_open_func(basic_managed_shared_memory * const frontend, type_t type)
         : m_frontend(frontend), m_type(type){}

      bool operator()(void *addr, std::size_t size, bool created) const
      {  
         if(((m_type == DoOpen)   &&  created) || 
            ((m_type == DoCreate) && !created))
            return false;

         if(created)
            return m_frontend->create_impl(addr, size);
         else
            return m_frontend->open_impl  (addr, size);
      }
      basic_managed_shared_memory *m_frontend;
      type_t                       m_type;
   };

   typedef detail::basic_managed_memory_impl 
      <CharType, AllocationAlgorithm, IndexType>                   base_t;

   //Friend declarations
   friend struct basic_managed_shared_memory::create_open_func;

   basic_managed_shared_memory();

   basic_managed_shared_memory *get_this_pointer()
   {  return this;   }

   public: //functions

   /*!Destructor. Calls close. Never throws.*/
   ~basic_managed_shared_memory()
   {}

   /*!Creates shared memory and creates and places the segment manager. 
      This can throw.*/
   basic_managed_shared_memory(detail::create_only_t create_only, const char *name,
                             std::size_t size, const void *addr = 0)
      : m_shmem(create_only, name, size, read_write, addr, 
                create_open_func(get_this_pointer(), create_open_func::DoCreate))
   {}

   /*!Creates shared memory and creates and places the segment manager if
      segment was not created. If segment was created it connects to the
      segment.
      This can throw.*/
   basic_managed_shared_memory (detail::open_or_create_t open_or_create,
                              const char *name, std::size_t size, 
                              const void *addr = 0)
      : m_shmem(open_or_create, name, size, read_write, addr, 
                create_open_func(get_this_pointer(), 
                create_open_func::DoCreateOrOpen))
   {}

   /*!Connects to a created shared memory and it's the segment manager.
      Never throws.*/
   basic_managed_shared_memory (detail::open_only_t open_only, const char* name, 
                              const void *addr = 0)
      : m_shmem(open_only, name, read_write, addr, 
                create_open_func(get_this_pointer(), 
                create_open_func::DoOpen))
   {}

   /*!Moves the ownership of "moved"'s managed memory to *this. Does not throw*/
   basic_managed_shared_memory
      (detail::moved_object<basic_managed_shared_memory> &moved)
   {  this->swap(moved.get());   }

   /*!Moves the ownership of "moved"'s managed memory to *this. Does not throw*/
   basic_managed_shared_memory &operator=
      (detail::moved_object<basic_managed_shared_memory> &moved)
   {  this->swap(moved.get());   return *this;  }

   /*!Swaps the ownership of the managed shared memories managed by *this and other.
      Never throws.*/
   void swap(basic_managed_shared_memory &other)
   {
      base_t::swap(other);
      m_shmem.swap(other.m_shmem);
   }
/*
   static bool remove(const char *name)
   {  return shared_memory_object::remove(name);  }
*/
   /*!Creates shared memory from file. Never throws.*/
/*
   template<class CharT> 
   bool create_from_file (const CharT *filename, const char *mem_name, 
                          const void *addr = 0)
   {
      segment_creator mem_creator(m_shmem, mem_name, addr);
      return base_t::create_from_file(filename, mem_creator);
   }
*/
   /*!Creates shared memory from an istream. Never throws.*/
/*
   bool create_from_istream (std::istream &instream, std::size_t size,
                             const char *mem_name, const void *addr = 0)
   {
      segment_creator mem_creator(m_shmem, mem_name, addr);
      return base_t::create_from_istream(instream, size, mem_creator);
   }
*/

   private:
   detail::managed_open_or_create_impl<shared_memory_object> m_shmem;
};

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_MANAGED_SHARED_MEMORY_HPP

