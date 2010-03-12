//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2009-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_INTERMODULE_SINGLETON_HPP
#define BOOST_INTERPROCESS_INTERMODULE_SINGLETON_HPP

#if defined(_MSC_VER)&&(_MSC_VER>=1200)
#pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/interprocess/detail/os_thread_functions.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/detail/tmp_dir_helpers.hpp>
#include <boost/interprocess/detail/os_file_functions.hpp>
#include <boost/interprocess/streams/bufferstream.hpp>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstring>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#if (defined BOOST_INTERPROCESS_WINDOWS)
#include <fcntl.h>
#include <io.h>

#include <sys/locking.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace boost{
namespace interprocess{
namespace detail{

namespace intermodule_singleton_helpers {

const int GMemMarkToBeRemoved = -1;
const int GMemNotPresent      = -2;

typedef char pid_str_t[sizeof(OS_process_id_t)*3+1];

inline void get_pid_str(pid_str_t &pid_str)
{
   OS_process_id_t pid = get_current_process_id();
   bufferstream bstream(pid_str, sizeof(pid_str));
   bstream << pid << std::ends;
}

inline const char *get_lock_file_base_name()
{  return "bip.gmem.lock.";  }

inline const char *get_lock_file_subdir_name()
{  return "bip.gmem.locks";  }

inline void create_and_get_lock_file_path(std::string &s)
{
   std::string filename(get_lock_file_base_name());
   pid_str_t pid_str;
   get_pid_str(pid_str);
   filename += pid_str;
   tmp_folder(s);
   if(!open_or_create_directory(s.c_str())){
      throw interprocess_exception(error_info(system_error_code()));
   }
   s += "/";
   s += get_lock_file_subdir_name();
   if(!open_or_create_directory(s.c_str())){
      throw interprocess_exception(error_info(system_error_code()));
   }
   s += "/";
   s += filename;
}

inline void get_lock_file_path(std::string &s)
{
   std::string filename(get_lock_file_base_name());
   pid_str_t pid_str;
   get_pid_str(pid_str);
   filename += pid_str;

   tmp_folder(s);
   s += "/";
   s += get_lock_file_subdir_name();
   s += "/";
   s += filename;
}

inline const char *get_shm_base_name()
{  return "bip.gmem.shm.";  }

inline void get_shm_name(std::string &shm_name)
{
   shm_name = get_shm_base_name();
   pid_str_t pid_str;
   get_pid_str(pid_str);
   shm_name += pid_str;
}

inline std::size_t get_shm_size()
{  return 65536;  }

inline bool is_other_process_gmem_lock_file(const char *filename, std::string &pid_string)
{
   const char * const base_str = get_lock_file_base_name();
   const std::size_t base_str_len = std::strlen(base_str);
   const std::size_t filename_length = std::strlen(filename);

   //Get pid string
   pid_str_t pid_str;
   get_pid_str(pid_str);

   //Check filename length
   const std::size_t pid_str_len = std::strlen(pid_str);
   if(base_str_len >= filename_length){
      return false;
   }
   //Check base str
   for(std::size_t i = 0, max = base_str_len; i != max; ++i){
      if(base_str[i] != filename[i]){
         return false;
      }
   }

   pid_string = &filename[base_str_len];
   if((base_str_len + pid_str_len) != filename_length){
      return true;
   }

   //Check pid
   for(std::size_t i = base_str_len, max = filename_length, pid_i = 0; i != max; ++i, ++pid_i){
      if(filename[i] != pid_str[pid_i]){
         return true;
      }
   }
   return false;
}

inline void apply_gmem_erase_logic(const char *filepath, const char *filename);

#if (defined BOOST_INTERPROCESS_WINDOWS)

inline bool remove_old_gmem()
{
   void *             hFile;                       // Handle to directory
   winapi::win32_find_data_t  FileInformation;     // File information

   //Get base directory
   std::string str;
   tmp_folder(str);
   str += "/";
   str += get_lock_file_subdir_name();
   const std::size_t base_root_dir_len = str.size();

   //Find all files and directories
   str  +=  "\\*.*";
   hFile = winapi::find_first_file(str.c_str(), &FileInformation);
   if(hFile != winapi::invalid_handle_value){
      do{   //Now loop every file
         str.erase(base_root_dir_len);
         //If it's not "." or ".." skip it
         if(FileInformation.cFileName[0] != '.'){
            str += "\\";   str += FileInformation.cFileName;
            //If it's a file, apply erase logic
            if(!(FileInformation.dwFileAttributes & winapi::file_attribute_directory)){
               apply_gmem_erase_logic(str.c_str(), FileInformation.cFileName);
            }
         }
      //Go to the next file
      } while(winapi::find_next_file(hFile, &FileInformation) == 1);

      // Close handle and see if the loop has ended with an error
      winapi::find_close(hFile);
      if(winapi::get_last_error() != winapi::error_no_more_files){
         return false;
      }
   }
   return true;
}


struct locking_file_serial_id
{
   int fd;
   unsigned long dwVolumeSerialNumber;
   unsigned long nFileIndexHigh;
   unsigned long nFileIndexLow;
   volatile boost::uint32_t ref_count;
};

inline bool lock_locking_file(int fd)
{
   int ret = 0;
   while(ret != 0 && errno == EDEADLK){
      ret = _locking(fd, _LK_LOCK, 1/*lock_file_contents_length()*/);
   }
   return 0 == ret;
}

inline bool try_lock_locking_file(int fd)
{
   return 0 == _locking(fd, _LK_NBLCK , 1);
}

inline int open_or_create_and_lock_file(const char *name)
{
   while(1){
      file_handle_t handle = create_or_open_file(name);
      int fd = _open_osfhandle((intptr_t)handle, _O_TEXT);
      if(fd < 0){
         close_file(handle);
         return fd;
      }
      if(!try_lock_locking_file(fd)){
         _close(fd);
         return -1;
      }
      struct _stat s;
      if(0 == _stat(name, &s)){
         return fd;
      }
      else{
         _close(fd);
      }
   }
}

inline int try_open_and_lock_file(const char *name)
{
   file_handle_t handle = open_existing_file(name);
   int fd = _open_osfhandle((intptr_t)handle, _O_TEXT);
   if(fd < 0){
      close_file(handle);
      return fd;
   }
   if(!try_lock_locking_file(fd)){
      _close(fd);
      return -1;
   }
   return fd;
}

inline void close_lock_file(int fd)
{  _close(fd); }

inline bool is_valid_fd(int fd)
{
   struct _stat s;
   return EBADF != _fstat(fd, &s);
}

inline bool is_normal_file(int fd)
{
   if(_isatty(fd))
      return false;
   struct _stat s;
   if(0 != _fstat(fd, &s))
      return false;
   return 0 != (s.st_mode & _S_IFREG);
}

inline std::size_t get_size(int fd)
{
   struct _stat s;
   if(0 != _fstat(fd, &s))
      return 0u;
   return (std::size_t)s.st_size;
}

inline bool fill_file_serial_id(int fd, locking_file_serial_id &id)
{
   winapi::interprocess_by_handle_file_information info;
   if(!winapi::get_file_information_by_handle((void*)_get_osfhandle(fd), &info))
      return false;
   id.fd = fd;
   id.dwVolumeSerialNumber = info.dwVolumeSerialNumber;
   id.nFileIndexHigh = info.nFileIndexHigh;
   id.nFileIndexLow = info.nFileIndexLow;
   id.ref_count = 1; //Initialize attached count
   return true;
}

inline bool compare_file_serial(int fd, const locking_file_serial_id &id)
{
   winapi::interprocess_by_handle_file_information info;
   if(!winapi::get_file_information_by_handle((void*)_get_osfhandle(fd), &info))
      return false;

   return   id.dwVolumeSerialNumber == info.dwVolumeSerialNumber  &&
            id.nFileIndexHigh       == info.nFileIndexHigh        &&
            id.nFileIndexLow        == info.nFileIndexLow;
}

#else //UNIX

inline bool remove_old_gmem()
{
   std::string refcstrRootDirectory;
   tmp_folder(refcstrRootDirectory);
   refcstrRootDirectory += "/";
   refcstrRootDirectory += get_lock_file_subdir_name();

   DIR *d = opendir(refcstrRootDirectory.c_str());
   if(!d) {
      return false;
   }

   struct dir_close
   {
      DIR *d_;
      dir_close(DIR *d) : d_(d) {}
      ~dir_close() { ::closedir(d_); }
   } dc(d); (void)dc;

   struct ::dirent *de;
   struct ::stat st;
   std::string fn;

   while((de=::readdir(d))) {
      if( de->d_name[0] == '.' && ( de->d_name[1] == '\0'
            || (de->d_name[1] == '.' && de->d_name[2] == '\0' )) ){
         continue;
      }
      fn = refcstrRootDirectory;
      fn += '/';
      fn += de->d_name;

      if(std::remove(fn.c_str())) {
         if(::stat(fn.c_str(), & st)) {
            return false;
         }
         //If it's a file, apply erase logic
         if(!S_ISDIR(st.st_mode)) {
            apply_gmem_erase_logic(fn.c_str(), de->d_name);
         }
      }
   }
   return true;
}

struct locking_file_serial_id
{
   int fd;
   dev_t st_dev;
   ino_t st_ino;
   volatile boost::uint32_t ref_count;
};

inline bool lock_locking_file(int fd)
{
   int ret = 0;
   while(ret != 0 && errno != EINTR){
      struct flock lock;
      lock.l_type = F_WRLCK;
      lock.l_whence = SEEK_SET;
      lock.l_start = 0;
      lock.l_len = 1;
      ret = fcntl (fd, F_SETLKW, &lock);
   }
   return 0 == ret;
}

inline bool try_lock_locking_file(int fd)
{
   struct flock lock;
   lock.l_type = F_WRLCK;
   lock.l_whence = SEEK_SET;
   lock.l_start = 0;
   lock.l_len = 1;
   return 0 == fcntl (fd, F_SETLK, &lock);
}

inline int open_or_create_and_lock_file(const char *name)
{
   while(1){
      int fd = create_or_open_file(name);
      if(fd < 0){
         return fd;
      }
      if(!try_lock_locking_file(fd)){
         close(fd);
         return -1;
      }
      struct stat s;
      if(0 == stat(name, &s)){
         return fd;
      }
      else{
         close(fd);
      }
   }
}

inline int try_open_and_lock_file(const char *name)
{
   int fd = open_existing_file(name);
   if(fd < 0){
      return fd;
   }
   if(!try_lock_locking_file(fd)){
      close(fd);
      return -1;
   }
   return fd;
}

inline void close_lock_file(int fd)
{  close(fd); }

inline bool is_valid_fd(int fd)
{
   struct stat s;
   return EBADF != fstat(fd, &s);
}

inline bool is_normal_file(int fd)
{
   struct stat s;
   if(0 != fstat(fd, &s))
      return false;
   return 0 != (s.st_mode & S_IFREG);
}

inline std::size_t get_size(int fd)
{
   struct stat s;
   if(0 != fstat(fd, &s))
      return 0u;
   return (std::size_t)s.st_size;
}

inline bool fill_file_serial_id(int fd, locking_file_serial_id &id)
{
   struct stat s;
   if(0 != fstat(fd, &s))
      return false;
   id.fd = fd;
   id.st_dev = s.st_dev;
   id.st_ino = s.st_ino;
   id.ref_count = 1; //Initialize attached count
   return true;
}

inline bool compare_file_serial(int fd, const locking_file_serial_id &id)
{
   struct stat info;
   if(0 != fstat(fd, &info))
      return false;

   return   id.st_dev == info.st_dev  &&
            id.st_ino == info.st_ino;
}

#endif

struct gmem_erase_func
{
   gmem_erase_func(const char *shm_name, const char *lock_file_path, managed_shared_memory & shm)
      :shm_name_(shm_name), lock_file_path_(lock_file_path), shm_(shm)
   {}

