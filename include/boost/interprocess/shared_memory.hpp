//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_MEMORY_HPP
#define BOOST_INTERPROCESS_MEMORY_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include <boost/type_traits/type_with_alignment.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/noncopyable.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/smart_ptr/scoped_ptr.hpp>
#include <boost/detail/no_exceptions_support.hpp>
#include <boost/function.hpp>
#include <boost/interprocess/detail/global_lock.hpp>
#include <boost/compatibility/cpp_c_headers/cstddef>
#include <boost/interprocess/memory_mapping.hpp>
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>

#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
#  include <boost/interprocess/sync/win32/win32_sync_primitives.hpp>
#else

#  ifdef BOOST_HAS_UNISTD_H
#    include <fcntl.h>        //O_CREAT, O_*... 
#    include <sys/mman.h>     //mmap
#    include <unistd.h>       //ftruncate, close
#    include <semaphore.h>  //sem_t* family, SEM_VALUE_MAX
#    include <string>         //std::string
#    include <sys/stat.h>     //mode_t, S_IRWXG, S_IRWXO, S_IRWXU,
#  else
#    error Unknown platform
#  endif

#endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)


/*!\file
   Describes shared_memory class
*/

namespace boost {

namespace interprocess {

/*!A class that wraps a shared memory mapping that can be used to
   create mapped regions from the mapped files*/
class shared_memory_mapping : public memory_mapping
{
   enum type_t {   DoCreate, DoOpen, DoCreateOrOpen  };
   public:

   /*!Opens a file mapping of file "filename", starting in offset 
      "file_offset", and the mapping's size will be "size". The mapping 
      can be opened for read-only "ro_mode" or read-write "rw_mode. modes.*/
//   shared_memory_mapping(const char *filename,   memory_mapping::accessmode_t mode);

   // Creates a shared memory object with name "name" and size "size", with the access mode "mode"
   // If the file previously exists, throws an error
   shared_memory_mapping(detail::create_only_t, const char *name, memory_mapping::accessmode_t mode)
   {  this->priv_open_or_create(DoCreate, name, mode);  }

   // Tries to create a shared memory object with name "name" and size "size", with the
   // access mode "mode". If the file previously exists, it tries to open it with mode "mode".
   // Otherwise throws an error.
   shared_memory_mapping(detail::open_or_create_t, const char *name, memory_mapping::accessmode_t mode)
   {  this->priv_open_or_create(DoCreateOrOpen, name, mode);  }

   // Tries to open a shared memory object with name "name", with the access mode "mode". 
   // If the file does not previously exist, it throws an error.
   shared_memory_mapping(detail::open_only_t, const char *name, memory_mapping::accessmode_t mode)
   {  this->priv_open_or_create(DoOpen, name, mode);  }

   // Erases a shared memory object from the system.
   static bool remove(const char *name);
   
   //Sets the size of the shared memory mapping
   void truncate(memory_mapping::mapping_offset_t length);

   // Closes the shared memory mapping. All mapped regions are still
   // valid after destruction. The shared memory object still exists and
   // can be newly opened.
   ~shared_memory_mapping();

   /*!Returns the name of the file.*/
   const char *get_name() const;

   /*!Return access mode*/
   accessmode_t get_mode() const;

   /*!Get mapping handle*/
   mapping_handle_t get_mapping_handle() const;

   private:
   /*!Closes a previously opened file mapping. Never throws.*/
   void priv_close();

   bool priv_open_or_create(type_t type, const char *filename, 
                            memory_mapping::accessmode_t mode);

