//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_SPIN_SEMAPHORE_HPP
#define BOOST_INTERPROCESS_DETAIL_SPIN_SEMAPHORE_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/interprocess/detail/os_thread_functions.hpp>
#include <boost/interprocess/sync/detail/common_algorithms.hpp>
#include <boost/interprocess/sync/detail/locks.hpp>
#include <boost/cstdint.hpp>

namespace boost {
namespace interprocess {
namespace ipcdetail {

class spin_semaphore
{
   spin_semaphore(const spin_semaphore &);
   spin_semaphore &operator=(const spin_semaphore &);

   public:
   spin_semaphore(unsigned int initialCount);
   ~spin_semaphore();

   void post();
   void wait();
   bool try_wait();
   template<class TimePoint> bool timed_wait(const TimePoint &abs_time);

//   int get_count() const;
   private:
   volatile boost::uint32_t m_count;
};


inline spin_semaphore::~spin_semaphore()
{}

inline spin_semaphore::spin_semaphore(unsigned int initialCount)
{
   //A release is enough to publish the initial count to whoever finds this
   //semaphore afterwards, nothing has to be ordered after it
   ipcdetail::atomic_write32_release(&this->m_count, boost::uint32_t(initialCount));
}

inline void spin_semaphore::post()
{
   //Posting hands work over to whoever waits, so everything done before must
   //be visible to the thread that takes the count. Nothing has to be ordered
   //after it, so a release is enough
   ipcdetail::atomic_inc32_release(&m_count);
}

inline void spin_semaphore::wait()
{
   ipcdetail::lock_to_wait<spin_semaphore> lw(*this);
   return ipcdetail::try_based_lock(lw);
}

inline bool spin_semaphore::try_wait()
{
   //Take one count unless there is none left. The initial read only looks for
   //a candidate value, the swap is what takes the count, so the weakest
   //available order is enough for it
   boost::uint32_t count = ipcdetail::atomic_read32_acquire(&m_count);
   while(count != 0u){
      //Taking a count is an acquire: it consumes what the thread that posted
      //released. A failed swap takes nothing and only leads to another round,
      //so nothing has to be ordered in that case
      const boost::uint32_t prev =
         ipcdetail::atomic_cas32_acquire(&m_count, count - 1u, count);
      if(prev == count){
         return true;
      }
      count = prev;
   }
   return false;
}

template<class TimePoint>
inline bool spin_semaphore::timed_wait(const TimePoint &abs_time)
{
   ipcdetail::lock_to_wait<spin_semaphore> lw(*this);
   return ipcdetail::try_based_timed_lock(lw, abs_time);
}

//inline int spin_semaphore::get_count() const
//{
   //return (int)ipcdetail::atomic_read32(&m_count);
//}

}  //namespace ipcdetail {
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_DETAIL_SPIN_SEMAPHORE_HPP
