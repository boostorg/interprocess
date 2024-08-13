//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_ROBUST_SHARED_MEMORY_OBJECT_HPP
#define BOOST_INTERPROCESS_ROBUST_SHARED_MEMORY_OBJECT_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/assert.hpp>
#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/os_file_functions.hpp>
#include <boost/interprocess/detail/shared_dir_helpers.hpp>
#include <boost/interprocess/permissions.hpp>
#include <boost/move/adl_move_swap.hpp>
#include <cstddef>
#include <string>

#if defined(BOOST_INTERPROCESS_POSIX_SHARED_MEMORY_OBJECTS)
#  include <fcntl.h>        //O_CREAT, O_*...
#  include <sys/mman.h>     //shm_xxx
#  include <unistd.h>       //ftruncate, close
#  include <sys/stat.h>     //mode_t, S_IRWXG, S_IRWXO, S_IRWXU,
#  if defined(BOOST_INTERPROCESS_RUNTIME_FILESYSTEM_BASED_POSIX_SHARED_MEMORY)
#     if defined(__FreeBSD__)
#        include <sys/sysctl.h>
#     endif
#  endif
#else
//
#endif

//!\file
//!Describes a shared memory object management class.

namespace boost {
namespace interprocess {

//!A class that emulates non-persistent shared memory objects with a temporary file on
//!filesystems that support flock-style shared and exclusive locks.
//!When the shared memory object is created, and the backing file still exists
//!but is not accessed by any other process, its contents are cleared.
class nonpersistent_shared_memory_object
{
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   //Non-copyable and non-assignable
   BOOST_MOVABLE_BUT_NOT_COPYABLE(nonpersistent_shared_memory_object)
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   public:
   //!Default constructor. Represents an empty nonpersistent_shared_memory_object.
   nonpersistent_shared_memory_object();

   //!Tries to create a shared memory object with name "name" and mode "mode", with the
   //!access mode "mode". If the file previously exists, but is not accessed by another process
   //!its contents are cleared. Otherwise throws an error.
   nonpersistent_shared_memory_object(create_only_t, const char *name, mode_t mode, const permissions &perm = permissions())
   {  this->priv_open_or_create(ipcdetail::DoCreate, name, mode, perm);  }

   //!Tries to open a shared memory object with name "name", with the access mode "mode".
   //!If the file does not previously exist, or it exists but is currently not opened by another
   //!process, it throws an error.
   nonpersistent_shared_memory_object(open_only_t, const char *name, mode_t mode)
   {  this->priv_open_or_create(ipcdetail::DoOpen, name, mode, permissions());  }

   //!Moves the ownership of "moved"'s shared memory object to *this.
   //!After the call, "moved" does not represent any shared memory object.
   //!Does not throw
   nonpersistent_shared_memory_object(BOOST_RV_REF(nonpersistent_shared_memory_object) moved)
      :  m_handle(file_handle_t(ipcdetail::invalid_file()))
      ,  m_mode(read_only)
   {  this->swap(moved);   }

   //!Moves the ownership of "moved"'s shared memory to *this.
   //!After the call, "moved" does not represent any shared memory.
   //!Does not throw
   nonpersistent_shared_memory_object &operator=(BOOST_RV_REF(nonpersistent_shared_memory_object) moved)
   {
      nonpersistent_shared_memory_object tmp(boost::move(moved));
      this->swap(tmp);
      return *this;
   }

   //!Swaps the nonpersistent_shared_memory_objects. Does not throw
   void swap(nonpersistent_shared_memory_object &moved);

   //!Erases a shared memory object from the system.
   //!Returns false on error. Never throws
   static bool remove(const char *name);

   //!Destroys *this and indicates that the calling process is finished using
   //!the resource. All mapped regions are still
   //!valid after destruction. The destructor function will deallocate
   //!any system resources allocated by the system for use by this process for
   //!this resource. The resource can still be opened again calling
   //!the open constructor overload. To erase the resource from the system
   //!use remove().
   ~nonpersistent_shared_memory_object();

   //!Returns the name of the shared memory object.
   const char *get_name() const;

   //!Returns true if the size of the shared memory object
   //!can be obtained and writes the size in the passed reference
   bool get_size(offset_t &size) const;

   //!Returns access mode
   mode_t get_mode() const;

