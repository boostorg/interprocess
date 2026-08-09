//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2004-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
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

#ifndef BOOST_INTERPROCESS_TEST_MUTEX_TEST_TEMPLATE_HEADER
#define BOOST_INTERPROCESS_TEST_MUTEX_TEST_TEMPLATE_HEADER

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/exceptions.hpp>
#include "boost_interprocess_check.hpp"
#include "util.hpp"
#include <boost/interprocess/detail/os_thread_functions.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <typeinfo>
#include <iostream>

namespace boost { namespace interprocess { namespace test {

template <typename M>
struct test_lock
{
   typedef M mutex_type;
   typedef boost::interprocess::scoped_lock<mutex_type> lock_type;


   void operator()()
   {
      mutex_type interprocess_mutex;

      // Test the lock's constructors.
      {
         lock_type lock(interprocess_mutex, boost::interprocess::defer_lock);
         BOOST_INTERPROCESS_CHECK(!lock);
      }
      lock_type lock(interprocess_mutex);
      BOOST_INTERPROCESS_CHECK(lock ? true : false);

      // Test the lock and unlock methods.
      lock.unlock();
      BOOST_INTERPROCESS_CHECK(!lock);
      lock.lock();
      BOOST_INTERPROCESS_CHECK(lock ? true : false);
   }
};

template <typename M>
struct test_trylock
{
   typedef M mutex_type;
   typedef boost::interprocess::scoped_lock<mutex_type> try_to_lock_type;

   void operator()()
   {
      mutex_type interprocess_mutex;

      // Test the lock's constructors.
      {
         try_to_lock_type lock(interprocess_mutex, boost::interprocess::try_to_lock);
         BOOST_INTERPROCESS_CHECK(lock ? true : false);
      }
      {
         try_to_lock_type lock(interprocess_mutex, boost::interprocess::defer_lock);
         BOOST_INTERPROCESS_CHECK(!lock);
      }
      try_to_lock_type lock(interprocess_mutex);
      BOOST_INTERPROCESS_CHECK(lock ? true : false);

      // Test the lock, unlock and trylock methods.
      lock.unlock();
      BOOST_INTERPROCESS_CHECK(!lock);
      lock.lock();
      BOOST_INTERPROCESS_CHECK(lock ? true : false);
      lock.unlock();
      BOOST_INTERPROCESS_CHECK(!lock);
      BOOST_INTERPROCESS_CHECK(lock.try_lock());
      BOOST_INTERPROCESS_CHECK(lock ? true : false);
   }
};

template <typename M>
struct test_timedlock
{
   typedef M mutex_type;
   typedef boost::interprocess::scoped_lock<mutex_type> timed_lock_type;

   void operator()()
   {
      mutex_type interprocess_mutex;

      // Test the lock's constructors.
      {
         // Construct and initialize an ptime for a fast time out.
         timed_lock_type lock(interprocess_mutex, ptime_delay_ms(unsigned(1*BaseMs)));
         BOOST_INTERPROCESS_CHECK(lock ? true : false);
      }
      {
         timed_lock_type lock(interprocess_mutex, boost::interprocess::defer_lock);
         BOOST_INTERPROCESS_CHECK(!lock);
      }
      timed_lock_type lock(interprocess_mutex);
      BOOST_INTERPROCESS_CHECK(lock ? true : false);

      // Test the lock, unlock and timedlock methods.
      lock.unlock();
      BOOST_INTERPROCESS_CHECK(!lock);
      lock.lock();
      BOOST_INTERPROCESS_CHECK(lock ? true : false);
      lock.unlock();
      BOOST_INTERPROCESS_CHECK(!lock);
      BOOST_INTERPROCESS_CHECK(lock.timed_lock(boost_systemclock_delay_ms(1*BaseMs)));
      BOOST_INTERPROCESS_CHECK(lock ? true : false);
   }
};

template <class Lock, class Mutex, class TimePoint>
void lock_twice_timed(Mutex& mx, const TimePoint& pt)
{
   Lock lock1(mx, pt);
   Lock lock2(mx, pt);
}

template <typename M>
struct test_recursive_lock
{
   typedef M mutex_type;
   typedef boost::interprocess::scoped_lock<mutex_type> lock_type;