   std::string    m_filename;
};

inline shared_memory_mapping::~shared_memory_mapping() 
{  this->priv_close(); }

/*!Returns the name of the file.*/
inline const char *shared_memory_mapping::get_name() const
{  return m_filename.c_str(); }

#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

inline bool shared_memory_mapping::priv_open_or_create
   (type_t type, 
    const char *filename,
    memory_mapping::accessmode_t mode)
{
   m_filename = filename;

   //Make sure a temporary path is created for shared memory
   const unsigned long BufferSize = 512;

   std::string shmfile;
   shmfile.resize(BufferSize);

   unsigned long n = winapi::get_temp_path(BufferSize, &shmfile[0]);
   if(n > BufferSize){
      shmfile.resize(n);
      n = winapi::get_temp_path(n, &shmfile[0]);      
   }

   if(!n){
      error_info err = winapi::get_last_error();
      throw interprocess_exception(err);
   }

   //Remove final null.
   shmfile.resize(n);
   shmfile += "boost_interprocess\\";

   if(!winapi::create_directory(shmfile.c_str(), 0)){
      unsigned long err = winapi::get_last_error();
      if(err != winapi::error_already_exists){
         error_info err = winapi::get_last_error();
         throw interprocess_exception(err);
      }
   }

   shmfile += filename;

   unsigned long file_access  = 0;
   unsigned long file_creation_flags  = 0;

   //Set accesses
   if (mode == rw_mode){
      file_access       |= winapi::generic_read | winapi::generic_write;
   }
   else if (mode == ro_mode){
      file_access       |= winapi::generic_read;
   }

   switch(type){
      case DoOpen:
         file_creation_flags = winapi::open_existing;
      break;
      case DoCreate:
         file_creation_flags = winapi::create_new;
      break;
      case DoCreateOrOpen:
         file_creation_flags = winapi::open_always;
      break;
      default:
         {
            error_info err = other_error;
            throw interprocess_exception(err);
         }
   }

   //Open file using windows API since we need the handle
   memory_mapping::mapping_handle_t hnd = winapi::create_file  
      (shmfile.c_str(), file_access, file_creation_flags, winapi::file_attribute_temporary);

   //Check for error
   if(hnd == winapi::invalid_handle_value){
      error_info err = winapi::get_last_error();
      this->priv_close();
      throw interprocess_exception(err);
   }

   memory_mapping::assign_data(hnd, mode);
   return true;
}

inline bool shared_memory_mapping::remove(const char *filename)
{
   try{
      //Make sure a temporary path is created for shared memory
      const unsigned long BufferSize = 512;

      std::string shmfile;
      shmfile.resize(BufferSize);

      unsigned long n = winapi::get_temp_path(BufferSize, &shmfile[0]);
      if(n > BufferSize){
         shmfile.resize(n);
         n = winapi::get_temp_path(n, &shmfile[0]);      
      }

      if(!n){
         error_info err = winapi::get_last_error();
         throw interprocess_exception(err);
      }

      //Remove final null.
      shmfile.resize(n);
      shmfile += "boost_interprocess\\";
      shmfile += filename;
      return std::remove(shmfile.c_str()) == 0;
   }
   catch(...){
      return false;
   }
}

inline void shared_memory_mapping::truncate(memory_mapping::mapping_offset_t length)
{
   if(!winapi::set_file_pointer_ex( memory_mapping::get_mapping_handle(), length
                               , 0, winapi::file_begin)){
      error_info err = winapi::get_last_error();
      throw interprocess_exception(err);
   }

   if(!winapi::set_end_of_file(memory_mapping::get_mapping_handle())){
      error_info err = winapi::get_last_error();
      throw interprocess_exception(err);
   }
}

inline void shared_memory_mapping::priv_close()
{
   if(memory_mapping::get_mapping_handle() != winapi::invalid_handle_value){
      winapi::close_handle(memory_mapping::get_mapping_handle());
      memory_mapping::assign_data
         (winapi::invalid_handle_value, memory_mapping::get_mode());
   }
}

#else    //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

inline bool shared_memory_mapping::priv_open_or_create
   (type_t type, 
    const char *filename,
    memory_mapping::accessmode_t mode)
{
   m_filename = filename;

   //Create new mapping

   int oflag = 0;
   if(mode == memory_mapping::ro_mode){
      oflag |= O_RDONLY;
   }
   else if(mode == memory_mapping::rw_mode){
      oflag |= O_RDWR;
   }
   else{
      error_info err(mode_error);
      throw interprocess_exception(err);
   }

   switch(type){
      case DoOpen:
         //No addition
      break;
      case DoCreate:
         oflag |= (O_CREAT | O_EXCL);
      break;
      case DoCreateOrOpen:
         oflag |= O_CREAT;
      break;
      default:
         {
            error_info err = other_error;
            throw interprocess_exception(err);
         }
   }

   //Open file using windows API since we need the handle
   memory_mapping::mapping_handle_t hnd = shm_open
      (filename, oflag, S_IRWXO | S_IRWXG | S_IRWXU);

   //Check for error
   if(hnd == -1){
      error_info err = errno;
      this->priv_close();
      throw interprocess_exception(err);
   }

   memory_mapping::assign_data(hnd, mode);
   return true;
}

inline bool shared_memory_mapping::remove(const char *filename)
{
   return 0 != shm_unlink(filename);
}

inline void shared_memory_mapping::truncate(memory_mapping::mapping_offset_t length)
{
   if(0 != ftruncate(memory_mapping::get_mapping_handle(), length)){
      error_info err(system_error_code());
      throw interprocess_exception(err);
   }
}

inline void shared_memory_mapping::priv_close()
{
   if(memory_mapping::get_mapping_handle() != -1){
      ::close(memory_mapping::get_mapping_handle());
      memory_mapping::assign_data
         (-1, memory_mapping::get_mode());
   }
}

#endif   //##if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)


/*!A class that wraps basic shared memory management*/
class shared_memory : private boost::noncopyable
{
   enum type_t {   DoCreate, DoOpen, DoCreateOrOpen  };
   shared_memory();

