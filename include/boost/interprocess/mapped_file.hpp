//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_MAPPED_FILE_HPP
#define BOOST_INTERPROCESS_MAPPED_FILE_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/creation_tags.hpp>
#include <boost/interprocess/memory_mapping.hpp>
#include <boost/interprocess/detail/global_lock.hpp>
#include <boost/interprocess/detail/os_file_functions.hpp>
#include <memory>    //std::auto_ptr
#include <fstream>   //std::fstream
#include <string>    //std::string
#include <cstdio>    //std::remove
#include <boost/noncopyable.hpp>

#include <string>

#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
#  include <boost/interprocess/sync/win32/win32_sync_primitives.hpp>
#else
#  ifdef BOOST_HAS_UNISTD_H
#    include <fcntl.h>
#    include <sys/mman.h>     //mmap, off64_t
#    include <unistd.h>
#    include <sys/stat.h>
#  else
#    error Unknown platform
#  endif

#endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

/*!\file
   Describes file_mapping and mapped region classes
*/

namespace boost {

namespace interprocess {

/*!A class that wraps a file-mapping that can be used to
   create mapped regions from the mapped files*/
class file_mapping : public memory_mapping
{
   public:
   typedef memory_mapping::accessmode_t      accessmode_t;
   typedef memory_mapping::mapping_handle_t  mapping_handle_t;

   /*!Opens a file mapping of file "filename", starting in offset 
      "file_offset", and the mapping's size will be "size". The mapping 
      can be opened for read-only "ro_mode" or read-write "rw_mode. modes.*/
   file_mapping(const char *filename, accessmode_t mode);

   /*!Calls close. Does not throw*/
   ~file_mapping();

   /*!Returns the name of the file.*/
   const char *get_name() const;

   private:
   /*!Closes a previously opened file mapping. Never throws.*/
   void priv_close();

   std::string    m_filename;
   accessmode_t   m_mode;
};

inline file_mapping::~file_mapping() 
{  this->priv_close(); }

/*!Returns the name of the file.*/
inline const char *file_mapping::get_name() const
{  return m_filename.c_str(); }

#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

inline file_mapping::file_mapping
   (const char *filename, memory_mapping::accessmode_t mode)
   :  memory_mapping()
   ,  m_filename(filename)
   ,  m_mode(mode)
{
   unsigned long file_access  = 0;
   unsigned long file_creation_flags  = 0;

   //Set accesses
   if (mode == rw_mode){
      file_access       |= winapi::generic_read | winapi::generic_write;
   }
   else if (mode == ro_mode){
      file_access       |= winapi::generic_read;
   }

   file_creation_flags = winapi::open_existing;

   //Open file using windows API since we need the handle
   memory_mapping::mapping_handle_t hnd =
      winapi::create_file(filename, file_access, file_creation_flags);

   //Check for error
   if(hnd == winapi::invalid_handle_value){
      error_info err = winapi::get_last_error();
      this->priv_close();
      throw interprocess_exception(err);
   }

   memory_mapping::assign_data(hnd, mode);
}

inline void file_mapping::priv_close()
{
   if(memory_mapping::get_mapping_handle() != winapi::invalid_handle_value){
      winapi::close_handle(memory_mapping::get_mapping_handle());
      memory_mapping::assign_data
         (winapi::invalid_handle_value, memory_mapping::get_mode());
   }
}

#else    //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

inline file_mapping::file_mapping
   (const char *filename,  memory_mapping::accessmode_t mode)
   :  m_filename(filename)
   ,  m_mode(mode)
{
   //Create new mapping
   int oflag   = O_CREAT;

   if(mode == ro_mode){
      oflag |= O_RDONLY;
   }
   else if(mode == rw_mode){
      oflag |= O_RDWR;
   }
   else{
      error_info err(mode_error);
      throw interprocess_exception(err);
   }

   //Open file and get handle
   int hnd = ::open(filename, // filename
                    oflag,    // read/write access
                    S_IRWXO | S_IRWXG | S_IRWXU);   // permissions
   if(hnd == -1){
      error_info err = system_error_code();
      this->priv_close();
      throw interprocess_exception(err);
   }
   memory_mapping::assign_data(hnd, mode);
}

inline void file_mapping::priv_close()
{
   if(memory_mapping::get_mapping_handle() != -1){
      ::close(memory_mapping::get_mapping_handle());
      memory_mapping::assign_data(-1, memory_mapping::get_mode());
   }
}

#endif   //##if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

/*!A class that wraps basic file-mapping management*/
class mapped_file : private boost::noncopyable
{
 public:

