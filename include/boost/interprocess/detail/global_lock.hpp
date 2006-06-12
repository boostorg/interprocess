//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_GLOBAL_LOCK_HPP
#define BOOST_INTERPROCESS_DETAIL_GLOBAL_LOCK_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
#  include <boost/interprocess/sync/win32/win32_sync_primitives.hpp>
#else

#  ifdef BOOST_HAS_UNISTD_H
#    include <fcntl.h>        //O_CREAT, O_*... 
#    include <unistd.h>       //ftruncate, close
#    include <semaphore.h>  //sem_t* family, SEM_VALUE_MAX
#    include <sys/stat.h>     //mode_t, S_IRWXG, S_IRWXO, S_IRWXU,
#  else
#    error Unknown platform
#  endif

#endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

namespace boost {

namespace interprocess {

namespace detail {

class global_lock
{
   public:
   global_lock();
   bool acquire();
   ~global_lock();
   void release();
   private:
   #if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   void    *m_mut;
   #else
   sem_t   *m_sem;
   #endif
   bool     m_acquired;
};

inline global_lock::~global_lock()
{  this->release();  }

#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

inline global_lock::global_lock()
   : m_mut(0), m_acquired(false){}
   
inline bool global_lock::acquire()
{  
   m_mut = winapi::create_mutex("/boost_shmem_shm_global_mutex");  
   if(m_mut == 0)
      return false;
   unsigned long ret = winapi::wait_for_single_object(m_mut, winapi::infinite_time);
   //Other thread has abandoned this without closing the handle
   if(ret == winapi::wait_abandoned){
      ret = winapi::wait_for_single_object(m_mut, winapi::infinite_time);
   }
   m_acquired = ret == winapi::wait_object_0; 
   return m_acquired;  
}

inline void global_lock::release()
{
   if(m_acquired){
      winapi::release_mutex(m_mut);
      m_acquired = false;
   }

   if(m_mut){
      winapi::close_handle(m_mut);
      m_mut = 0;
   }
}

#else    //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

inline global_lock::global_lock()
   : m_sem((sem_t*)-1), m_acquired(false){}
   
inline bool global_lock::acquire()
{  
   mode_t mode = S_IRWXG | S_IRWXO | S_IRWXU;
   m_sem = sem_open("/boost_shmem_shm_global_mutex", O_CREAT, mode, 1);
   if(m_sem == ((sem_t*)-1))
      return false;
   m_acquired = sem_wait(m_sem) == 0; 
   return m_acquired;  
}

inline void global_lock::release()
{
   if(m_acquired){
      sem_post(m_sem);
      m_acquired = false;
   }

   if(m_sem != ((sem_t*)-1)){
      sem_close(m_sem);
      m_sem = ((sem_t*)-1);
   }
}

#endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

}  //namespace detail {

}  //namespace interprocess {

}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_DETAIL_GLOBAL_LOCK_HPP
