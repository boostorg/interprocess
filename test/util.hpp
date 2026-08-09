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

#ifndef BOOST_INTERPROCESS_TEST_UTIL_HEADER
#define BOOST_INTERPROCESS_TEST_UTIL_HEADER

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/detail/os_thread_functions.hpp>
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/interprocess/sync/spin/wait.hpp>
#include <boost/interprocess/timed_utils.hpp>
#include <boost/cstdint.hpp>

#if defined(BOOST_CLANG) || (defined(BOOST_GCC) && (BOOST_GCC >= 40600))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#  if (BOOST_GCC >= 100000)
#pragma GCC diagnostic ignored "-Warith-conversion"
#  endif
#endif

#if (BOOST_CXX_VERSION >= 201103L) && (!defined(BOOST_GCC) || (BOOST_GCC >= 40800))
//boost.System is not tested under GCC <= 4.8 and has compilation errors
//due to incomplete language support in older compilers
#define BOOST_INTERPROCESS_BOOST_CHRONO_AVAILABLE
#endif

#if (BOOST_CXX_VERSION >= 201103L)
#define BOOST_INTERPROCESS_DATE_TIME_AVAILABLE
#include <boost/date_time/posix_time/posix_time_types.hpp>
#endif

#ifdef BOOST_INTERPROCESS_BOOST_CHRONO_AVAILABLE
#define BOOST_CHRONO_HEADER_ONLY
#include <boost/chrono/system_clocks.hpp>
#endif

#include <boost/version.hpp>

#if !defined(BOOST_NO_CXX11_HDR_CHRONO)
#include <chrono>
#endif

#if defined(BOOST_GCC) && (BOOST_GCC >= 40600)
#pragma GCC diagnostic pop
#endif

namespace boost {
namespace interprocess {
namespace test {

// ptime_delay_ms + ptime_ms

#if defined(BOOST_INTERPROCESS_DATE_TIME_AVAILABLE)
inline boost::posix_time::ptime ptime_delay_ms(unsigned msecs)
{
   using namespace boost::posix_time;
   int count = static_cast<int>(double(msecs) *
      (double(time_duration::ticks_per_second()) / double(1000.0)));
   return microsec_clock::universal_time() + time_duration(0, 0, 0, count);
}

inline boost::posix_time::time_duration ptime_ms(unsigned msecs)
{
   using namespace boost::posix_time;
   int count = static_cast<int>(double(msecs) *
      (double(time_duration::ticks_per_second()) / double(1000.0)));
   return time_duration(0, 0, 0, count);
}
#else
   inline ustime ptime_delay_ms(unsigned msecs)
   {  return ustime_delay_milliseconds(msecs); }

   inline usduration ptime_ms(unsigned msecs)
   {  return usduration_from_milliseconds(msecs); }
#endif

// boost_systemclock_delay_ms + boost_systemclock_ms

#if defined(BOOST_INTERPROCESS_BOOST_CHRONO_AVAILABLE)
   inline boost::chrono::system_clock::time_point boost_systemclock_delay_ms(unsigned msecs)
   {  return boost::chrono::system_clock::now() + boost::chrono::milliseconds(msecs);  }

   inline boost::chrono::milliseconds boost_systemclock_ms(unsigned msecs)
   {  return boost::chrono::milliseconds(msecs);  }
#else
   inline ustime boost_systemclock_delay_ms(unsigned msecs)
   {  return ustime_delay_milliseconds(msecs); }

   inline usduration boost_systemclock_ms(unsigned msecs)
   {  return usduration_from_milliseconds(msecs); }
#endif 

// std_systemclock_delay_ms + std_systemclock_ms

#if !defined(BOOST_NO_CXX11_HDR_CHRONO)
   //Use std chrono if available
   inline std::chrono::system_clock::time_point std_systemclock_delay_ms(unsigned msecs)
   {  return std::chrono::system_clock::now() + std::chrono::milliseconds(msecs);  }

   inline std::chrono::milliseconds std_systemclock_ms(unsigned msecs)
   {  return std::chrono::milliseconds(msecs);  }

#elif defined(BOOST_INTERPROCESS_BOOST_CHRONO_AVAILABLE)
   //Otherwise use boost chrono
   inline boost::chrono::system_clock::time_point std_systemclock_delay_ms(unsigned msecs)
   {  return boost_systemclock_delay_ms(msecs);  }

   inline boost::chrono::milliseconds std_systemclock_ms(unsigned msecs)
   {  return boost_systemclock_ms(msecs);  }
#else
   inline ustime std_systemclock_delay_ms(unsigned msecs)
   {  return ustime_delay_milliseconds(msecs); }