   void operator()()
   {
      locking_file_serial_id *pserial_id = shm_.find<locking_file_serial_id>("lock_file_fd").first;
      if(pserial_id){
         pserial_id->fd = GMemMarkToBeRemoved;
      }
      delete_file(lock_file_path_);
      shared_memory_object::remove(shm_name_);
   }
   
   const char * const shm_name_;
   const char * const lock_file_path_;
   managed_shared_memory & shm_;
};

inline void apply_gmem_erase_logic(const char *filepath, const char *filename)
{
   int fd = GMemMarkToBeRemoved;
   try{
      std::string str;
      if(!is_other_process_gmem_lock_file(filename, str)){
         return;
      }
      fd = try_open_and_lock_file(filepath);
      if(fd < 0){
         return;
      }
      str.insert(0, get_shm_base_name());
      try{
         managed_shared_memory shm(open_only, str.c_str());
         gmem_erase_func func(str.c_str(), filepath, shm);
         shm.try_atomic_func(func);
      }
      catch(interprocess_exception &e){
         if(e.get_error_code() == not_found_error){
            delete_file(filepath);
         }
      }
   }
   catch(...){

   }
   if(fd >= 0){
      close_lock_file(fd);
   }
}

}  //namespace intermodule_singleton_helpers {



namespace intermodule_singleton_helpers {

struct lock_file_logic
{
   lock_file_logic(managed_shared_memory &shm)
      : mshm(shm)
   {}

