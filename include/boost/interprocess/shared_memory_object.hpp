//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_SHARED_MEMORY_OBJECT_HPP
#define BOOST_INTERPROCESS_SHARED_MEMORY_OBJECT_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/os_file_functions.hpp>
#include <cstddef>


#include <string>

#ifdef BOOST_INTERPROCESS_POSIX_SHARED_MEMORY_OBJECTS
#  include <fcntl.h>        //O_CREAT, O_*... 
#  include <sys/mman.h>     //shm_xxx
#  include <unistd.h>       //ftruncate, close
#  include <sys/stat.h>     //mode_t, S_IRWXG, S_IRWXO, S_IRWXU,
#else
#  include<boost/interprocess/detail/os_file_functions.hpp>
#endif

/*!\file
   Describes a shared memory object management class.
*/

namespace boost {
namespace interprocess {

/*!A class that wraps a shared memory mapping that can be used to
   create mapped regions from the mapped files*/
class shared_memory_object
{
   /// @cond
   //Non-copyable and non-assignable
   shared_memory_object(const shared_memory_object &);
   shared_memory_object &operator=(const shared_memory_object &);
   /// @endcond

   public:

   //!Default constructor. Represents an empty shared_memory_object.
   shared_memory_object();

   //!Creates a shared memory object with name "name" and mode "mode", with the access mode "mode"
   //!If the file previously exists, throws an error.*/
   shared_memory_object(detail::create_only_t, const char *name, mode_t mode)
   {  this->priv_open_or_create(DoCreate, name, mode);  }

   //!Tries to create a shared memory object with name "name" and mode "mode", with the
   //!access mode "mode". If the file previously exists, it tries to open it with mode "mode".
   //!Otherwise throws an error.
   shared_memory_object(detail::open_or_create_t, const char *name, mode_t mode)
   {  this->priv_open_or_create(DoCreateOrOpen, name, mode);  }

   //!Tries to open a shared memory object with name "name", with the access mode "mode". 
   //!If the file does not previously exist, it throws an error.
   shared_memory_object(detail::open_only_t, const char *name, mode_t mode)
   {  this->priv_open_or_create(DoOpen, name, mode);  }

   //!Moves the ownership of "moved"'s shared memory object to *this. 
   //!After the call, "moved" does not represent any shared memory object. 
   //!Does not throw
   shared_memory_object
      (detail::moved_object<shared_memory_object> &moved)
   {  this->swap(moved.get());   }

   //!Moves the ownership of "moved"'s shared memory to *this.
   //!After the call, "moved" does not represent any shared memory. 
   //!Does not throw
   shared_memory_object &operator=
      (detail::moved_object<shared_memory_object> &moved)
   {  
      shared_memory_object tmp(moved);
      this->swap(tmp);
      return *this;  
   }

   //!Swaps the shared_memory_objects. Does not throw
   void swap(shared_memory_object &other);

   //!Erases a shared memory object from the system. Never throws
   static bool remove(const char *name);
   
   //!Sets the size of the shared memory mapping
   void truncate(offset_t length);

   //!Closes the shared memory mapping. All mapped regions are still
   //!valid after destruction. The shared memory object still exists and
   //!can be newly opened.
   ~shared_memory_object();

   //!Returns the name of the file.
   const char *get_name() const;

   //!Returns access mode
   mode_t get_mode() const;

   //!Returns mapping handle. Never throws.
   mapping_handle_t get_mapping_handle() const;

   /// @cond
   private:
   //!Closes a previously opened file mapping. Never throws.
   void priv_close();

   //!Closes a previously opened file mapping. Never throws.
   bool priv_open_or_create(create_enum_t type, const char *filename, mode_t mode);