   /*!Creates a memory mapped file with name "name", and size "size".
      If the file was previously created it throws an error.
      The mapping can be opened in read-only "ro_mode" or read-write "rw_mode" modes.
      The user can also specify the mapping address in "addr". If "addr" is 0,
      the operating system will choose the mapping address.

      This function can throw boost::inteprocess_exception and
      other standard exceptions derived from std::exception
      (like std::bad_alloc)*/
   mapped_file(detail::create_only_t, 
               const char *name,
               std::size_t size,
               memory_mapping::accessmode_t mode = memory_mapping::rw_mode,
               const void *addr = 0);

   /*!Creates a memory mapped file with name "name", and size "size" if
      the file was not previously created. If it was previously 
      created it tries to open it.
      The file can be opened in read-only "ro_mode" or read-write "rw_mode" modes.
      The user can also specify the mapping address in "addr". If "addr" is 0,
      the operating system will choose the mapping address.

      This function can throw boost::inteprocess_exception and
      other standard exceptions derived from std::exception
      (like std::bad_alloc)*/
   mapped_file(detail::open_only_t, 
               const char *name,
               memory_mapping::accessmode_t mode = memory_mapping::rw_mode,
               const void *addr = 0);

   /*!Creates a shared memory segment with name "name", and size "size" if
      the shared memory was not previously created. If it was previously 
      created it tries to open it. 
      The file can be opened in read-only "ro_mode" or read-write "rw_mode" modes.
      The user can specify the mapping address in "addr".
      If "addr" is 0, the operating system will choose the mapping address.

      This function can throw boost::inteprocess_exception and
      other standard exceptions derived from std::exception
      (like std::bad_alloc)*/
   mapped_file(detail::open_or_create_t, 
               const char *name,
               std::size_t size,
               memory_mapping::accessmode_t mode = memory_mapping::rw_mode,
               const void *addr = 0);

   /*!Creates a shared memory segment with name "name", with size "size".
      The file can be opened in read-only "ro_mode" or read-write "rw_mode" modes.
      The user must specify the mapping address in "addr".
      If "addr" is 0, the operating system will choose the mapping address.
      It also executes a copy of the functor "construct_func" atomically if
      the segment is created. The functor must have the following signature:

      bool operator()(const mapped_file_info_t * info, bool created)
      
      "info" is an initialized segment info structure, and "created" 
      must be "true". If the functor returns "false", or throws an error 
      is supposed and the shared memory won't be created.
      The user must specify a destroy_func, that will be copied in this
      constructor and executed in the destructor, before unmapping the
      shared memory segment.
      The functor must have the following signature:

      void operator()(const mapped_file_info_t * info, bool last) const
      
      "info" is an initialized segment info structure, and "last" 
      indicates if this unmapping is the last unmapping so that
      there will be no no other processes attached to the segment.
      The functor must NOT throw.

      This function can throw if copying any functor throws, 
      the construct_func execution throws, and it 
      will throw boost::inteprocess_exception and
      other standard exceptions derived from std::exception
      (like std::bad_alloc) on any error*/
   template <class ConstructFunc>
   mapped_file(detail::create_only_t, 
               const char *name,
               std::size_t size,
               memory_mapping::accessmode_t mode,
               const void *addr,
               const ConstructFunc &construct_func);

