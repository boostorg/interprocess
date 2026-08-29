//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2012-2013. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_SYNC_DETAIL_COMMON_ALGORITHMS_HPP
#define BOOST_INTERPROCESS_SYNC_DETAIL_COMMON_ALGORITHMS_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/sync/spin/wait.hpp>
#include <boost/interprocess/timed_utils.hpp>

namespace boost {
namespace interprocess {
namespace ipcdetail {

//////////////////////////////////////////////////////////////////////////////
//
//    Optional cheap probe: MutexType::maybe_lockable()
//
//////////////////////////////////////////////////////////////////////////////
//
// A mutex may offer "bool maybe_lockable()", a cheap, conservative answer to
// "would try_lock() have a chance right now?". It is a hint, subject to three
// rules:
//    - returning true is always allowed: the caller then performs the real
//      try_lock(), which is what actually decides,
//    - it must not keep returning false while the mutex is in fact free, or a
//      spinning caller would never take it. Re-reading the state each call is
//      enough to satisfy this,
//    - and, less obviously, a mutex whose try_lock() has side effects the
//      waiter depends on must NOT implement it. robust_spin_mutex is the
//      example
//
// The point is that for a spin mutex try_lock() is a read-modify-write, and a
// failed one still takes the cache line for writing, so a loop that retries it
// makes every other core's copy of the line invalid on every spin. A plain
// load only needs the line shared, so the spinning threads stop fighting each
// other and the holder can make progress. This is the classic
// test-and-test-and-set, expressed once here instead of in each mutex.
//
// Mutexes without the member are unaffected: the probe then answers true
// unconditionally and the loops behave exactly as before.
//
template<class MutexType>
struct has_maybe_lockable
{
   typedef char one_type;
   struct two_type { char dummy[2]; };

   //Viable only when "m.maybe_lockable()" is a valid expression
   template<class U> static one_type test(char (*)[sizeof(((U*)0)->maybe_lockable(), 1)]);
   template<class U> static two_type test(...);

   static const bool value = sizeof(test<MutexType>(0)) == sizeof(one_type);
};

template<class MutexType, bool HasProbe>
struct maybe_lockable_impl
{
   //No probe available: always let the caller attempt the real try_lock
   BOOST_INTERPROCESS_FORCEINLINE static bool call(MutexType &)
   {  return true;  }
};

template<class MutexType>
struct maybe_lockable_impl<MutexType, true>
{
   BOOST_INTERPROCESS_FORCEINLINE static bool call(MutexType &m)
   {  return m.maybe_lockable();  }
};

template<class MutexType>
inline bool maybe_lockable(MutexType &m)
{
   return maybe_lockable_impl
      <MutexType, has_maybe_lockable<MutexType>::value>::call(m);
}

template<class MutexType, class TimePoint>
bool try_based_timed_lock(MutexType &m, const TimePoint &abs_time)
{
   //Same as lock()
   if(is_pos_infinity(abs_time)){
      m.lock();
      return true;
   }
   //Always try to lock to achieve POSIX guarantees:
   // "Under no circumstance shall the function fail with a timeout if the mutex
   //  can be locked immediately. The validity of the abs_timeout parameter need not
   //  be checked if the mutex can be locked immediately."
   else if(m.try_lock()){
      return true;
   }
   else{
      spin_wait swait;
      while(microsec_clock<TimePoint>::universal_time() < abs_time){
         //Probe before the real attempt, so a mutex that can answer cheaply is
         //not hammered with read-modify-writes while it is held. One yield per
         //iteration either way, so the back-off progression is unchanged
         if(maybe_lockable(m) && m.try_lock()){
            return true;
         }
         swait.yield();
      }
      return false;
   }
}

template<class MutexType>
void try_based_lock(MutexType &m)
{
   //The first attempt is unconditional: it must not be gated by a hint, both
   //because taking a free mutex has to be immediate and because POSIX requires
   //it of the timed variant
   if(!m.try_lock()){
      spin_wait swait;
      do{
         if(maybe_lockable(m) && m.try_lock()){
            break;
         }
         else{
            swait.yield();
         }
      }
      while(1);
   }
}

template<class MutexType>
void timeout_when_locking_aware_lock(MutexType &m)
{
   #ifdef BOOST_INTERPROCESS_ENABLE_TIMEOUT_WHEN_LOCKING
      if (!m.timed_lock(microsec_clock<ustime>::universal_time()
           + usduration_from_milliseconds(BOOST_INTERPROCESS_TIMEOUT_WHEN_LOCKING_DURATION_MS)))
      {
         throw interprocess_exception(timeout_when_locking_error
                                     , "Interprocess mutex timeout when locking. Possible deadlock: "
                                       "owner died without unlocking?");
      }
   #else
      m.lock();
   #endif
}

}  //namespace ipcdetail
}  //namespace interprocess
}  //namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_SYNC_DETAIL_COMMON_ALGORITHMS_HPP