   locking_file_serial_id * register_lock_file(int fd)
   {
      locking_file_serial_id *pinfo = mshm.construct<locking_file_serial_id>("lock_file_fd")();
      fill_file_serial_id(fd, *pinfo);
      return pinfo;
   }

   void operator()(void)
   {
      retry_with_new_shm = false;
      locking_file_serial_id *pserial_id = mshm.find<locking_file_serial_id>("lock_file_fd").first;

      int fd;
      if(!pserial_id){
         fd = GMemNotPresent;
      }
      else{
         fd = pserial_id->fd;
      }
      if(fd == GMemNotPresent){
         std::string lck_str;
         create_and_get_lock_file_path(lck_str);
         int fd = intermodule_singleton_helpers::open_or_create_and_lock_file(lck_str.c_str());
         if(fd < 0){
            this->register_lock_file(GMemMarkToBeRemoved);
            std::string s;
            get_shm_name(s);
            shared_memory_object::remove(s.c_str());
            retry_with_new_shm = true;
         }
         else{
            this->register_lock_file(fd);
         }
      }
      else if (fd == GMemMarkToBeRemoved){
         retry_with_new_shm = true;
      }
      else if(!is_valid_fd(fd) ||
            !is_normal_file(fd) ||
            0 != get_size(fd) ||
            !compare_file_serial(fd, *pserial_id)){
         pserial_id->fd = GMemMarkToBeRemoved;
         std::string s;
         get_shm_name(s);
         shared_memory_object::remove(s.c_str());
         retry_with_new_shm = true;
      }
      else{
         //Valid lock file, just increment ref count
         atomic_inc32(&pserial_id->ref_count);
      }
   }