   /*!Creates a shared memory segment with name "name", and size "size" if
      the shared memory was not previously created. If it was previously 
      created tries to open it.
      The file can be opened in read-only "ro_mode" or read-write "rw_mode" modes.
      The user must specify the mapping address in "addr".
      If "addr" is 0, the operating system will choose the mapping address.
      It also executes a copy of the functor "construct_func" atomically
      if the segment is opened. The functor must have the following signature:

      bool operator()(const mapped_file_info_t * info, bool created)
      
      "info" is an initialized segment info structure, and "created" 
      must be "false". If the functor returns "false", or throws an error 
      is supposed and the shared memory won't be opened.
      The user must specify a destroy_func, that will be copied in this
      constructor and executed in the destructor, before unmapping the
      shared memory segment.
      The functor must have the following signature:

      void operator()(const mapped_file_info_t * info, bool last) const
      
      "info" is an initialized segment info structure, and "last" 
      indicates if this unmapping is the last unmapping so that
      there will be no no other processes attached to the segment.
      The functor must NOT throw.

      This function can throw if copying any functor throws, 
      the construct_func execution throws, and it 
      will throw boost::inteprocess_exception and
      other standard exceptions derived from std::exception
      (like std::bad_alloc) on any error*/
   template <class ConstructFunc>
   mapped_file(detail::open_only_t, 
               const char *name,
               memory_mapping::accessmode_t mode,
               const void *addr,
               const ConstructFunc &construct_func);

   /*!Creates a shared memory segment with name "name", and size "size" if
      the shared memory was not previously created. If it was previously 
      created it tries to open it.
      The file can be opened in read-only "ro_mode" or read-write "rw_mode" modes.
      The user must specify the mapping address in "addr".
      If "addr" is 0, the operating system will choose the mapping address.
      It also executes the functor "construct_func" atomically if the segment is 
      created or opened. The functor must have the following signature:

      bool operator()(const mapped_file_info_t * info, bool created)
      
      "info" is an initialized segment info structure, and "created" 
      will be "true" if the shared memory was created. If the functor 
      returns "false", or throws an error is supposed and the
      shared memory won't be opened.
      The user must specify a destroy_func, that will be copied in this
      constructor and executed in the destructor, before unmapping the
      shared memory segment.
      The functor must have the following signature:

      void operator()(const mapped_file_info_t * info, bool last) const
      
      "info" is an initialized segment info structure, and "last" 
      indicates if this unmapping is the last unmapping so that
      there will be no no other processes attached to the segment.
      The functor must NOT throw.

      This function can throw if copying any functor throws, 
      the construct_func execution throws, and it 
      will throw boost::inteprocess_exception and
      other standard exceptions derived from std::exception
      (like std::bad_alloc) on any error*/
   template <class ConstructFunc>
   mapped_file(detail::open_or_create_t, 
               const char *name,
               std::size_t size,
               memory_mapping::accessmode_t mode,
               const void *addr,
               const ConstructFunc &construct_func);

   /*!Destructor. Unmaps the file from process' address space.
      It also executes the destruction functor atomically if
      the user has registered that the destruction functor
      in the constructor of this class.
      Never throws.*/
   ~mapped_file();

   /*!Returns the size of the file mapping. Never throws.*/
   std::size_t get_size() const;

   /*!Returns the base address of the file mapping. Never throws.*/
   void*    get_address() const;

   /*!Flushes to the disk the whole file.
      Never throws*/
   bool     flush();

   /*!Returns the name of the shared memory segment used in the
      constructor. Never throws.*/
   const char *get_name() const;

   private:
   enum type_t {   DoCreate, DoOpen, DoCreateOrOpen  };

   template <class ConstructFunc>
   bool priv_open_or_create(type_t type,
                            const char *name,
                            std::size_t size,
                            memory_mapping::accessmode_t mode, 
                            const void *addr,
                            ConstructFunc construct_func);

