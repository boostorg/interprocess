//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
//
// Parts of the pthread code come from Boost Threads code:
//
//////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2001-2003
// William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/sync/posix/ptime_to_timespec.hpp>
#include <boost/interprocess/detail/posix_time_types_wrk.hpp>

namespace boost {

namespace interprocess {

#if (_POSIX_VERSION >= 200112L || _XOPEN_VERSION >= 500) && defined _POSIX_TIMEOUTS
inline interprocess_recursive_mutex::interprocess_recursive_mutex()
{
   detail::mutexattr_wrapper mut_attr(true);
   detail::mutex_initializer mut(m_mut, mut_attr);
   mut.release();
}

inline interprocess_recursive_mutex::~interprocess_recursive_mutex()
{
   int res = pthread_mutex_destroy(&m_mut);
   assert(res == 0);(void)res;
}

inline void interprocess_recursive_mutex::lock()
{
   int res = 0;
   res = pthread_mutex_lock(&m_mut);
   if (res == EDEADLK) throw lock_exception();
   assert(res == 0);
}

inline bool interprocess_recursive_mutex::try_lock()
{
   int res = 0;
   res = pthread_mutex_trylock(&m_mut);
   if (res == EDEADLK) throw lock_exception();
   assert(res == 0 || res == EBUSY);
   return res == 0;
}

inline bool interprocess_recursive_mutex::timed_lock(const boost::posix_time::ptime &abs_time)
{
   timespec ts = detail::ptime_to_timespec(abs_time);
   int res = pthread_mutex_timedlock(&m_mut, &ts);
   if (res == EDEADLK) throw lock_exception();
   assert(res == 0 || res == EBUSY);
   return res == 0;
}

inline void interprocess_recursive_mutex::unlock()
{
   int res = 0;
   res = pthread_mutex_unlock(&m_mut);
   if (res == EPERM) throw lock_exception();
   assert(res == 0);
}

#else //#if (_POSIX_VERSION >= 200112L || _XOPEN_VERSION >= 500) && defined _POSIX_TIMEOUTS

inline interprocess_recursive_mutex::interprocess_recursive_mutex()
    : m_valid_id(false), m_count(0)
{
   //Mutex init
   detail::mutexattr_wrapper mut_attr;
   detail::mutex_initializer mut(m_mut, mut_attr);

   //Condition init
   detail::condattr_wrapper cond_attr;
   detail::condition_initializer cond(m_unlocked, cond_attr);

   mut.release(); 
   cond.release(); 
}

inline interprocess_recursive_mutex::~interprocess_recursive_mutex()
{
   int res = 0;
   res = pthread_mutex_destroy(&m_mut);
   assert(res == 0);

   res = pthread_cond_destroy(&m_unlocked);
   assert(res == 0);
}

inline void interprocess_recursive_mutex::lock()
{
   int res = 0;
   res = pthread_mutex_lock(&m_mut);
   if(res != 0){
      throw inteprocess_exception(system_error_code());
   }
   assert(res == 0);

   pthread_t tid = pthread_self();
   if (m_valid_id && pthread_equal(m_thread_id, tid)){
      ++m_count;
   }
   else{
      while (m_valid_id)
      {
         res = pthread_cond_wait(&m_unlocked, &m_mut);
         assert(res == 0);
      }

      m_thread_id = tid;
      m_valid_id = true;
      m_count = 1;
   }

   res = pthread_mutex_unlock(&m_mut);
   assert(res == 0);
}

inline bool interprocess_recursive_mutex::try_lock()
{
   int res = 0;
   res = pthread_mutex_lock(&m_mut);
   assert(res == 0);

   bool ret = false;
   pthread_t tid = pthread_self();
   if (m_valid_id && pthread_equal(m_thread_id, tid)){
      ++m_count;
      ret = true;
   }
   else if (!m_valid_id){
      m_thread_id = tid;
      m_valid_id = true;
      m_count = 1;
      ret = true;
   }

   res = pthread_mutex_unlock(&m_mut);
   assert(res == 0);
   return ret;
}

inline bool interprocess_recursive_mutex::timed_lock(const boost::posix_time::ptime &abs_time)
{
   int res = 0;
   res = pthread_mutex_lock(&m_mut);
   assert(res == 0);

   bool ret = false;
   pthread_t tid = pthread_self();
   if (m_valid_id && pthread_equal(m_thread_id, tid)){
      ++m_count;
      ret = true;
   }
   else{
      timespec ts = detail::ptime_to_timespec(abs_time);

      while (m_valid_id){
         res = pthread_cond_timedwait(&m_unlocked, &m_mut, &ts);
         if (res == ETIMEDOUT)
               break;
         assert(res == 0);
      }

      if (!m_valid_id){
         m_thread_id = tid;
         m_valid_id = true;
         m_count = 1;
         ret = true;
      }
   }

   res = pthread_mutex_unlock(&m_mut);
   assert(res == 0);
   return ret;
}

inline void interprocess_recursive_mutex::unlock()
{
   int res = 0;
   res = pthread_mutex_lock(&m_mut);
   assert(res == 0);

   pthread_t tid = pthread_self();
   if (m_valid_id && !pthread_equal(m_thread_id, tid)){
      res = pthread_mutex_unlock(&m_mut);
      assert(res == 0);
      throw lock_exception();
   }

   if (--m_count == 0){
      assert(m_valid_id);
      m_valid_id = false;

      res = pthread_cond_signal(&m_unlocked);
      assert(res == 0);
   }

   res = pthread_mutex_unlock(&m_mut);
   assert(res == 0);
}

#endif   //_POSIX_VERSION >= 200112L

}  //namespace interprocess {

}  //namespace boost {