   inline usduration std_systemclock_ms(unsigned msecs)
   {  return usduration_from_milliseconds(msecs); }
#endif

// test_event

//!Upper bound used by test_event::wait() so that a broken test fails instead
//!of hanging forever. It is deliberately much larger than any legitimate wait,
//!as it must never trigger on a heavily loaded but otherwise working machine.
static const unsigned WatchdogMs = 120u*1000u;

//!Number of times a wait taking a predicate is retried before the test gives
//!up. Such a wait returning false only means that its deadline expired before
//!the predicate became true, and on a loaded machine that just means the
//!notifying thread was late: it has to be woken up, scheduled and re-acquire
//!the mutex before it can update the state and notify, and while it does the
//!mutex stays free, so the waiter can time out and see the old state.
//!Each retry rechecks the predicate (the predicate overloads test it on entry
//!and on timeout), so no notification can be missed, and the accumulated
//!budget is large enough that only a real failure to notify exhausts it.
static const unsigned PredicateWaitRetries = 10u;

//!A one-shot event flag used to synchronize test threads.
//!
//!Sleeping for a while to "make sure" that another thread has already reached
//!a given state (e.g. that it owns a mutex) is not a synchronization method:
//!under heavy CPU load (specially on shared cloud CPUs) a thread can be
//!descheduled for an arbitrarily long time, so the assumed ordering can be
//!inverted and the test fails without anything being broken.
//!
//!Threads signal the state they reached and their peers wait for it, which
//!makes the ordering deterministic no matter how loaded the machine is.
//!Both sides live in the same process, so a plain atomic flag is enough.
class test_event
{
   public:
   test_event() : m_signaled(0u) {}

   //!Announces that the awaited state has been reached
   void signal()
   {  ipcdetail::atomic_write32(&m_signaled, 1u);  }

   bool signaled()
   {  return 0u != ipcdetail::atomic_read32(&m_signaled);  }

   //!Waits until the event is signaled. Returns false if the watchdog expires,
   //!so that a broken test fails instead of hanging forever. The watchdog is
   //!not a test deadline: it must stay generous enough to never trigger on a
   //!loaded but working machine.
   bool wait(unsigned timeout_ms = WatchdogMs)
   {
      const ustime deadline = ustime_delay_milliseconds(timeout_ms);
      spin_wait swait;
      while(!this->signaled()){
         if(ustime(ipcdetail::universal_time_u64_us()) > deadline){
            return false;
         }
         swait.yield();
      }
      return true;
   }

   private:
   test_event(const test_event &);
   test_event &operator=(const test_event &);

   volatile boost::uint32_t m_signaled;
};

//!Returns the current time in microseconds, to measure how long an operation
//!really took
inline boost::uint64_t elapsed_now_us()
{  return ipcdetail::universal_time_u64_us();  }

//!Checks that a timed operation that was expected to fail really waited for
//!its timeout instead of failing right away.
//!
//!Only a lower bound can be checked: a loaded machine can make the wait
//!longer, never shorter, so this can't become a source of spurious failures.
//!Checking an upper bound instead would be exactly the kind of assertion that
//!a busy CPU breaks. A small tolerance absorbs the resolution difference
//!between the clock used to build the deadline and the one measuring here.
inline bool waited_at_least(boost::uint64_t elapsed_us, unsigned timeout_ms)
{
   const boost::uint64_t timeout_us   = boost::uint64_t(timeout_ms)*1000u;
   const boost::uint64_t tolerance_us = timeout_us/10u;
   return (elapsed_us + tolerance_us) >= timeout_us;
}

// thread_adapter + data

template <typename P>
class thread_adapter
{
   public:
   thread_adapter(void (*func)(void*, P &), void* param1, P &param2)
      : func_(func), param1_(param1) ,param2_(param2){ }
   void operator()() const { func_(param1_, param2_); }

   private:
   void (*func_)(void*, P &);
   void* param1_;
   P& param2_;
};

template <typename P>
struct data
{
   explicit data(int id, int msecs=0, int flags = 0, bool block = false)
      : m_id(id), m_value(-1), m_msecs(msecs), m_error(no_error), m_flags(flags)
      , m_elapsed_us(0u), m_block(block)
   {}

   int            m_id;
   int            m_value;
   int            m_msecs;
   error_code_t   m_error;
   int            m_flags;
   //!Time the locking operation really took. A test that expects an operation
   //!to fail because of its timeout can only tell that apart from an operation
   //!failing right away by looking at how long it waited.
   boost::uint64_t m_elapsed_us;
   //!When true, the thread keeps the lock until m_release is signaled, instead
   //!of holding it for a fixed amount of time. This lets a test guarantee that
   //!a peer operation really happens while the lock is taken.
   bool           m_block;
   //!Signaled by the thread just before it starts locking. Waiting for it does
   //!not prove the thread is already blocked on the lock, but it does remove
   //!the unbounded part of that wait (creating and scheduling the thread),
   //!which is what a sleep can't cope with on a loaded machine
   test_event     m_locking;
   //!Signaled by the thread once it owns the lock
   test_event     m_acquired;
   //!Signaled by the test to let a m_block thread release the lock
   test_event     m_release;
};

int shared_val = 0;
static const unsigned BaseMs = 1000;

}  //namespace test {
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_TEST_UTIL_HEADER