   //Members
   mapped_region     m_mapped_region;
   std::string       m_name;
};

inline
mapped_file::mapped_file
                  (detail::create_only_t, 
                  const char *name,
                  std::size_t size,
                  memory_mapping::accessmode_t mode,
                  const void *addr)
{
   this->priv_open_or_create(DoCreate, name, size, mode, addr, null_mapped_region_function());
}

inline
mapped_file::mapped_file
                  (detail::open_only_t, 
                  const char *name,
                  memory_mapping::accessmode_t mode,
                  const void *addr)
{
   this->priv_open_or_create(DoOpen, name, 0, mode, addr, null_mapped_region_function());
}

inline
mapped_file::mapped_file
                  (detail::open_or_create_t, 
                  const char *name,
                  std::size_t size,
                  memory_mapping::accessmode_t mode,
                  const void *addr)
{
   this->priv_open_or_create(DoCreateOrOpen, name, size, mode, addr, null_mapped_region_function());
}

template <class ConstructFunc> inline
mapped_file::mapped_file(detail::create_only_t, 
                  const char *name,
                  std::size_t size,
                  memory_mapping::accessmode_t mode,
                  const void *addr,
                  const ConstructFunc &construct_func)
{
   this->priv_open_or_create(DoCreate, name, size, mode, addr, construct_func);
}

template <class ConstructFunc> inline
mapped_file::mapped_file(detail::open_only_t, 
                  const char *name,
                  memory_mapping::accessmode_t mode,
                  const void *addr,
                  const ConstructFunc &construct_func)
{
   this->priv_open_or_create(DoOpen, name, 0, mode, addr, construct_func);
}


template <class ConstructFunc> inline
mapped_file::mapped_file(detail::open_or_create_t, 
                  const char *name,
                  std::size_t size,
                  memory_mapping::accessmode_t mode,
                  const void *addr,
                  const ConstructFunc &construct_func)
{
   this->priv_open_or_create(DoCreateOrOpen, name, size, mode, addr, construct_func);
}


inline
mapped_file::~mapped_file()
{}

std::size_t mapped_file::get_size() const
{  return m_mapped_region.get_size();  }

void*    mapped_file::get_address() const
{  return m_mapped_region.get_address();   }

bool     mapped_file::flush()
{  return m_mapped_region.flush();  }

inline const char *mapped_file::get_name()  const  
{  return m_name.c_str();  }

template <class ConstructFunc> inline
bool mapped_file::priv_open_or_create
   (mapped_file::type_t type, const char *name,
   std::size_t size, memory_mapping::accessmode_t mode,
   const void *addr, ConstructFunc construct_func)
{
   error_info err;
   bool created   = true;

   m_name = name;

   //This global interprocess_mutex guarantees synchronized creation/open logic
   detail::global_lock mut;
   if(!mut.acquire()){
      #ifdef BOOST_NO_EXCEPTIONS
      return false;      
      #else
      throw interprocess_exception(err);
      #endif
   }

   if(type == DoOpen){
      //Open existing shared memory
      created = false;
   }
   else if(type == DoCreateOrOpen || type == DoCreate){
      detail::OS_file_handle_t hnd = detail::create_new_file(name);
      if(hnd != detail::invalid_file()){
         if(!detail::truncate_file(hnd, size)){
            err = system_error_code();
            detail::close_file(hnd);
            std::remove(name);
            #ifdef BOOST_NO_EXCEPTIONS
            return false;      
            #else
            throw interprocess_exception(err);
            #endif
         }
         //Free resources
         if(!detail::close_file(hnd)){
            err = system_error_code();
            #ifdef BOOST_NO_EXCEPTIONS
            return false;      
            #else
            throw interprocess_exception(err);
            #endif
         }
      }
      else if(type == DoCreate){
         err = system_error_code();
         #ifdef BOOST_NO_EXCEPTIONS
         return false;      
         #else
         throw interprocess_exception(err);
         #endif
      }
   }
   else{
      throw interprocess_exception(err);
   }

   //Open file mapping
   file_mapping mapping(name, /*file_mapping::rw_mode*/mode);
   mapped_region mapped(mapping, 0, size, /*file_mapping::rw_mode*/mode, addr);

   //Now swap mapped region
   m_mapped_region.swap(mapped);

   //Execute atomic functor
   construct_func(m_mapped_region, created);

   return true;
}

}  //namespace interprocess {

}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_MAPPED_FILE_HPP

