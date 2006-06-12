//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_FULLY_MAPPED_FILE_HPP
#define BOOST_INTERPROCESS_FULLY_MAPPED_FILE_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/detail/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/global_lock.hpp>
#include <boost/interprocess/detail/os_file_functions.hpp>
#include <memory>    //std::auto_ptr
#include <fstream>   //std::fstream
#include <string>    //std::string
#include <cstdio>    //std::remove
//#include <unistd.h>  //close

/*!\file
   Describes mapped_file class
*/

namespace boost {
namespace interprocess {

/*!A class that wraps basic file-mapping management*/
class mapped_file : private boost::noncopyable
{
 public:

   class mapped_file_info_t
   {
      friend class shared_memory;
      public:
      mapped_file_info_t(void* addr, std::size_t size)
         :  m_addr(addr), m_size(size)
      {}
      /*!Returns pointer to the shared memory fragment
         the user can overwrite*/
      void *      get_address() const;
      /*!Returns the size of the shared memory fragment
         the user can overwrite*/
      std::size_t get_size()const;
      private:
      void*          m_addr;
      std::size_t    m_size;
   };

   /*!No-op functor*/
   struct null_func_t
   {
      bool operator()(const mapped_file_info_t *, bool) const
         {   return true;   }
   };

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
//                     file_mapping::accessmode_t mode,
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
//                     file_mapping::accessmode_t mode,
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
//                     file_mapping::accessmode_t mode,
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
//                     file_mapping::accessmode_t mode,
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
//                     file_mapping::accessmode_t mode,
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
//                     file_mapping::accessmode_t mode,
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

   /*!Flushes to the disk a byte range within the mapped file. 
      Never throws*/
   bool     flush();

   /*!Returns the name of the shared memory segment used in the
      constructor. Never throws.*/
   const char *get_name() const;

   private:
   enum type_t {   DoCreate, DoOpen, DoCreateOrOpen  };

   template <class ConstructFunc>
   bool
        priv_open_or_create(type_t type,
                            const char *name,
                            std::size_t size,
//                            file_mapping::accessmode_t mode, 
                            const void *addr,
                            ConstructFunc construct_func);

   #if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   typedef void * OS_handle_t;
   #else    //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   typedef int    OS_handle_t;
   #endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

   //Members
   std::auto_ptr<mapped_region> mp_mapped_region;
   std::string m_name;
};

inline void *mapped_file::mapped_file_info_t::get_address() const
{  return this->m_addr; }

inline std::size_t mapped_file::mapped_file_info_t::get_size() const
{  return this->m_size; }

inline
mapped_file::mapped_file
                  (detail::create_only_t, 
                  const char *name,
                  std::size_t size,
//                  file_mapping::accessmode_t mode,
                  const void *addr)
{
   this->priv_open_or_create(DoCreate, name, size, /*mode, */addr, null_func_t());
}

inline
mapped_file::mapped_file
                  (detail::open_only_t, 
                  const char *name,
//                  file_mapping::accessmode_t mode,
                  const void *addr)
{
   this->priv_open_or_create(DoOpen, name, 0, /*mode, */addr, null_func_t());
}

inline
mapped_file::mapped_file
                  (detail::open_or_create_t, 
                  const char *name,
                  std::size_t size,
//                  file_mapping::accessmode_t mode,
                  const void *addr)
{
   this->priv_open_or_create(DoCreateOrOpen, name, size, /*mode, */addr, null_func_t());
}

template <class ConstructFunc> inline
mapped_file::mapped_file(detail::create_only_t, 
                  const char *name,
                  std::size_t size,
//                  file_mapping::accessmode_t mode,
                  const void *addr,
                  const ConstructFunc &construct_func)
{
   this->priv_open_or_create(DoCreate, name, size, /*mode, */addr, construct_func);
}

template <class ConstructFunc> inline
mapped_file::mapped_file(detail::open_only_t, 
                  const char *name,
//                  file_mapping::accessmode_t mode,
                  const void *addr,
                  const ConstructFunc &construct_func)
{
   this->priv_open_or_create(DoOpen, name, 0, /*mode, */addr, construct_func);
}


template <class ConstructFunc> inline
mapped_file::mapped_file(detail::open_or_create_t, 
                  const char *name,
                  std::size_t size,
//                  file_mapping::accessmode_t mode,
                  const void *addr,
                  const ConstructFunc &construct_func)
{
   this->priv_open_or_create(DoCreateOrOpen, name, size, /*mode, */addr, construct_func);
}


inline
mapped_file::~mapped_file()
{
}

std::size_t mapped_file::get_size() const
{  return mp_mapped_region->get_size();  }

void*    mapped_file::get_address() const
{  return mp_mapped_region->get_address();  }

bool     mapped_file::flush()
{  return mp_mapped_region->flush();  }

inline const char *mapped_file::get_name()  const  
{  return m_name.c_str();  }

template <class ConstructFunc> inline
bool
      mapped_file::priv_open_or_create(mapped_file::type_t type,
                           const char *name,
                           std::size_t size,
//                            file_mapping::accessmode_t mode, 
                           const void *addr,
                           ConstructFunc construct_func)
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

   boost::shared_ptr<file_mapping> p_mapped_file;

   if(type == DoOpen){
      //Open existing shared memory
      created = false;
   }
   else if(type == DoCreateOrOpen || type == DoCreate){
      OS_handle_t hnd = detail::create_new_file(name);
      if(hnd != detail::invalid_file()){
         if(!detail::close_file(hnd)){
            err = system_error_code();
            #ifdef BOOST_NO_EXCEPTIONS
            return false;      
            #else
            throw interprocess_exception(err);
            #endif
         }
         std::fstream file;
         file.open(name, std::ios::binary | std::ios::out | std::ios::trunc);
         file.seekp(static_cast<std::fstream::off_type>(size-1), std::ios::beg);
         file.write("", 1);
         if(!file.good()){
            file.close();
            std::remove(name);
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
   p_mapped_file.reset(new file_mapping(name, file_mapping::rw_mode/*mode*/));
   mp_mapped_region.reset(new mapped_region
      (p_mapped_file, 0, size, file_mapping::rw_mode/*mode */,addr));

   //Execute atomic functor
   mapped_file_info_t info(mp_mapped_region->get_address(), size);
   construct_func(&info, created);

   return true;
}

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_FULLY_MAPPED_FILE_HPP
