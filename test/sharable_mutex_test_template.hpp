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
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_TEST_SHARABLE_MUTEX_TEST_TEMPLATE_HEADER
#define BOOST_INTERPROCESS_TEST_SHARABLE_MUTEX_TEST_TEMPLATE_HEADER

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/detail/os_thread_functions.hpp>
#include "boost_interprocess_check.hpp"
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <iostream>
#include <cassert>
#include "util.hpp"
#include <typeinfo>

namespace boost { namespace interprocess { namespace test {

//Once the lock is owned, announce it so that the launcher does not need to
//guess it with a sleep, and either keep it until the test releases it
//(m_block) or for the requested amount of time
template<typename SM>
void hold_lock(data<SM> *pdata, unsigned msecs)
{
   pdata->m_acquired.signal();
   if(pdata->m_block){
      BOOST_INTERPROCESS_CHECK(pdata->m_release.wait());
   }
   else if(msecs){
      boost::interprocess::ipcdetail::thread_sleep_ms(msecs);
   }
}

template<typename SM>
void plain_exclusive(void *arg, SM &sm)
{
   data<SM> *pdata = static_cast<data<SM>*>(arg);
   pdata->m_locking.signal();
   boost::interprocess::scoped_lock<SM> l(sm);
   hold_lock(pdata, unsigned(3*BaseMs));
   shared_val += 10;
   pdata->m_value = shared_val;
}

template<typename SM>
void plain_shared(void *arg, SM &sm)
{
   data<SM> *pdata = static_cast<data<SM>*>(arg);
   boost::interprocess::sharable_lock<SM> l(sm);
   hold_lock(pdata, unsigned(pdata->m_msecs));
   pdata->m_value = shared_val;
}

template<typename SM>
void try_exclusive(void *arg, SM &sm)
{
   data<SM> *pdata = static_cast<data<SM>*>(arg);
   boost::interprocess::scoped_lock<SM> l(sm, boost::interprocess::defer_lock);
   if (l.try_lock()){
      hold_lock(pdata, unsigned(3*BaseMs));
      shared_val += 10;
      pdata->m_value = shared_val;
   }
}

template<typename SM>
void try_shared(void *arg, SM &sm)
{
   data<SM> *pdata = static_cast<data<SM>*>(arg);
   boost::interprocess::sharable_lock<SM> l(sm, boost::interprocess::defer_lock);
   if (l.try_lock()){
      hold_lock(pdata, unsigned(pdata->m_msecs));
      pdata->m_value = shared_val;
   }
}

template<typename SM>
void test_plain_sharable_mutex()
{
   {
      shared_val = 0;
      SM mtx;
      data<SM> e1(1);
      data<SM> e2(2);
      data<SM> s1(1);
      data<SM> s2(2);

      // Writer one launches, holds the lock for 3*BaseMs seconds.
      boost::interprocess::ipcdetail::OS_thread_t tw1;
      boost::interprocess::ipcdetail::thread_launch(tw1, thread_adapter<SM>(plain_exclusive, &e1, mtx));

      //Wait until e1 really owns the mutex, so that it is guaranteed to be
      //the first writer no matter how loaded the machine is
      BOOST_INTERPROCESS_CHECK(e1.m_acquired.wait());

      // Writer two launches and tries to grab the lock, which writer one
      //  is already holding.
      boost::interprocess::ipcdetail::OS_thread_t tw2;
      boost::interprocess::ipcdetail::thread_launch(tw2, thread_adapter<SM>(plain_exclusive, &e2, mtx));

      // Wait until writer two is about to lock, so that the readers below
      //  arrive with a writer already queued. This only shapes the scenario,
      //  no check depends on it: e2 is the only other writer, so it ends up
      //  with the value 20 whatever the order turns out to be.
      BOOST_INTERPROCESS_CHECK(e2.m_locking.wait());

      // Readers launch, after writer two, and while writer 1 still holds
      //   the lock
      boost::interprocess::ipcdetail::OS_thread_t thr1;
      boost::interprocess::ipcdetail::thread_launch(thr1, thread_adapter<SM>(plain_shared,&s1, mtx));
      boost::interprocess::ipcdetail::OS_thread_t thr2;
      boost::interprocess::ipcdetail::thread_launch(thr2, thread_adapter<SM>(plain_shared,&s2, mtx));

      boost::interprocess::ipcdetail::thread_join(thr2);
      boost::interprocess::ipcdetail::thread_join(thr1);
      boost::interprocess::ipcdetail::thread_join(tw2);
      boost::interprocess::ipcdetail::thread_join(tw1);

      //We can only assure that the writer will be first
      BOOST_INTERPROCESS_CHECK(e1.m_value == 10);
      //A that we will execute all
      BOOST_INTERPROCESS_CHECK(s1.m_value == 20 || s2.m_value == 20 || e2.m_value == 20);
   }

   {
      shared_val = 0;
      SM mtx;

      data<SM> s1(1, 3);
      data<SM> s2(2, 3);
      data<SM> e1(1);
      data<SM> e2(2);

      //We launch 2 readers, that will block for 3*BaseTime seconds
      boost::interprocess::ipcdetail::OS_thread_t thr1;
      boost::interprocess::ipcdetail::thread_launch(thr1, thread_adapter<SM>(plain_shared,&s1, mtx));
      boost::interprocess::ipcdetail::OS_thread_t thr2;
      boost::interprocess::ipcdetail::thread_launch(thr2, thread_adapter<SM>(plain_shared,&s2, mtx));

      //Wait until both readers really own the sharable lock, so that they are
      //guaranteed to read the value before any writer changes it
      BOOST_INTERPROCESS_CHECK(s1.m_acquired.wait());
      BOOST_INTERPROCESS_CHECK(s2.m_acquired.wait());

      // We launch two writers, that should block until the readers end
      boost::interprocess::ipcdetail::OS_thread_t tw1;
      boost::interprocess::ipcdetail::thread_launch(tw1, thread_adapter<SM>(plain_exclusive,&e1, mtx));

      boost::interprocess::ipcdetail::OS_thread_t tw2;
      boost::interprocess::ipcdetail::thread_launch(tw2, thread_adapter<SM>(plain_exclusive,&e2, mtx));

      boost::interprocess::ipcdetail::thread_join(tw2);
      boost::interprocess::ipcdetail::thread_join(tw1);
      boost::interprocess::ipcdetail::thread_join(thr2);
      boost::interprocess::ipcdetail::thread_join(thr1);

      //We can only assure that the shared will finish first...
      BOOST_INTERPROCESS_CHECK(s1.m_value == 0 || s2.m_value == 0);
      //...and writers will be mutually excluded after readers
      BOOST_INTERPROCESS_CHECK((e1.m_value == 10 && e2.m_value == 20) ||
             (e1.m_value == 20 && e2.m_value == 10) );
   }
}

template<typename SM>
void test_try_sharable_mutex()
{
   SM mtx;

   data<SM> s1(1);
   //e1 keeps the lock until the others are done, so that they provably try to
   //lock an owned mutex instead of relying on e1 still sleeping by then
   data<SM> e1(2, 0, 0, true);
   data<SM> e2(3);

   // We start with some specialized tests for "try" behavior
   shared_val = 0;

   // Writer one launches, holds the lock until released.
   boost::interprocess::ipcdetail::OS_thread_t tw1;
   boost::interprocess::ipcdetail::thread_launch(tw1, thread_adapter<SM>(try_exclusive,&e1,mtx));

   //Wait until e1 really owns the mutex
   BOOST_INTERPROCESS_CHECK(e1.m_acquired.wait());

   // Reader one launches, after writer #1 holds the lock
   //   and before it releases the lock.
   boost::interprocess::ipcdetail::OS_thread_t thr1;
   boost::interprocess::ipcdetail::thread_launch(thr1, thread_adapter<SM>(try_shared,&s1,mtx));

   // Writer two launches in the same timeframe.
   boost::interprocess::ipcdetail::OS_thread_t tw2;
   boost::interprocess::ipcdetail::thread_launch(tw2, thread_adapter<SM>(try_exclusive,&e2,mtx));

   //Only once both are done the lock can be released
   boost::interprocess::ipcdetail::thread_join(tw2);
   boost::interprocess::ipcdetail::thread_join(thr1);
   e1.m_release.signal();
   boost::interprocess::ipcdetail::thread_join(tw1);

   BOOST_INTERPROCESS_CHECK(e1.m_value == 10);
   BOOST_INTERPROCESS_CHECK(s1.m_value == -1);        // Try would return w/o waiting
   BOOST_INTERPROCESS_CHECK(e2.m_value == -1);        // Try would return w/o waiting
}

template<typename SM>
void timed_exclusive(void *arg, SM &sm)
{
   data<SM> *pdata = static_cast<data<SM>*>(arg);
   boost::interprocess::scoped_lock<SM>
      l (sm, boost::interprocess::defer_lock);

   bool r = false;
   const boost::uint64_t start_us = elapsed_now_us();
   if(pdata->m_flags == (int)TimedLock){
      r = l.timed_lock(std_systemclock_delay_ms(unsigned(pdata->m_msecs)));
   }
   else if (pdata->m_flags == (int)TryLockUntil) {
      r = l.try_lock_until(ptime_delay_ms(unsigned(pdata->m_msecs)));
   }
   else if (pdata->m_flags == (int)TryLockFor) {
      r = l.try_lock_for(boost_systemclock_ms(unsigned(pdata->m_msecs)));
   }
   //Record how long the attempt took, so that a test expecting it to fail can
   //tell the timeout apart from an immediate failure
   pdata->m_elapsed_us = elapsed_now_us() - start_us;

   if (r){
      hold_lock(pdata, unsigned(3*BaseMs));
      shared_val += 10;
      pdata->m_value = shared_val;
   }
}

template<typename SM>
void timed_shared(void *arg, SM &sm)
{
   data<SM> *pdata = static_cast<data<SM>*>(arg);
   boost::interprocess::sharable_lock<SM>
      l(sm, boost::interprocess::defer_lock);

   bool r = false;
   const boost::uint64_t start_us = elapsed_now_us();
   if(pdata->m_flags == (int)TimedLock){
      r = l.timed_lock(std_systemclock_delay_ms(unsigned(pdata->m_msecs)));
   }
   else if (pdata->m_flags == (int)TryLockUntil) {
      r = l.try_lock_until(ptime_delay_ms(unsigned(pdata->m_msecs)));
   }
   else if (pdata->m_flags == (int)TryLockFor) {
      r = l.try_lock_for(boost_systemclock_ms(unsigned(pdata->m_msecs)));
   }
   //Record how long the attempt took, so that a test expecting it to fail can
   //tell the timeout apart from an immediate failure
   pdata->m_elapsed_us = elapsed_now_us() - start_us;

   if (r){
      hold_lock(pdata, unsigned(3*BaseMs));
      pdata->m_value = shared_val;
   }
}

template<typename SM>
void test_timed_sharable_mutex()
{
   for (int flag = 0; flag != (int)ETimedLockFlagsMax; ++flag)
   {
      SM mtx;
      //e1 keeps the lock until the lockers that must fail are done. Holding it
      //for a fixed time instead is not enough: under heavy CPU load a thread
      //can take longer to start than the hold time, find the mutex already
      //free and succeed, which would defeat the purpose of the test
      data<SM> e1(3, 3*BaseMs, flag, true);
      data<SM> e2(4, 1*BaseMs, flag);
      data<SM> s1(1, 1*BaseMs, flag);
      //s2 is the only one that must succeed. It is launched just before
      //releasing e1, so its timeout only has to cover that short window
      data<SM> s2(2, 9*BaseMs, flag);

      // We begin with some specialized tests for "timed" behavior

      shared_val = 0;

      // Writer one will hold the lock until released.
      boost::interprocess::ipcdetail::OS_thread_t tw1;
      boost::interprocess::ipcdetail::thread_launch(tw1, thread_adapter<SM>(timed_exclusive,&e1,mtx));

      //Wait until e1 really owns the mutex, so that the others are guaranteed
      //to find it locked no matter how loaded the machine is
      BOOST_INTERPROCESS_CHECK(e1.m_acquired.wait());

      // Writer two tries for the lock while writer one holds it, waiting up to
      //  1*BaseMs seconds. This write will fail.
      boost::interprocess::ipcdetail::OS_thread_t tw2;
      boost::interprocess::ipcdetail::thread_launch(tw2, thread_adapter<SM>(timed_exclusive,&e2,mtx));

      // Reader one also tries while writer one holds the lock, waiting up to
      //  1*BaseMs seconds, and will fail to get it.
      boost::interprocess::ipcdetail::OS_thread_t thr1;
      boost::interprocess::ipcdetail::thread_launch(thr1, thread_adapter<SM>(timed_shared,&s1,mtx));

      // Both must have failed before the lock is released
      boost::interprocess::ipcdetail::thread_join(tw2);
      boost::interprocess::ipcdetail::thread_join(thr1);

      // Reader two is launched and the lock released right after, so it gets
      //   the lock and reads the value written by writer one
      boost::interprocess::ipcdetail::OS_thread_t thr2;
      boost::interprocess::ipcdetail::thread_launch(thr2, thread_adapter<SM>(timed_shared,&s2,mtx));
      e1.m_release.signal();

      boost::interprocess::ipcdetail::thread_join(thr2);
      boost::interprocess::ipcdetail::thread_join(tw1);

      BOOST_INTERPROCESS_CHECK(e1.m_value == 10);
      BOOST_INTERPROCESS_CHECK(e2.m_value == -1);
      BOOST_INTERPROCESS_CHECK(s1.m_value == -1);
      BOOST_INTERPROCESS_CHECK(s2.m_value == 10);
      //Both failures must come from waiting for the timeout, and not from
      //giving up at once, which the value alone can't tell apart
      BOOST_INTERPROCESS_CHECK(waited_at_least(e2.m_elapsed_us, unsigned(e2.m_msecs)));
      BOOST_INTERPROCESS_CHECK(waited_at_least(s1.m_elapsed_us, unsigned(s1.m_msecs)));
   }
}

template<typename SM>
void test_all_sharable_mutex()
{
   std::cout << "test_plain_sharable_mutex<" << typeid(SM).name() << ">" << std::endl;
   test_plain_sharable_mutex<SM>();

   std::cout << "test_try_sharable_mutex<" << typeid(SM).name() << ">" << std::endl;
   test_try_sharable_mutex<SM>();

   std::cout << "test_timed_sharable_mutex<" << typeid(SM).name() << ">" << std::endl;
   test_timed_sharable_mutex<SM>();
}


}}}   //namespace boost { namespace interprocess { namespace test {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_TEST_SHARABLE_MUTEX_TEST_TEMPLATE_HEADER
