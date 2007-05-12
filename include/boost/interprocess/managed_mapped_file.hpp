//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_MANAGED_MAPPED_FILE_HPP
#define BOOST_INTERPROCESS_MANAGED_MAPPED_FILE_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/managed_open_or_create_impl.hpp>
#include <boost/interprocess/detail/managed_memory_impl.hpp>
#include <boost/interprocess/detail/creation_tags.hpp>
#include <boost/interprocess/detail/file_wrapper.hpp>
#include <boost/interprocess/detail/move.hpp>

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
class basic_managed_mapped_file 
   : public detail::basic_managed_memory_impl
      <CharType, AllocationAlgorithm, IndexType>
{
   /// @cond
   private:

   typedef detail::basic_managed_memory_impl 
      <CharType, AllocationAlgorithm, IndexType>   base_t;
   typedef detail::create_open_func<base_t>        create_open_func_t;   

   basic_managed_mapped_file *get_this_pointer()
   {  return this;   }
   /// @endcond

   public: //functions
   /*!Creates shared memory and creates and places the segment manager. 
      This can throw.*/
   basic_managed_mapped_file(detail::create_only_t create_only, const char *name,
                             std::size_t size, const void *addr = 0)
      : m_mfile(create_only, name, size, read_write, addr, 
                create_open_func_t(get_this_pointer(), DoCreate))
   {}

   /*!Creates shared memory and creates and places the segment manager if
      segment was not created. If segment was created it connects to the
      segment.
      This can throw.*/
   basic_managed_mapped_file (detail::open_or_create_t open_or_create,
                              const char *name, std::size_t size, 
                              const void *addr = 0)
      : m_mfile(open_or_create, name, size, read_write, addr, 
                create_open_func_t(get_this_pointer(), 
                DoCreateOrOpen))
   {}

   /*!Connects to a created shared memory and it's the segment manager.
      Never throws.*/
   basic_managed_mapped_file (detail::open_only_t open_only, const char* name, 
                              const void *addr = 0)
      : m_mfile(open_only, name, read_write, addr, 
                create_open_func_t(get_this_pointer(), 
                DoOpen))
   {}

   /*!Moves the ownership of "moved"'s managed memory to *this. Does not throw*/
   basic_managed_mapped_file
      (detail::moved_object<basic_managed_mapped_file> &moved)
   {  this->swap(moved.get());   }

   /*!Moves the ownership of "moved"'s managed memory to *this. Does not throw*/
   basic_managed_mapped_file &operator=
      (detail::moved_object<basic_managed_mapped_file> &moved)
   {  this->swap(moved.get());   return *this;  }

   /*!Destructor. Never throws.*/
   ~basic_managed_mapped_file()
   {}

   /*!Swaps the ownership of the managed mapped memories managed by *this and other.
      Never throws.*/
   void swap(basic_managed_mapped_file &other)
   {
      base_t::swap(other);
      m_mfile.swap(other.m_mfile);
   }

   /*!Flushes cached data to file. Never throws*/
   bool flush()
   {  return m_mfile.flush();  }

   /*!Tries to resize mapped file so that we have room for 
      more objects. 
      WARNING: The memory mapping can change. To be able to use
      this function, all pointers constructed in this buffer
      must be offset pointers. Otherwise, the result is undefined.
      Returns true if the growth has been successful, so you will
      have some extra bytes to allocate new objects. If returns 
      false, the heap allocation has failed.*/
/*
   bool grow(std::size_t extra_bytes)
   {  
      //If memory is reallocated, data will
      //be automatically copied
      std::size_t old_size = m_mfile.get_size();
      std::size_t new_size = old_size + extra_bytes;
      m_mfile.close();
      //Increase file size
      {
         std::ofstream file(m_filename.c_str(), 
                            std::ios::binary |std::ios::in | std::ios::out);
         if(!file){
            return false;
         }
         if(!file.seekp(static_cast<std::streamoff>(new_size - 1))){
            return false;
         }
         if(!file.write("", 1)){
            return false;
         }
      }

      if(!m_mfile.open(m_filename.c_str(), 0, new_size, 
                       (file_mapping::mode_t)read_write)){
         return false;
      }

      //Grow always works
      base_t::close_impl();
      base_t::open_impl(m_mfile.get_address(), new_size);
      base_t::grow(extra_bytes);
      return true;
   }
*/
   /// @cond
   private:
   detail::managed_open_or_create_impl<detail::file_wrapper> m_mfile;
   /// @endcond
};

}  //namespace interprocess {

}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_MANAGED_MAPPED_FILE_HPP
