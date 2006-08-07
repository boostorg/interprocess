//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_OS_FILE_FUNCTIONS_HPP
#define BOOST_INTERPROCESS_DETAIL_OS_FILE_FUNCTIONS_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
#  include <boost/interprocess/sync/win32/win32_sync_primitives.hpp>
#else
#  ifdef BOOST_HAS_UNISTD_H
#    include <fcntl.h>
#    include <unistd.h>
#  else
#    error Unknown platform
#  endif
#endif

#include <cstring>

namespace boost {
namespace interprocess {
namespace detail{

#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

typedef void * OS_file_handle_t;

inline OS_file_handle_t create_new_file(const char *name)
{  
   return winapi::create_file
      (name, winapi::generic_read | winapi::generic_write, winapi::create_new);  
}

inline OS_file_handle_t open_existing_file(const char *name)
{  
   return winapi::create_file
      (name, winapi::generic_read | winapi::generic_write, winapi::open_existing);  
}

inline bool truncate_file (OS_file_handle_t hnd, std::size_t size)
{  
   if(!winapi::set_file_pointer_ex(hnd, size, 0, winapi::file_begin)){
      return false;
   }

   if(!winapi::set_end_of_file(hnd)){
      return false;
   }
   return true;
}

inline OS_file_handle_t invalid_file()
{  return winapi::invalid_handle_value;  }

inline bool close_file(OS_file_handle_t hnd)
{  return 0 != winapi::close_handle(hnd);   }

inline bool acquire_file_lock(OS_file_handle_t hnd)
{  
   static winapi::interprocess_overlapped overlapped;
   const unsigned long len = 0xffffffff;
//   winapi::interprocess_overlapped overlapped;
//   std::memset(&overlapped, 0, sizeof(overlapped));
   return winapi::lock_file_ex
      (hnd, winapi::lockfile_exclusive_lock, 0, len, len, &overlapped);
}

inline bool try_acquire_file_lock(OS_file_handle_t hnd, bool &acquired)
{  
   const unsigned long len = 0xffffffff;
   winapi::interprocess_overlapped overlapped;
   std::memset(&overlapped, 0, sizeof(overlapped));
   if(!winapi::lock_file_ex
      (hnd, winapi::lockfile_exclusive_lock | winapi::lockfile_fail_immediately, 
       0, len, len, &overlapped)){
      return winapi::get_last_error() == winapi::error_lock_violation ? 
               acquired = false, true : false;
   
   }
   return (acquired = true);
}

inline bool timed_acquire_file_lock
   (OS_file_handle_t hnd, bool &acquired, const boost::posix_time::ptime &abs_time)
{  
   //Obtain current count and target time
   boost::posix_time::ptime now = 
      boost::posix_time::microsec_clock::universal_time();
   using namespace boost::detail;

   if(now >= abs_time) return false;

   do{
      if(!try_acquire_file_lock(hnd, acquired))
         return false;

      if(acquired)
         return true;
      else{
         now = boost::posix_time::microsec_clock::universal_time();

         if(now >= abs_time){
            acquired = false;
            return true;
         }
         // relinquish current time slice
         winapi::sched_yield();
      }
   }while (true);
}

inline bool release_file_lock(OS_file_handle_t hnd)
{  
   const unsigned long len = 0xffffffff;
   winapi::interprocess_overlapped overlapped;
   std::memset(&overlapped, 0, sizeof(overlapped));
   return winapi::unlock_file_ex(hnd, 0, len, len, &overlapped);
}

inline bool acquire_file_lock_sharable(OS_file_handle_t hnd)
{  
   const unsigned long len = 0xffffffff;
   winapi::interprocess_overlapped overlapped;
   std::memset(&overlapped, 0, sizeof(overlapped));
   return winapi::lock_file_ex(hnd, 0, 0, len, len, &overlapped);
}

inline bool try_acquire_file_lock_sharable(OS_file_handle_t hnd, bool &acquired)
{  
   const unsigned long len = 0xffffffff;
   winapi::interprocess_overlapped overlapped;
   std::memset(&overlapped, 0, sizeof(overlapped));
   if(!winapi::lock_file_ex
      (hnd, winapi::lockfile_fail_immediately, 0, len, len, &overlapped)){
      return winapi::get_last_error() == winapi::error_lock_violation ? 
               acquired = false, true : false;
   }
   return (acquired = true);
}

inline bool timed_acquire_file_lock_sharable
   (OS_file_handle_t hnd, bool &acquired, const boost::posix_time::ptime &abs_time)
{  
   //Obtain current count and target time
   boost::posix_time::ptime now = 
      boost::posix_time::microsec_clock::universal_time();
   using namespace boost::detail;

   if(now >= abs_time) return false;

   do{
      if(!try_acquire_file_lock_sharable(hnd, acquired))
         return false;

      if(acquired)
         return true;
      else{
         now = boost::posix_time::microsec_clock::universal_time();

         if(now >= abs_time){
            acquired = false;
            return true;
         }
         // relinquish current time slice
         winapi::sched_yield();
      }
   }while (true);
}

inline bool release_file_lock_sharable(OS_file_handle_t hnd)
{  return release_file_lock(hnd);   }

#else    //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

typedef int    OS_file_handle_t;

inline OS_file_handle_t create_new_file(const char *name)
{  
   ::mode_t mode = S_IRWXG | S_IRWXO | S_IRWXU;
   return ::open(name, O_RDWR | O_EXCL | O_CREAT, mode); 
}

inline OS_file_handle_t open_existing_file(const char *name)
{  
   ::mode_t mode = S_IRWXG | S_IRWXO | S_IRWXU;
   return ::open(name, O_RDWR, mode); 
}

inline bool truncate_file (OS_file_handle_t hnd, std::size_t size)
{  return 0 == ::ftruncate(hnd, size);   }

inline OS_file_handle_t invalid_file()
{  return -1;  }

inline bool close_file(OS_file_handle_t hnd)
{  return ::close(hnd) == 0;   }

inline bool acquire_file_lock(OS_file_handle_t hnd)
{
   struct ::flock lock;
   lock.l_type    = F_WRLCK;
   lock.l_whence  = SEEK_SET;
   lock.l_start   = 0;
   lock.l_len     = 0;
   return -1 != ::fcntl(hnd, F_SETLKW, &lock);
}

inline bool try_acquire_file_lock(OS_file_handle_t hnd, bool &acquired)
{
   struct ::flock lock;
   lock.l_type    = F_WRLCK;
   lock.l_whence  = SEEK_SET;
   lock.l_start   = 0;
   lock.l_len     = 0;
   int ret = ::fcntl(hnd, F_SETLK, &lock);
   if(ret == -1){
      return (errno != EAGAIN && errno != EACCES) ?
               acquired = false, true : false;
   }
   return (acquired = true);
}

inline bool timed_acquire_file_lock
   (OS_file_handle_t hnd, bool &acquired, const boost::posix_time::ptime &abs_time)
{
   //Obtain current count and target time
   boost::posix_time::ptime now = 
      boost::posix_time::microsec_clock::universal_time();
   using namespace boost::detail;

   if(now >= abs_time) return false;

   do{
      if(!try_acquire_file_lock(hnd, acquired))
         return false;

      if(acquired)
         return true;
      else{
         now = boost::posix_time::microsec_clock::universal_time();

         if(now >= abs_time){
            acquired = false;
            return true;
         }
         // relinquish current time slice
         sleep(0);
      }
   }while (true);
}

inline bool release_file_lock(OS_file_handle_t hnd)
{
   return 0 == lockf(hnd, F_ULOCK, 0);
/*
   struct ::flock lock;
   lock.l_type    = F_UNLCK;
   lock.l_whence  = SEEK_SET;
   lock.l_start   = 0;
   lock.l_len     = 0;
   return -1 != ::fcntl(hnd, F_SETLK, &lock);
*/
}

inline bool acquire_file_lock_sharable(OS_file_handle_t hnd)
{  
   struct ::flock lock;
   lock.l_type    = F_RDLCK;
   lock.l_whence  = SEEK_SET;
   lock.l_start   = 0;
   lock.l_len     = 0;
   return -1 != ::fcntl(hnd, F_SETLKW, &lock);
}

inline bool try_acquire_file_lock_sharable(OS_file_handle_t hnd, bool &acquired)
{  
   struct flock lock;
   lock.l_type    = F_RDLCK;
   lock.l_whence  = SEEK_SET;
   lock.l_start   = 0;
   lock.l_len     = 0;
   int ret = ::fcntl(hnd, F_SETLK, &lock);
   if(ret == -1){
      return (errno != EAGAIN && errno != EACCES) ? 
               acquired = false, true : false;
   }
   return (acquired = true);
}

inline bool timed_acquire_file_lock_sharable
   (OS_file_handle_t hnd, bool &acquired, const boost::posix_time::ptime &abs_time)
{  
   //Obtain current count and target time
   boost::posix_time::ptime now = 
      boost::posix_time::microsec_clock::universal_time();
   using namespace boost::detail;

   if(now >= abs_time) return false;

   do{
      if(!try_acquire_file_lock_sharable(hnd, acquired))
         return false;

      if(acquired)
         return true;
      else{
         now = boost::posix_time::microsec_clock::universal_time();

         if(now >= abs_time){
            acquired = false;
            return true;
         }
         // relinquish current time slice
         ::sleep(0);
      }
   }while (true);
}

inline bool release_file_lock_sharable(OS_file_handle_t hnd)
{  return release_file_lock(hnd);   }

#endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

}  //namespace detail{
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_DETAIL_OS_FILE_FUNCTIONS_HPP
