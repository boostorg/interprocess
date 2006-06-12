//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_NAMED_RECURSIVE_MUTEX_HPP
#define BOOST_INTERPROCESS_NAMED_RECURSIVE_MUTEX_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/noncopyable.hpp>

#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
#  include <boost/interprocess/sync/win32/win32_sync_primitives.hpp>
#  include <boost/interprocess/sync/named_mutex.hpp>

#else    //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

#include <unistd.h>     //close
#include <string>       //std::string
#include <pthread.h>    //pthread_* family
#include <boost/interprocess/sync/posix/pthread_helpers.hpp>
#  include <boost/interprocess/sync/named_mutex.hpp>
#endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

/*!\file
   Describes a named interprocess_mutex class for inter-process synchronization
*/

namespace boost {
namespace interprocess {

/*!A recursive interprocess_mutex with a global name, so it can be found from different 
   processes. This interprocess_mutex can't be placed in shared memory, and
   each process should have it's own interprocess_mutex.*/
class named_recursive_mutex : private boost::noncopyable
{
 public:
   /*!Creates a global interprocess_mutex with a name.*/
   named_recursive_mutex(detail::create_only_t create_only, const char *name);

   /*!Opens or creates a global interprocess_mutex with a name. 
      If the interprocess_mutex is created, this call is equivalent to create(). 
      If the interprocess_mutex is already created, this call is equivalent to open(). 
      Does not throw*/
   named_recursive_mutex(detail::open_or_create_t open_or_create, const char *name);

   /*!Opens a global interprocess_mutex with a name if that interprocess_mutex is previously.
      created. If it is not previously created this function return false.
      Does not throw*/
   named_recursive_mutex(detail::open_only_t open_only, const char *name);

   /*!Destroys named interprocess_mutex. Does not throw*/
   ~named_recursive_mutex();

   /*!Unlocks a previously locked interprocess_mutex.*/
   void unlock();

   /*!Locks interprocess_mutex, sleeps when interprocess_mutex is already locked.
      Throws interprocess_exception if a severe error is found*/
   void lock();

   /*! Tries to lock the interprocess_mutex, returns false when interprocess_mutex 
      is already locked, returns true when success.
      Throws interprocess_exception if a severe error is found*/
   bool try_lock();

   /*! Tries to lock the interprocess_mutex until time abs_time,
      Returns false when timeout expires, returns true when locks.
      Throws interprocess_exception if a severe error is found*/
   bool timed_lock(const boost::posix_time::ptime &abs_time);

   /*! Erases a named recursive mutex from the system*/
   static bool remove(const char *name);

 private:
   named_mutex    m_mutex;
   #if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   typedef unsigned long   thread_type;
   static thread_type s_get_invalid_thread_id(){ return thread_type(0xffffffff); }
   static thread_type s_get_current_thread_id(){ return winapi::current_thread_id(); }
   static bool s_thread_equal(thread_type a, thread_type b){ return a ==  b; }
   #else    //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   typedef pthread_t       thread_type;
   static thread_type s_get_invalid_thread_id(){ return thread_type(false); }
   static thread_type s_get_current_thread_id(){ return pthread_self(); }
   static bool s_thread_equal(thread_type a, thread_type b){ return pthread_equal(a, b) != 0; }
   #endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   thread_type    m_valid_id;
   unsigned int   m_count;
};

inline named_recursive_mutex::~named_recursive_mutex()
{}

inline named_recursive_mutex::named_recursive_mutex
   (detail::create_only_t, const char *name)
   :  m_mutex(boost::interprocess::create_only, name)
   ,  m_valid_id(named_recursive_mutex::s_get_invalid_thread_id())
   ,  m_count(0)
{}

inline named_recursive_mutex::named_recursive_mutex
   (detail::open_or_create_t, const char *name)
   :  m_mutex(boost::interprocess::open_or_create, name)
   ,  m_valid_id(named_recursive_mutex::s_get_invalid_thread_id())
   ,  m_count(0)
{}


inline named_recursive_mutex::named_recursive_mutex
   (detail::open_only_t, const char *name)
   :  m_mutex(boost::interprocess::open_only, name)
   ,  m_valid_id(named_recursive_mutex::s_get_invalid_thread_id())
   ,  m_count(0)
{}

inline void named_recursive_mutex::lock()
{
   thread_type tid = named_recursive_mutex::s_get_current_thread_id();
   if (named_recursive_mutex::s_thread_equal(m_valid_id, tid)){
      ++m_count;
   }
   else{
      m_mutex.lock();
      m_valid_id = tid;
      ++m_count;
   }
}

inline void named_recursive_mutex::unlock()
{
   thread_type tid = named_recursive_mutex::s_get_current_thread_id();
   assert(named_recursive_mutex::s_thread_equal(m_valid_id, tid));

   --m_count;
   if(!m_count){
      m_valid_id = named_recursive_mutex::s_get_invalid_thread_id();
      m_mutex.unlock();
   }
}

inline bool named_recursive_mutex::try_lock()
{
   thread_type tid = named_recursive_mutex::s_get_current_thread_id();
   if (named_recursive_mutex::s_thread_equal(m_valid_id, tid)){
      ++m_count;
      return true;
   }
   else if(m_mutex.try_lock()){
      m_valid_id = tid;
      ++m_count;
      return true;
   }
   return false;
}

inline bool named_recursive_mutex::timed_lock(const boost::posix_time::ptime &abs_time)
{
   thread_type tid = named_recursive_mutex::s_get_current_thread_id();
   if (named_recursive_mutex::s_thread_equal(m_valid_id, tid)){
      ++m_count;
      return true;
   }
   else if(m_mutex.timed_lock(abs_time)){
      m_valid_id = tid;
      ++m_count;
      return true;
   }
   return false;
}

inline bool named_recursive_mutex::remove(const char *name)
{  return named_mutex::remove(name); }

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_NAMED_RECURSIVE_MUTEX_HPP