   managed_shared_memory &mshm;
   bool retry_with_new_shm;
};

}  //namespace intermodule_singleton_helpers {

//This class contains common code for all singleton types, so that we instantiate this
//code just once per module. This class also holds a reference counted shared memory
//to be used by all instances

template<int Dummy>
class intermodule_singleton_common
{
   public:
   typedef void*(init_func_t)(managed_shared_memory &);
   typedef void (fini_func_t)(void *, managed_shared_memory &);

   static void destroy_pc(void *ptr, fini_func_t fini_func)
   {
      if(ptr)
         fini_func(ptr, get_shm());
      if(1 == atomic_dec32(&shm_ref_count)){
         destroy_shm();
      }
   }

   static void initialize_pc(void *&ptr, volatile boost::uint32_t &barrier, init_func_t ini_func);

   private:
   static managed_shared_memory &get_shm()
   {
      return *static_cast<managed_shared_memory *>(static_cast<void *>(&shm_mem));
   }

   static const std::size_t MemSize = ((sizeof(managed_shared_memory)-1)/sizeof(max_align))+1u;
   static void initialize_shm();
   static void destroy_shm();
   //Static data, zero-initalized without any dependencies
   static volatile boost::uint32_t shm_ref_count;
   static volatile boost::uint32_t shm_initialized;
   static max_align shm_mem[MemSize];
};

template<int Dummy>
volatile boost::uint32_t intermodule_singleton_common<Dummy>::shm_ref_count;

template<int Dummy>
const std::size_t intermodule_singleton_common<Dummy>::MemSize;

template<int Dummy>
volatile boost::uint32_t intermodule_singleton_common<Dummy>::shm_initialized;

template<int Dummy>
max_align intermodule_singleton_common<Dummy>::shm_mem[intermodule_singleton_common<Dummy>::MemSize];

template<int Dummy>
void intermodule_singleton_common<Dummy>::initialize_shm()
{
   std::string s;
   intermodule_singleton_helpers::get_shm_name(s);
   const char *ShmName = s.c_str();
   const std::size_t ShmSize = intermodule_singleton_helpers::get_shm_size();;
   while(1){
      ::boost::uint32_t tmp = atomic_cas32(&shm_initialized, 1, 0);
      if(tmp >= 2u){
         break;
      }
      else if(tmp == 1u){
         thread_yield();
      }
      else{ //(tmp == 0u)
         try{
            intermodule_singleton_helpers::remove_old_gmem();
            ::new (&get_shm())managed_shared_memory(open_or_create, ShmName, ShmSize);
             intermodule_singleton_helpers::lock_file_logic f(get_shm());
            get_shm().atomic_func(f);
            if(f.retry_with_new_shm){
               get_shm().~managed_shared_memory();
               atomic_write32(&shm_initialized, 0);
            }
            else{
               atomic_write32(&shm_initialized, 2);
               break;
            }
         }
         catch(...){
            //
            throw;
         }
      }
   }
}

struct unlink_shmlogic
{
   unlink_shmlogic(managed_shared_memory &mshm)
      : mshm_(mshm)
   {}
   void operator()()
   {
      intermodule_singleton_helpers::locking_file_serial_id *pserial_id =
         mshm_.find<intermodule_singleton_helpers::locking_file_serial_id>
            ("lock_file_fd").first;
      assert(0 != pserial_id);
      if(1 == atomic_dec32(&pserial_id->ref_count)){
         int fd = pserial_id->fd;
         if(fd > 0){
            pserial_id->fd = intermodule_singleton_helpers::GMemMarkToBeRemoved;
            std::string s;
            intermodule_singleton_helpers::get_lock_file_path(s);
            delete_file(s.c_str());
            intermodule_singleton_helpers::close_lock_file(fd);
            intermodule_singleton_helpers::get_shm_name(s);
            shared_memory_object::remove(s.c_str());
         }
      }
   }
   managed_shared_memory &mshm_;
};

template<int Dummy>
void intermodule_singleton_common<Dummy>::destroy_shm()
{
   if(!atomic_read32(&shm_ref_count)){
      unlink_shmlogic f(get_shm());
      get_shm().atomic_func(f);
      (get_shm()).~managed_shared_memory();
      atomic_write32(&shm_initialized, 0u);
      intermodule_singleton_helpers::remove_old_gmem();
   }
}

template<int Dummy>
void intermodule_singleton_common<Dummy>::initialize_pc(void *&ptr, volatile boost::uint32_t &barrier, init_func_t init_func)
{
   // insert memory barrier
   if(atomic_read32(&barrier) != 2u){
      //Try to pass from 0 to 1, and insert memory barrier
      ::boost::uint32_t tmp_barrier = atomic_cas32(&barrier, 1, 0);
      if(tmp_barrier == 0u){
         try{
            initialize_shm();
            atomic_inc32(&shm_ref_count);
            //This can throw
            void *tmp = init_func(get_shm());
            //Barrier...
            atomic_write32(&barrier, 1u);
            //This won't throw
            ptr = tmp;
            //Memory barrier inserted, all previous operations should complete
            //before this one
            atomic_inc32(&barrier);
         }
         catch(...){
            //Mark singleton failed to initialize
            atomic_write32(&barrier, 3u);
            throw;
         }
      }
      else if(tmp_barrier == 1u){
         //Another thread is initializing the singleton, just wait
         while(1){
            tmp_barrier = atomic_read32(&barrier);
            if(tmp_barrier >= 2u){
               //Already initialized, or exception thrown by initializer thread
               break;
            }
            else if(tmp_barrier == 1u){
               detail::thread_yield();
            }
            else{
               //This can't be happening!
               assert(0);
            }
         }
      }
      if(tmp_barrier > 2u){
         //Exception thrown, singleton initialization failed
         throw interprocess_exception();
      }
   }
   assert(ptr != 0);
}


//Now this class is a classic singleton, initializing the singleton in
//the first get() function call

template<typename C>
class intermodule_singleton
{
   public:
   static C& get()   //Let's make inlining easy
   {
      if(!pc){
         if(deleter.dummy_function())  //This forces deleter instantiation, for reference counted destruction
            intermodule_singleton_common<0>::initialize_pc(pc, barrier, initialize);
      }
      return *static_cast<C*>(pc);
   }