   void operator()()
   {
      mutex_type mx;
      {
         lock_type lock1(mx);
         lock_type lock2(mx);
      }
      {
         lock_type lock1(mx, defer_lock);
         lock_type lock2(mx, defer_lock);
      }
      {
         lock_type lock1(mx, try_to_lock);
         lock_type lock2(mx, try_to_lock);
      }
      {
         //This should always lock
         lock_twice_timed<lock_type>(mx, ptime_delay_ms(2*BaseMs));
      }
      {
         //This should always lock
         lock_twice_timed<lock_type>(mx, boost_systemclock_delay_ms(2*BaseMs));
      }
      {
         //This should always lock
         lock_twice_timed<lock_type>(mx, std_systemclock_delay_ms(2*BaseMs));
      }
   }
};

// plain_exclusive exercises the "infinite" lock for each
//   read_write_mutex type.

template<typename M>
void lock_and_sleep(void *arg, M &sm)
{
   data<M> *pdata = static_cast<data<M>*>(arg);
   boost::interprocess::scoped_lock<M> l(sm);
   //Announce the lock is owned, so that the launcher does not need to guess it
   //with a sleep, which is unreliable under heavy CPU load
   pdata->m_acquired.signal();
   if(pdata->m_block){
      //Keep the lock until the test says otherwise
      BOOST_INTERPROCESS_CHECK(pdata->m_release.wait());
   }
   else if(pdata->m_msecs){
      boost::interprocess::ipcdetail::thread_sleep_ms(unsigned(pdata->m_msecs));
   }
   else{
      boost::interprocess::ipcdetail::thread_sleep_ms(unsigned(2*BaseMs));
   }

   ++shared_val;
   pdata->m_value = shared_val;
}

template<typename M>
void lock_and_catch_errors(void *arg, M &sm)
{
   data<M>* pdata = static_cast<data<M>*>(arg);
   const boost::uint64_t start_us = elapsed_now_us();
   BOOST_INTERPROCESS_TRY
   {
      lock_and_sleep(arg, sm);
   }
   BOOST_INTERPROCESS_CATCH(interprocess_exception const & e)
   {
      //Record how long the locking attempt took, so that the test can tell a
      //failure caused by the timeout from a failure happening right away
      pdata->m_elapsed_us = elapsed_now_us() - start_us;
      pdata->m_error = e.get_error_code();
   } BOOST_INTERPROCESS_CATCH_END
}

template<typename M>
void try_lock_and_sleep(void *arg, M &sm)
{
   data<M> *pdata = static_cast<data<M>*>(arg);
   boost::interprocess::scoped_lock<M> l(sm, boost::interprocess::defer_lock);
   if (l.try_lock()){
      pdata->m_acquired.signal();
      if(pdata->m_block){
         //Keep the lock until the test says otherwise
         BOOST_INTERPROCESS_CHECK(pdata->m_release.wait());
      }
      else{
         boost::interprocess::ipcdetail::thread_sleep_ms(unsigned(2*BaseMs));
      }
      ++shared_val;
      pdata->m_value = shared_val;
   }
}

enum ETimedLockFlags
{
   TimedLock = 0,
   TryLockUntil = 1,
   TryLockFor = 2,
   ETimedLockFlagsMax
};

template<typename M>
void timed_lock_and_sleep(void *arg, M &sm)
{
   data<M> *pdata = static_cast<data<M>*>(arg);
   boost::interprocess::scoped_lock<M>
      l (sm, boost::interprocess::defer_lock);
   bool r = false;
   if(pdata->m_flags == (int)TimedLock){
      r = l.timed_lock(std_systemclock_delay_ms(unsigned(pdata->m_msecs)));
   }
   else if (pdata->m_flags == (int)TryLockUntil) {
      r = l.try_lock_until(ptime_delay_ms(unsigned(pdata->m_msecs)));
   }
   else if (pdata->m_flags == (int)TryLockFor) {
      r = l.try_lock_for(boost_systemclock_ms(unsigned(pdata->m_msecs)));
   }

   if (r){
      pdata->m_acquired.signal();
      if(pdata->m_block){
         //Keep the lock until the test says otherwise
         BOOST_INTERPROCESS_CHECK(pdata->m_release.wait());
      }
      else{
         boost::interprocess::ipcdetail::thread_sleep_ms(unsigned(2*BaseMs));
      }
      ++shared_val;
      pdata->m_value = shared_val;
   }
}

template<typename M>
void test_mutex_lock()
{
   shared_val = 0;

   M mtx;

   data<M> d1(1);
   data<M> d2(2);

   // Locker one launches, holds the lock for 2*BaseMs seconds.
   boost::interprocess::ipcdetail::OS_thread_t tm1;
   boost::interprocess::ipcdetail::thread_launch(tm1, thread_adapter<M>(&lock_and_sleep, &d1, mtx));

   //Wait until tm1 really owns the lock, so that the order in which both
   //threads take it is guaranteed no matter how loaded the machine is
   BOOST_INTERPROCESS_CHECK(d1.m_acquired.wait());

   // Locker two launches, but it won't hold the lock for 2*BaseMs seconds.
   boost::interprocess::ipcdetail::OS_thread_t tm2;
   boost::interprocess::ipcdetail::thread_launch(tm2, thread_adapter<M>(&lock_and_sleep, &d2, mtx));

   //Wait completion
   boost::interprocess::ipcdetail::thread_join(tm1);
   boost::interprocess::ipcdetail::thread_join(tm2);

   BOOST_INTERPROCESS_CHECK(d1.m_value == 1);
   BOOST_INTERPROCESS_CHECK(d2.m_value == 2);
}

template<typename M>
void test_mutex_lock_timeout()
{
   shared_val = 0;

   M mtx;

   unsigned wait_time_ms = BOOST_INTERPROCESS_TIMEOUT_WHEN_LOCKING_DURATION_MS;

   //tm1 keeps the lock until tm2 is done, so that tm2 provably tries to lock
   //an owned mutex and the library timeout is what makes it fail
   data<M> d1(1, 0, 0, true);
   data<M> d2(2, (int)wait_time_ms * 1);

   // Locker one launches, and holds the lock until released.
   boost::interprocess::ipcdetail::OS_thread_t tm1;
   boost::interprocess::ipcdetail::thread_launch(tm1, thread_adapter<M>(&lock_and_sleep, &d1, mtx));

   //Wait until tm1 really owns the lock
   BOOST_INTERPROCESS_CHECK(d1.m_acquired.wait());

   // Locker two launches, and should fail to lock after the library timeout.
   boost::interprocess::ipcdetail::OS_thread_t tm2;
   boost::interprocess::ipcdetail::thread_launch(tm2, thread_adapter<M>(&lock_and_catch_errors, &d2, mtx));

   //Wait completion. Only once tm2 is done the lock can be released
   boost::interprocess::ipcdetail::thread_join(tm2);
   d1.m_release.signal();
   boost::interprocess::ipcdetail::thread_join(tm1);

   BOOST_INTERPROCESS_CHECK(d1.m_value == 1);
   BOOST_INTERPROCESS_CHECK(d2.m_value == -1);
   BOOST_INTERPROCESS_CHECK(d1.m_error == no_error);
   BOOST_INTERPROCESS_CHECK(d2.m_error == boost::interprocess::timeout_when_locking_error);
   //The error must come from waiting for the timeout, not from giving up at
   //once, which the error code alone can't tell apart
   BOOST_INTERPROCESS_CHECK(waited_at_least(d2.m_elapsed_us, wait_time_ms));
}

template<typename M>
void test_mutex_try_lock()
{
   shared_val = 0;

   M mtx;

   //tm1 keeps the lock until tm2 is done, so that tm2 provably tries to lock
   //an owned mutex instead of relying on tm1 still sleeping by then
   data<M> d1(1, 0, 0, true);
   data<M> d2(2);

   // Locker one launches, holds the lock until released.
   boost::interprocess::ipcdetail::OS_thread_t tm1;
   boost::interprocess::ipcdetail::thread_launch(tm1, thread_adapter<M>(&try_lock_and_sleep, &d1, mtx));

   //Wait until tm1 really owns the lock
   BOOST_INTERPROCESS_CHECK(d1.m_acquired.wait());

   // Locker two launches, but it should fail acquiring the lock
   boost::interprocess::ipcdetail::OS_thread_t tm2;
   boost::interprocess::ipcdetail::thread_launch(tm2, thread_adapter<M>(&try_lock_and_sleep, &d2, mtx));

   //Wait completion. Only once tm2 is done the lock can be released
   boost::interprocess::ipcdetail::thread_join(tm2);
   d1.m_release.signal();
   boost::interprocess::ipcdetail::thread_join(tm1);

   //Only the first should succeed locking
   BOOST_INTERPROCESS_CHECK(d1.m_value == 1);
   BOOST_INTERPROCESS_CHECK(d2.m_value == -1);
}

template<typename M>
void test_mutex_timed_lock()
{
   for (int flag = 0; flag != (int)ETimedLockFlagsMax; ++flag)
   {
      //int flag = 2;
      shared_val = 0;

      M mtx, m2;

      //tm1 keeps the lock for 2*BaseMs once acquired. Both lockers must succeed
      //here, so tm2 gets a timeout with a wide margin over that hold time: a
      //loaded machine must not be able to make the timeout expire
      data<M> d1(1, 2*BaseMs, flag);
      data<M> d2(2, 8*BaseMs, flag);

      // Locker one launches, holds the lock for 2*BaseMs seconds.
      boost::interprocess::ipcdetail::OS_thread_t tm1;
      boost::interprocess::ipcdetail::thread_launch(tm1, thread_adapter<M>(&timed_lock_and_sleep, &d1, mtx));

      //Wait until tm1 really owns the lock, so that the order in which both
      //threads take it is guaranteed no matter how loaded the machine is
      BOOST_INTERPROCESS_CHECK(d1.m_acquired.wait());

      // Locker two launches, holds the lock for 2*BaseMs seconds.
      boost::interprocess::ipcdetail::OS_thread_t tm2;
      boost::interprocess::ipcdetail::thread_launch(tm2, thread_adapter<M>(&timed_lock_and_sleep, &d2, mtx));

      //Wait completion
      boost::interprocess::ipcdetail::thread_join(tm1);
      boost::interprocess::ipcdetail::thread_join(tm2);

      //Both should succeed locking
      BOOST_INTERPROCESS_CHECK(d1.m_value == 1);
      BOOST_INTERPROCESS_CHECK(d2.m_value == 2);
   }
}

template <typename M>
inline void test_all_lock()
{
   //Now generic interprocess_mutex tests
   std::cout << "test_lock<" << typeid(M).name() << ">" << std::endl;
   test_lock<M>()();
   std::cout << "test_trylock<" << typeid(M).name() << ">" << std::endl;
   test_trylock<M>()();
   std::cout << "test_timedlock<" << typeid(M).name() << ">" << std::endl;
   test_timedlock<M>()();
}

template <typename M>
inline void test_all_recursive_lock()
{
   //Now generic interprocess_mutex tests
   std::cout << "test_recursive_lock<" << typeid(M).name() << ">" << std::endl;
   test_recursive_lock<M>()();
}

template<typename M>
void test_all_mutex()
{
   std::cout << "test_mutex_lock<" << typeid(M).name() << ">" << std::endl;
   test_mutex_lock<M>();
   std::cout << "test_mutex_try_lock<" << typeid(M).name() << ">" << std::endl;
   test_mutex_try_lock<M>();
   std::cout << "test_mutex_timed_lock<" << typeid(M).name() << ">" << std::endl;
   test_mutex_timed_lock<M>();
}

}}}   //namespace boost { namespace interprocess { namespace test {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_TEST_MUTEX_TEST_TEMPLATE_HEADER
