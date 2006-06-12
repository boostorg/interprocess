//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_CONDITION_HPP
#define BOOST_INTERPROCESS_CONDITION_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

//#include <boost/date_time/posix_time/ptime.hpp>
//#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/limits.hpp>
#include <cassert>

#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
#  include <boost/interprocess/sync/win32/win32_sync_primitives.hpp>
#else
#  include <errno.h>
#  include <pthread.h>
#endif

/*!\file
   Describes process-shared variables interprocess_condition class
*/

namespace boost {

namespace posix_time
{  class ptime;   }

namespace interprocess {

class interprocess_condition : private noncopyable
{
 public:
   /*!Constructs a interprocess_condition*/
   interprocess_condition();

   /*!Destroys *this*/
   ~interprocess_condition();

   /*!If there is a thread waiting on *this, change that 
      thread's state to ready. Otherwise there is no effect.*/
   void notify_one();

   /*!Change the state of all threads waiting on *this to ready.
      If there are no waiting threads, notify_all() has no effect.*/
   void notify_all();

   /*!Releases the lock on the interprocess_mutex object associated with lock, blocks 
      the current thread of execution until readied by a call to 
      this->notify_one() or this->notify_all(), and then reacquires the lock.*/
   template <typename L>
   void wait(L& lock)
   {
      if (!lock)
         throw lock_exception();
      do_wait(*lock.interprocess_mutex());
   }

   /*!The same as: while (!pred()) wait(lock)*/
   template <typename L, typename Pr>
   void wait(L& lock, Pr pred)
   {
      if (!lock)
         throw lock_exception();

      while (!pred())
         do_wait(*lock.interprocess_mutex());
   }

   /*!Releases the lock on the interprocess_mutex object associated with lock, blocks 
      the current thread of execution until readied by a call to 
      this->notify_one() or this->notify_all(), or until time abs_time is reached, 
      and then reacquires the lock.
      Returns: false if time abs_time is reached, otherwise true.*/
   template <typename L>
   bool timed_wait(L& lock, const boost::posix_time::ptime &abs_time)
   {
      if (!lock)
            throw lock_exception();

      return do_timed_wait(abs_time, *lock.interprocess_mutex());
   }

   /*!The same as:   while (!pred()) { 
                        if (!timed_wait(lock, abs_time)) return false; 
                     } return true;*/
   template <typename L, typename Pr>
   bool timed_wait(L& lock, const boost::posix_time::ptime &abs_time, Pr pred)
   {
      if (!lock)
            throw lock_exception();

      while (!pred()){
            if (!do_timed_wait(abs_time, *lock.interprocess_mutex()))
               return false;
      }

      return true;
   }

 private:
   void do_wait(interprocess_mutex  &mut);
   bool do_timed_wait(const boost::posix_time::ptime &abs_time, interprocess_mutex &mut);
#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   enum { SLEEP, NOTIFY_ONE, NOTIFY_ALL };
   interprocess_mutex m_enter_mut;
   interprocess_mutex     m_check_mut;
   volatile long    m_command;
   volatile long    m_num_waiters;
   bool do_timed_wait(bool tout_enabled, const boost::posix_time::ptime &abs_time, interprocess_mutex &mut);
   void notify(long command);
#else
   pthread_cond_t   m_condition;
#endif
};

}  //namespace interprocess

}  // namespace boost

#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
#  include <boost/interprocess/sync/win32/interprocess_condition.hpp>
#else 
#  include <boost/interprocess/sync/posix/interprocess_condition.hpp>
#endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

#include <boost/interprocess/detail/config_end.hpp>

#endif // BOOST_INTERPROCESS_CONDITION_HPP