   struct ref_count_ptr
   {
      ref_count_ptr(C *p, boost::uint32_t count)
         : ptr(p), ref_count(count)
      {}
      C *ptr;
      volatile boost::uint32_t ref_count;
   };

   private:
   //These will be zero-initialized without any constructor call dependency
   static void*                      pc;
   static volatile boost::uint32_t   barrier;
   //This class destructor will trigger singleton destruction
   static struct deleter_type
   {
      bool dummy_function()
      {  return m_dummy == 0; }

      ~deleter_type()
      {
         intermodule_singleton_common<0>::destroy_pc(pc, finalize);
      }
      //Dummy volatile so that the compiler can't resolve its value at compile-time
      //and can't avoid deleter_type instantiation if dummy_function() is called.
      static volatile int m_dummy;
   } deleter;

   
   struct init_atomic_func
   {
      init_atomic_func(managed_shared_memory &m)
         : mshm(m)
      {}
      void operator()()
      {
         ref_count_ptr *rcount = mshm.find<ref_count_ptr>(unique_instance).first;
         if(!rcount){
            C *p = new C();
            try{
               rcount = mshm.construct<ref_count_ptr>(unique_instance)(p, 0u);
            }
            catch(...){
               delete p;
               throw;
            }
         }
         atomic_inc32(&rcount->ref_count);
         ret_ptr = rcount->ptr;
      }
      managed_shared_memory &mshm;
      void *ret_ptr;
   };