   //!Returns mapping handle. Never throws.
   mapping_handle_t get_mapping_handle() const;

   void truncate(offset_t length);

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:

   //!Closes a previously opened file mapping. Never throws.
   void priv_close();

   //!Opens or creates a shared memory object.
   bool priv_open_or_create(ipcdetail::create_enum_t type, const char *filename, mode_t mode, const permissions &perm);

   file_handle_t  m_handle;
   mode_t         m_mode;
   std::string    m_filename;
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};

#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

inline nonpersistent_shared_memory_object::nonpersistent_shared_memory_object()
   :  m_handle(file_handle_t(ipcdetail::invalid_file()))
   ,  m_mode(read_only)
{}

inline nonpersistent_shared_memory_object::~nonpersistent_shared_memory_object()
{  this->priv_close(); }


inline const char *nonpersistent_shared_memory_object::get_name() const
{  return m_filename.c_str(); }

inline bool nonpersistent_shared_memory_object::get_size(offset_t &size) const
{  return ipcdetail::get_file_size((file_handle_t)m_handle, size);  }

inline void nonpersistent_shared_memory_object::swap(nonpersistent_shared_memory_object &other)
{
   boost::adl_move_swap(m_handle, other.m_handle);
   boost::adl_move_swap(m_mode,   other.m_mode);
   m_filename.swap(other.m_filename);
}

inline mapping_handle_t nonpersistent_shared_memory_object::get_mapping_handle() const
{
   return ipcdetail::mapping_handle_from_file_handle(m_handle);
}

inline mode_t nonpersistent_shared_memory_object::get_mode() const
{  return m_mode; }


inline void nonpersistent_shared_memory_object::truncate(offset_t length)
{
   if(!ipcdetail::truncate_file(m_handle, length)){
      error_info err = system_error_code();
      throw interprocess_exception(err);
   }
}

inline bool nonpersistent_shared_memory_object::priv_open_or_create
   (ipcdetail::create_enum_t type, const char *filename, mode_t mode, const permissions &perm)
{
   BOOST_ASSERT(type==ipcdetail::DoCreate || type==ipcdetail::DoOpen);
   
   m_filename = filename;
   m_mode = mode;
	
   std::string shmfile;
   ipcdetail::create_shared_dir_cleaning_old_and_get_filepath(filename, shmfile);

   // Try to create or open file with exclusive lock. If that succeeds we are the first to open that file.
   m_handle = ::open(shmfile.c_str(), ((int)mode) | O_CREAT | O_EXLOCK | O_NONBLOCK, perm.get_permissions());
   if(ipcdetail::invalid_file()==m_handle) {
      if(EWOULDBLOCK != system_error_code()) {
		throw interprocess_exception(system_error_code());
	  }
	  
      if(ipcdetail::DoCreate==type) {
		throw interprocess_exception(already_exists_error);
	  }
	  
      // We are not the first to open backing file, block until we get shared lock instead
	  m_handle = ::open(shmfile.c_str(), ((int)mode) | O_SHLOCK);
      if(ipcdetail::invalid_file()==m_handle) {
        throw interprocess_exception(system_error_code());
      }
   } else {
      if(ipcdetail::DoOpen==type
      // We are the first to open this file. Make sure it's truncated to 0 bytes.
      || !ipcdetail::truncate_file(m_handle, 0)
      // Degrade lock to shared lock when we have truncated file
      // This operation is not atomic. The lock is released before the shared lock is acquired.
      // However, this does not pose a problem:
      // a) Another process might be waiting for a shared lock above and acquires
      //    it before we get the shared lock here. The order of shared locks does not matter.
      // b) When the lock is released another process acquires the exclusive lock again
      //    but this will just truncate the file to size 0 again and then we proceed
      //    with a shared lock here. 
      || 0!=flock(m_handle, LOCK_SH)) 
      {
          auto const err = ipcdetail::DoOpen==type ? error_info(not_found_error) : error_info(system_error_code());
          this->priv_close();
          throw interprocess_exception(err);
      }
   }

   return true;
}

inline void nonpersistent_shared_memory_object::priv_close()
{
   if(m_handle != ipcdetail::invalid_file()){
      ipcdetail::close_file(m_handle);
      m_handle = ipcdetail::invalid_file();
   }
}

#endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_ROBUST_SHARED_MEMORY_OBJECT_HPP
