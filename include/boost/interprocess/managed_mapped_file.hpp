//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
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
#include <boost/interprocess/detail/managed_memory_impl.hpp>
#include <boost/interprocess/mapped_file.hpp>
#include <string>
#include <fstream>

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
 private:

   typedef detail::basic_managed_memory_impl 
      <CharType, AllocationAlgorithm, IndexType>             base_t;

   struct create_open_func
   {
      enum type_t {   DoCreate, DoOpen, DoCreateOrOpen  };

      create_open_func(basic_managed_mapped_file * const frontend, type_t type)
         : m_frontend(frontend), m_type(type){}

      bool operator()(const mapped_region &region, bool created) const
      {  
         if(((m_type == DoOpen)   &&  created) || 
            ((m_type == DoCreate) && !created))
            return false;

         if(created)
            return m_frontend->create_impl(region.get_address(), region.get_size());
         else
            return m_frontend->open_impl  (region.get_address(), region.get_size());
      }
      basic_managed_mapped_file *m_frontend;
      type_t                     m_type;
   };

   basic_managed_mapped_file *get_this_pointer()
   {  return this;   }

   public: //functions

   typedef enum { 
                  ro_mode = file_mapping::ro_mode, 
                  rw_mode = file_mapping::rw_mode,
                }    accessmode_t;

   /*!Creates shared memory and creates and places the segment manager. 
      This can throw.*/
   basic_managed_mapped_file(detail::create_only_t create_only, const char *name,
                             std::size_t size, const void *addr = 0)
      : m_mfile(create_only, name, size, memory_mapping::rw_mode, addr, 
                create_open_func(get_this_pointer(), create_open_func::DoCreate))
   {}

   /*!Creates shared memory and creates and places the segment manager if
      segment was not created. If segment was created it connects to the
      segment.
      This can throw.*/
   basic_managed_mapped_file (detail::open_or_create_t open_or_create,
                              const char *name, std::size_t size, 
                              const void *addr = 0)
      : m_mfile(open_or_create, name, size, memory_mapping::rw_mode, addr, 
                create_open_func(get_this_pointer(), 
                create_open_func::DoCreateOrOpen))
   {}

   /*!Connects to a created shared memory and it's the segment manager.
      Never throws.*/
   basic_managed_mapped_file (detail::open_only_t open_only, const char* name, 
                              const void *addr = 0)
      : m_mfile(open_only, name, memory_mapping::rw_mode, addr, 
                create_open_func(get_this_pointer(), 
                create_open_func::DoOpen))
   {}

   /*!Destructor. Never throws.*/
   ~basic_managed_mapped_file()
      {}
/*
   bool create(const char *name, std::size_t size)
   {
      //Create file with given size
      std::ofstream file(name, std::ios::binary | std::ios::trunc);
      file.seekp(static_cast<std::streamoff>(size-1));
      file.write("", 1);
      file.close();

      //Create mapped file
      if(!m_mfile.open(name, 0, size, (file_mapping::accessmode_t)rw_mode)){
         return false;
      }

      //Create Interprocess machinery
      if(!base_t::create_impl(m_mfile.get_address(), size)){
         close();
         return false;
      }

      m_filename = name;
      return true;    
   }

   bool open(const char *name, accessmode_t mode = rw_mode)
   {
      //Open file and get size
      std::ifstream file(name, std::ios::binary);
      std::size_t size = file.seekg(0, std::ios::end).tellg();
      file.close();

      //Create mapped file
      if(!m_mfile.open(name, 0, size, (file_mapping::accessmode_t)mode)){
         return false;
      }

      //Open Interprocess machinery
      if(!base_t::open_impl(m_mfile.get_address(), size)){
         close();
         return false;
      }

      m_filename = name;
      return true;    
   }
*/
   /*!Flushes cached data to file. Never throws*/
   bool flush()
      {  return m_mfile.flush();  }

   /*!Frees resources. Never throws.*/
/*
   void close()
   {  
      base_t::close_impl();
      m_mfile.close();
   }
*/
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
                       (file_mapping::accessmode_t)rw_mode)){
         return false;
      }

      //Grow always works
      base_t::close_impl();
      base_t::open_impl(m_mfile.get_address(), new_size);
      base_t::grow(extra_bytes);
      return true;
   }
*/
   private:
   mapped_file m_mfile;
   std::string       m_filename;
};

}  //namespace interprocess {

}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_MANAGED_MAPPED_FILE_HPP

