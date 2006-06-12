//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
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

#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace boost {

namespace interprocess {

inline static void do_lock_func(volatile long &m_s)
{
   using namespace boost::detail;
   do{
      long prev_s = BOOST_INTERLOCKED_COMPARE_EXCHANGE((long*)&m_s, 0, 1);   

      if (m_s == 0 && prev_s == 1){
            break;
      }
      // relinquish current timeslice
      winapi::sched_yield();
   }while (true);
}

inline static bool do_trylock_func(volatile long &m_s)
{
   using namespace boost::detail;
   long prev_s = BOOST_INTERLOCKED_COMPARE_EXCHANGE((long*)&m_s, 0, 1);   

   if (m_s == 0 && prev_s == 1){
      return true;
   }
   return false;
}

inline static bool do_timedlock_func(volatile long &m_s, const boost::posix_time::ptime &abs_time)
{
   //Obtain current count and target time
   boost::posix_time::ptime now = 
      boost::posix_time::microsec_clock::universal_time();
   using namespace boost::detail;

   if(now >= abs_time) return false;

   do{
      long prev_s = BOOST_INTERLOCKED_COMPARE_EXCHANGE((long*)&m_s, 0, 1);   

      if (m_s == 0 && prev_s == 1){
         break;
      }
      now = boost::posix_time::microsec_clock::universal_time();

      if(now >= abs_time){
         return false;
      }
      // relinquish current time slice
      winapi::sched_yield();
   }while (true);

   return true;
}

inline static void do_unlock_func(volatile long &m_s)
{
   using namespace boost::detail;
   BOOST_INTERLOCKED_COMPARE_EXCHANGE((long*)&m_s, 1, 0);
}

inline interprocess_mutex::interprocess_mutex() 
   : m_s(1) 
{}

inline interprocess_mutex::~interprocess_mutex() 
{}

inline void interprocess_mutex::lock(void)
{
   do_lock_func(m_s);
}

inline bool interprocess_mutex::try_lock(void)
{
   return do_trylock_func(m_s);
}

inline bool interprocess_mutex::timed_lock(const boost::posix_time::ptime &abs_time)
{
   return do_timedlock_func(m_s, abs_time);
}

inline void interprocess_mutex::unlock(void)
{
   do_unlock_func(m_s);
}

}  //namespace interprocess {

}  //namespace boost {