   public:

   /*!Creates a shared memory segment with name "name", with size "size".
      If the segment was previously created it throws an error.
      The segment can be created in two modes: read-only "ro_mode" or read-write "rw_mode".
      The user can also specify the mapping address in "addr". If "addr" is 0,
      the operating system will choose the mapping address.

      This function can throw boost::inteprocess_exception and
      other standard exceptions derived from std::exception
      (like std::bad_alloc)*/
   shared_memory(detail::create_only_t, 
                 const char *name,
                 std::size_t size,
                 memory_mapping::accessmode_t mode = memory_mapping::rw_mode,
                 const void *addr = 0);

   /*!Opens a shared memory segment with name "name". If it was previously 
      created throws an error.
      The segment can be opened in two modes: read-only "ro_mode" or read-write "rw_mode".
      The user can also specify the mapping address in "addr". If "addr" is 0,
      the operating system will choose the mapping address.

      This function can throw boost::inteprocess_exception and
      other standard exceptions derived from std::exception
      (like std::bad_alloc)*/
   shared_memory(detail::open_only_t, 
                 const char *name,
                 memory_mapping::accessmode_t mode = memory_mapping::rw_mode,
                 const void *addr = 0);

   /*!Creates a shared memory segment with name "name", and size "size" if
      the shared memory was not previously created. If it was previously 
      created it tries to open it. User can specify the mapping address in "addr".
      The segment can be opened in two modes: read-only "ro_mode" or read-write "rw_mode".
      If "addr" is 0, the operating system will choose the mapping address.

      This function can throw boost::inteprocess_exception and
      other standard exceptions derived from std::exception
      (like std::bad_alloc)*/
   shared_memory(detail::open_or_create_t, 
                 const char *name,
                 std::size_t size,
                 memory_mapping::accessmode_t mode = memory_mapping::rw_mode,
                 const void *addr = 0);

