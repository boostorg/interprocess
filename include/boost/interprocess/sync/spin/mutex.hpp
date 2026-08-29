//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_SPIN_MUTEX_HPP
#define BOOST_INTERPROCESS_DETAIL_SPIN_MUTEX_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/assert.hpp>
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/cstdint.hpp>
#include <boost/interprocess/detail/os_thread_functions.hpp>
#include <boost/interprocess/sync/detail/common_algorithms.hpp>
#include <boost/interprocess/timed_utils.hpp>

namespace boost {
namespace interprocess {
namespace ipcdetail {

class spin_mutex
{
   spin_mutex(const spin_mutex &);
   spin_mutex &operator=(const spin_mutex &);
   public:

   spin_mutex();
   ~spin_mutex();

   void lock();
   bool try_lock();
   bool maybe_lockable();
   template<class TimePoint>
   bool timed_lock(const TimePoint &abs_time);

   template<class TimePoint> bool try_lock_until(const TimePoint &abs_time)
   {  return this->timed_lock(abs_time);  }

   template<class Duration>  bool try_lock_for(const Duration &dur)
   {  return this->timed_lock(duration_to_ustime(dur)); }

   void unlock();
   void take_ownership(){}
   private:
   volatile boost::uint32_t m_s;

   struct common_lock_wrapper
   {
      common_lock_wrapper(spin_mutex &sp)
         : m_sp(sp)
      {}

      void lock()
      {
         ipcdetail::try_based_lock(m_sp);
      }

      template<class TimePoint>
      bool timed_lock(const TimePoint &abs_time)
      {  return m_sp.timed_lock(abs_time);   }

      spin_mutex &m_sp;
   };
};

BOOST_INTERPROCESS_FORCEINLINE spin_mutex::spin_mutex()
   : m_s(0)
{
   //Note that this class is initialized to zero.
   //So zeroed memory can be interpreted as an
   //initialized mutex
}

BOOST_INTERPROCESS_FORCEINLINE spin_mutex::~spin_mutex()
{
   //Trivial destructor
}

inline void spin_mutex::lock(void)
{
   common_lock_wrapper clw(*this);
   ipcdetail::timeout_when_locking_aware_lock(clw);
}

//A plain load, which only needs the cache line shared, where try_lock() needs
//it exclusive. Used by the spin loops in common_algorithms.hpp to avoid
//hammering the line while the mutex is held.
BOOST_INTERPROCESS_FORCEINLINE bool spin_mutex::maybe_lockable(void)
{  return ipcdetail::atomic_read32(const_cast<boost::uint32_t*>(&m_s)) == 0u;  }

BOOST_INTERPROCESS_FORCEINLINE bool spin_mutex::try_lock(void)
{
   //Taking the mutex must be an acquire, so that everything the previous owner
   //did before releasing it is visible here. Nothing has to be ordered when the
   //mutex is already taken and the swap fails
   boost::uint32_t prev_s = ipcdetail::atomic_xchg32_acquire(const_cast<boost::uint32_t*>(&m_s), 1);
   //A zero previous value means this thread performed the swap and owns the
   //mutex. Re-reading m_s would add nothing: no other thread can change it
   //while it is owned here
   return prev_s == 0;
}

template<class TimePoint>
BOOST_INTERPROCESS_FORCEINLINE bool spin_mutex::timed_lock(const TimePoint &abs_time)
{  return ipcdetail::try_based_timed_lock(*this, abs_time); }

BOOST_INTERPROCESS_FORCEINLINE void spin_mutex::unlock(void)
{
   //Only the owner unlocks, and no other thread can change m_s while it is
   //owned, so nothing has to be compared: a store is enough. It must be a
   //release, so that everything done inside the critical section is visible to
   //the next thread that takes the mutex
   ipcdetail::atomic_write32_release(const_cast<boost::uint32_t*>(&m_s), 0);
}

}  //namespace ipcdetail {
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_DETAIL_SPIN_MUTEX_HPP