   struct fini_atomic_func
   {
      fini_atomic_func(managed_shared_memory &m)
         : mshm(m)
      {}
      void operator()()
      {
         ref_count_ptr *rcount = mshm.find<ref_count_ptr>(unique_instance).first;
            //The object must exist
         assert(rcount);
         //Check if last reference
         if(atomic_dec32(&rcount->ref_count) == 1){
            //If last, destroy the object
            assert(rcount->ptr != 0);
            delete rcount->ptr;
            //Now destroy shm entry
            bool destroyed = mshm.destroy<ref_count_ptr>(unique_instance);
            (void)destroyed;  assert(destroyed == true);
         }
      }
      managed_shared_memory &mshm;
      void *ret_ptr;
   };


   static void *initialize(managed_shared_memory &mshm)
   {
      init_atomic_func f(mshm);
      mshm.atomic_func(f);
      return f.ret_ptr;
   }

   static void finalize(void *p, managed_shared_memory &mshm)
   {  (void)p;
      fini_atomic_func f(mshm);
      mshm.atomic_func(f);
   }
};

template <typename C>
volatile int intermodule_singleton<C>::deleter_type::m_dummy;

//These will be zero-initialized by the loader
template <typename C>
void *intermodule_singleton<C>::pc;

template <typename C>
volatile boost::uint32_t intermodule_singleton<C>::barrier;

template <typename C>
typename intermodule_singleton<C>::deleter_type intermodule_singleton<C>::deleter;
  

}  //namespace detail{
}  //namespace interprocess{
}  //namespace boost{