   file_handle_t  m_handle;
   mode_t      m_mode;
   std::string       m_filename;
   /// @endcond
};

inline shared_memory_object::shared_memory_object() 
   :  m_handle(file_handle_t(detail::invalid_file()))
{}

inline shared_memory_object::~shared_memory_object() 
{  this->priv_close(); }


inline const char *shared_memory_object::get_name() const
{  return m_filename.c_str(); }

inline void shared_memory_object::swap(shared_memory_object &other)
{  
   std::swap(m_handle,  other.m_handle);
   std::swap(m_mode,    other.m_mode);
   m_filename.swap(other.m_filename);   
}

inline mapping_handle_t shared_memory_object::get_mapping_handle() const
{  return detail::mapping_handle_from_file_handle(m_handle);  }

inline mode_t shared_memory_object::get_mode() const
{  return m_mode; }

#if !defined(BOOST_INTERPROCESS_POSIX_SHARED_MEMORY_OBJECTS)

inline bool shared_memory_object::priv_open_or_create
   (create_enum_t type, const char *filename, mode_t mode)
{
   m_filename = filename;

   const char *tmp_path = detail::get_temporary_path(); 
   if(!tmp_path){
      error_info err = system_error_code();
      throw interprocess_exception(err);
   }

   std::string shmfile = tmp_path;

   shmfile += "/boost_interprocess";

   //Create the temporary directory.
   //If fails, check that it's because already exists
   if(!detail::create_directory(shmfile.c_str())){
      error_info info(system_error_code());
      if(info.get_error_code() != already_exists_error){
         throw interprocess_exception(info);
      }
   }

   //Add filename
   shmfile += '/';
   shmfile += filename;

   //Set accesses
   if (mode != read_write && mode != read_only){
      error_info err = other_error;
      throw interprocess_exception(err);
   }

   switch(type){
      case DoOpen:
         m_handle = detail::open_existing_file(shmfile.c_str(), mode, true);
      break;
      case DoCreate:
         m_handle = detail::create_new_file(shmfile.c_str(), mode, true);
      break;
      case DoCreateOrOpen:
         m_handle = detail::create_or_open_file(shmfile.c_str(), mode, true);
      break;
      default:
         {
            error_info err = other_error;
            throw interprocess_exception(err);
         }
   }

   //Check for error
   if(m_handle == detail::invalid_file()){
      error_info err = system_error_code();
      this->priv_close();
      throw interprocess_exception(err);
   }

   m_mode = mode;
   return true;
}

inline bool shared_memory_object::remove(const char *filename)
{
   try{
      //Make sure a temporary path is created for shared memory
      std::string shmfile;

      const char *tmp_dir = detail::get_temporary_path();
      if(!tmp_dir){
         error_info err = system_error_code();
         throw interprocess_exception(err);
      }
      shmfile = tmp_dir;

      //Remove final null.
      shmfile += "/boost_interprocess/";
      shmfile += filename;
      return std::remove(shmfile.c_str()) == 0;
   }
   catch(...){
      return false;
   }
}

inline void shared_memory_object::truncate(offset_t length)
{
   if(!detail::truncate_file(m_handle, length)){
      error_info err = system_error_code();
      throw interprocess_exception(err);
   }
}

inline void shared_memory_object::priv_close()
{
   if(m_handle != detail::invalid_file()){
      detail::close_file(m_handle);
      m_handle = detail::invalid_file();
   }
}

#else //!defined(BOOST_INTERPROCESS_POSIX_SHARED_MEMORY_OBJECTS)

inline bool shared_memory_object::priv_open_or_create
   (create_enum_t type, 
    const char *filename,
    mode_t mode)
{
   m_filename = filename;

   //Create new mapping

   int oflag = 0;
   if(mode == read_only){
      oflag |= O_RDONLY;
   }
   else if(mode == read_write){
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

   //Open file using POSIX API
   m_handle = shm_open(filename, oflag, S_IRWXO | S_IRWXG | S_IRWXU);

   //Check for error
   if(m_handle == -1){
      error_info err = errno;
      this->priv_close();
      throw interprocess_exception(err);
   }

   m_mode = mode;
   return true;
}

inline bool shared_memory_object::remove(const char *filename)
{
   return 0 != shm_unlink(filename);
}

inline void shared_memory_object::truncate(offset_t length)
{
   if(0 != ftruncate(m_handle, length)){
      error_info err(system_error_code());
      throw interprocess_exception(err);
   }
}

inline void shared_memory_object::priv_close()
{
   if(m_handle != -1){
      ::close(m_handle);
      m_handle = -1;
   }
}

#endif

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_SHARED_MEMORY_OBJECT_HPP