   /*!Creates a shared memory segment with name "name", with size "size".
      The user must also specify the mapping address in "addr". If "addr" is 0,
      the operating system will choose the mapping address.
      It also executes a copy of the functor "construct_func" atomically if
      the segment is created. The functor must have the following signature:

      bool operator()(const segment_info_t * info, bool created)
      
      "info" is an initialized segment info structure, and "created" 
      must be "true". If the functor returns "false", or throws an error 
      is supposed and the shared memory won't be created.
      The user must specify a destroy_func, that will be copied in this
      constructor and executed in the destructor, before unmapping the
      shared memory segment.
      The functor must have the following signature:

      void operator()(const segment_info_t * info, bool last) const
      
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
   shared_memory(detail::create_only_t, 
                 const char *name,
                 std::size_t size,
                 memory_mapping::accessmode_t mode,
                 const void *addr,
                 const ConstructFunc &construct_func);

   /*!Creates a shared memory segment with name "name", and size "size" if
      the shared memory was not previously created. If it was previously 
      created tries to open it. 
      User must specify the mapping address in "addr". If "addr" is 0,
      the operating system will choose the mapping address.
      It also executes a copy of the functor "construct_func" atomically
      if the segment is opened. The functor must have the following signature:

      bool operator()(const segment_info_t * info, bool created)
      
      "info" is an initialized segment info structure, and "created" 
      must be "false". If the functor returns "false", or throws an error 
      is supposed and the shared memory won't be opened.
      The user must specify a destroy_func, that will be copied in this
      constructor and executed in the destructor, before unmapping the
      shared memory segment.
      The functor must have the following signature:

      void operator()(const segment_info_t * info, bool last) const
      
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
   shared_memory(detail::open_only_t, 
                 const char *name,
                 memory_mapping::accessmode_t mode,
                 const void *addr,
                 const ConstructFunc &construct_funcc);

   /*!Creates a shared memory segment with name "name", and size "size" if
      the shared memory was not previously created. If it was previously 
      created it tries to open it. User can specify the mapping address in "addr".
      If "addr" is 0, the operating system will choose the mapping address.
      It also executes the functor "construct_func" atomically if the segment is 
      created or opened. The functor must have the following signature:

      bool operator()(const segment_info_t * info, bool created)
      
      "info" is an initialized segment info structure, and "created" 
      will be "true" if the shared memory was created. If the functor 
      returns "false", or throws an error is supposed and the
      shared memory won't be opened.
      The user must specify a destroy_func, that will be copied in this
      constructor and executed in the destructor, before unmapping the
      shared memory segment.
      The functor must have the following signature:

      void operator()(const segment_info_t * info, bool last) const
      
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
   shared_memory(detail::open_or_create_t, 
                 const char *name,
                 std::size_t size,
                 memory_mapping::accessmode_t mode,
                 const void *addr,
                 const ConstructFunc &construct_func);

   /*!Destructor. Unmaps shared memory from process' address space.
      It also executes the destruction functor atomically if
      the user has registered that the destruction functor
      in the constructor of this class.
      Never throws.*/
   ~shared_memory();

   /*!Returns the size of the shared memory segment. Never throws.*/
   std::size_t get_size()  const;

   /*!Returns shared memory segment's address for this process. 
      Never throws.*/
   void*    get_address()  const;

   /*!Swaps the shared memory segment this shared_memory object 
      manages with another managed by "other".
      Never throws.*/
   void swap(shared_memory &other);

   /*!Returns the name of the shared memory segment used in the
      constructor. Never throws.*/
   const char *get_name() const;

   /*!Erases a shared memory object from the system.*/
   static bool remove(const char *name);

   #if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   typedef void * OS_handle_t;
   #else    //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   typedef int    OS_handle_t;
   #endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

   /*!Returns the handle that identifies the shared memory
      segment in a operating system. This is operating system
      and implementation dependent and should be used only
      in special situations.
      Never throws.*/
   OS_handle_t get_OS_handle() const;

   private:

   template <class ConstructFunc>
   bool priv_open_or_create(type_t type, const char *name, std::size_t size,
                            memory_mapping::accessmode_t mode, const void *addr, 
                            ConstructFunc construct_func);
   mapped_region     m_mapped_region;
   std::string       m_name;
};

inline shared_memory::shared_memory
   (detail::create_only_t, const char *name, std::size_t size, 
   memory_mapping::accessmode_t mode, const void *addr)
{
   this->priv_open_or_create(DoCreate, name, size, mode, addr, null_mapped_region_function());
}

inline shared_memory::shared_memory
   (detail::open_only_t, const char *name, 
   memory_mapping::accessmode_t mode, const void *addr)
{
   this->priv_open_or_create(DoOpen, name, 0, mode, addr, null_mapped_region_function());
}

inline shared_memory::shared_memory
   (detail::open_or_create_t, const char *name, std::size_t size, 
   memory_mapping::accessmode_t mode, const void *addr)
{
   this->priv_open_or_create(DoCreateOrOpen, name, size, mode, addr, null_mapped_region_function());
}

template <class ConstructFunc>
inline shared_memory::shared_memory
   (detail::create_only_t, const char *name, std::size_t size, 
   memory_mapping::accessmode_t mode, const void *addr, 
   const ConstructFunc &construct_func)
{
   this->priv_open_or_create(DoCreate, name, size, mode, addr, construct_func);
}

template <class ConstructFunc>
inline shared_memory::shared_memory
   (detail::open_only_t, const char *name,
   memory_mapping::accessmode_t mode, const void *addr, 
   const ConstructFunc &construct_func)
{
   this->priv_open_or_create(DoOpen, name, 0, mode, addr, construct_func);
}

template <class ConstructFunc>
inline shared_memory::shared_memory
   (detail::open_or_create_t, const char *name, std::size_t size, 
   memory_mapping::accessmode_t mode, const void *addr, 
   const ConstructFunc &construct_func)
{
   this->priv_open_or_create(DoCreateOrOpen, name, size, mode, addr, construct_func);
}

   /*!Erases a shared memory object from the system.*/
inline bool shared_memory::remove(const char *name)
{  return shared_memory_mapping::remove(name);  }

inline shared_memory::~shared_memory() 
{}

inline std::size_t   shared_memory::get_size()  const  
{  return m_mapped_region.get_size(); }

inline void*    shared_memory::get_address()  const  
{  return m_mapped_region.get_address();  }

inline const char *shared_memory::get_name()  const  
{  return m_name.c_str();  }

inline void shared_memory::swap(shared_memory &other)
{
   detail::do_swap(this->m_name,    other.m_name);
   detail::do_swap(this->m_mapped_region, other.m_mapped_region);
}

inline void swap(shared_memory &x, shared_memory &y)
{  x.swap(y);  }

template <class ConstructFunc> inline 
bool shared_memory::priv_open_or_create
   (shared_memory::type_t type,  const char *name, std::size_t size, 
   memory_mapping::accessmode_t mode, const void *addr,
   ConstructFunc construct_func)
{
   error_info err;

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

   //shared_memory_mapping has no default constructor.
   //And embedded system paranoids like me don't like dynamic memoryç
   //boost::optional does not solve well in place factories
   boost::optional<shared_memory_mapping> mapping;
   
   //Do some in-place construction
   bool created = false;
   switch(type){
      case DoOpen:
      {
         mapping = in_place(open_only, name, memory_mapping::rw_mode);
      }
      break;
      case DoCreateOrOpen:
      {
         try{
            mapping = in_place(create_only, name, memory_mapping::rw_mode);
            mapping->truncate(size);
            created = true;
         }
         catch(interprocess_exception &ex){
            if(ex.get_error_code() != already_exists_error){
               throw;
            }
            mapping = in_place(open_only, name, memory_mapping::rw_mode);            
         }
      }
      break;
      case DoCreate:
      {
         mapping = in_place(create_only, name, memory_mapping::rw_mode);
         mapping->truncate(size);
         created = true;
      }
      break;
      default:
      break;
   }
   
   mapped_region mapped(*mapping, 0, size, /*file_mapping::rw_mode*/mode, addr);

   //Now swap mapped region
   m_mapped_region.swap(mapped);

   //Execute atomic functor
   construct_func(m_mapped_region, created);

   return true;
}

}  //namespace interprocess {

}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_MEMORY_HPP