#include <boost/interprocess/detail/config_end.hpp>

#endif


/*






                       if try_lock_file
                          open_shm_1
open_or_create_shm_1         
                          atomic_1
                            mark_to_be_removed
                            remove_file
                            remove_shm
                            close
                                                
atomic_1     
   if not present fd
     if(!create_and_lock) //delete in course, try again later
       mark_to_be_removed
       remove_shm_1
       try_again
     register_fd
   else if to_be_removed
     try_again
   else if not_valid_fd
     mark_to_be_removed
     remove_shm_1
     try_again
   else if(valid_fd)
      continue;
















Hint: fcntl is a nightmare because:

- to check if a file is locked you must open an new descriptor
- if you close any descriptor to that file all locks on that file are gone
- so each time you want to check something is locked without disburbing
  anything, you must leak that descriptor.

Hints to have only one open descriptor:

- Mark the lock file with a magic string.
- Store the file descriptor in shared memory.
- Open or create the shared memory:
    - If created, no problem,
         create a file and lock it in a loop testing that the file exists
    - If opened, take the the stored fd:
        - Take a lock on shared memory, if time exceeds too much time, unlink it and start again
        - IF it's a regular file (IFREG, using fstat to check) AND 
          its size is the expected (fstat) AND
          contains the magic number.
              LOCK IT
          ELSE
             the fd is invalid so unlink the shared memory and try to create it again


REMOVE_OLD_GMEM()

lock semaphore

loop all files in /tmp/boost_interprocess/gmem
  check pattern(name) #PID#.lock
  if (!pattern)
    continue;
  pid = extract_pid(name)
  if(pid == self_pid)
     continue;
  open(name);
  if(::fnctl(fd, GET_LCK)
     assert(flock.pid != self_pid)
     continue;
  shm_remove(bip_gmem_#PID#)
  unlink(name);

unlock_semaphore



STATIC_SINGLETON_CONSTRUCTOR 

remove_old_gmem()

  managed_shared_memory shm;
  bool completed = false;
  while(!completed){
     exclusive_lock(semaphore);
     try{
       managed_shared_memory tmp(create_only, bip_gmem_#SELFPID#, SIZE);
       shm = ::boost::move(tmp);
       if(!::open(/tmp/boost_interprocess/gmem/#PID#.lock) ||
          !::fnctl(fd, LOCK)
         throw whatever;
       leave file locked;
     }
     catch(interprocess_exception &e){
       if(e.error_code() != already_exists_error)){
         throw;
       }
       try{
          managed_shared_memory tmp(open_only, bip_gmem_#SELFPID#)
          shm = ::boost::move(tmp);
       }
       catch(...){
          if(e.error_code() != not_found_error)){
            throw;
          }
       }
     }
     completed = true;
  }
  ref_counted_type *rcount = shm.find_or_create<ref_counted_type>(unique_instance)(int(0));
  atomic_inc(rcount->cnt);

STATIC_SINGLETON_DESTRUCTOR

remove_old_mem()

exclusive_lock(semaphore)
  ref_count_type *rcount = shm.find_or_create<ref_count_type>("ref_count")(int(0));
  if(!atomic_dec(rcount->cnt))
    shm.destroy<T>(unique_instance);

*/
