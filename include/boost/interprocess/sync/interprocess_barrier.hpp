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
// barrier is a modified version of Boost Threads barrier
//
//////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2002-2003
// David Moore, William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.

#ifndef BOOST_INTERPROCESS_BARRIER_HPP
#define BOOST_INTERPROCESS_BARRIER_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#if defined BOOST_INTERPROCESS_USE_PTHREAD_BARRIER
#  include <pthread.h>
#  include <errno.h>   
#  include <boost/interprocess/sync/posix/pthread_helpers.hpp>   
#else    //#if defined BOOST_INTERPROCESS_USE_PTHREAD_BARRIER
#  include <boost/interprocess/sync/interprocess_mutex.hpp>
#  include <boost/interprocess/sync/scoped_lock.hpp>
#  include <boost/interprocess/sync/interprocess_condition.hpp>
#  include <stdexcept>
#endif   //#if defined BOOST_INTERPROCESS_USE_PTHREAD_BARRIER

#  include <boost/interprocess/exceptions.hpp>

namespace boost {

namespace interprocess {

/*!An object of class barrier is a synchronization primitive that 
   can be placed in shared memory used to cause a set of threads from 
   different processes to wait until they each perform a certain 
   function or each reach a particular point in their execution.*/
class barrier
{
   public:
   /*!Constructs a barrier object that will cause count threads 
      to block on a call to wait(). */
   barrier(unsigned int count);

   /*!Destroys *this. If threads are still executing their wait() 
      operations, the behavior for these threads is undefined.*/
   ~barrier();

   /*!Effects: Wait until N threads call wait(), where N equals the count 
      provided to the constructor for the barrier object.
      Note that if the barrier is destroyed before wait() can return, 
      the behavior is undefined.
      Returns: Exactly one of the N threads will receive a return value 
      of true, the others will receive a value of false. Precisely which 
      thread receives the return value of true will be implementation-defined. 
      Applications can use this value to designate one thread as a leader that 
      will take a certain action, and the other threads emerging from the barrier 
      can wait for that action to take place.*/
   bool wait();

   private:
   #if defined BOOST_INTERPROCESS_USE_PTHREAD_BARRIER
      pthread_barrier_t    m_barrier;
   #else //#if defined BOOST_INTERPROCESS_USE_PTHREAD_BARRIER
      interprocess_mutex m_mutex;
      interprocess_condition m_cond;
      unsigned int m_threshold;
      unsigned int m_count;
      unsigned int m_generation;
   #endif//#if defined BOOST_INTERPROCESS_USE_PTHREAD_BARRIER
};

#if defined BOOST_INTERPROCESS_USE_PTHREAD_BARRIER

inline barrier::barrier(unsigned int count)
{
   if (count == 0)
      throw std::invalid_argument("count cannot be zero.");
   detail::barrierattr_wrapper barrier_attr;
   detail::barrier_initializer barrier
      (m_barrier, barrier_attr, static_cast<int>(count));
   barrier.release();
}

inline barrier::~barrier()
{
   int res = pthread_barrier_destroy(&m_barrier);
   assert(res  == 0);(void)res;
}

inline bool barrier::wait()
{
   int res = pthread_barrier_wait(&m_barrier);

   if (res != PTHREAD_BARRIER_SERIAL_THREAD || res != 0)
      throw interprocess_exception(system_error_code());
   return res == PTHREAD_BARRIER_SERIAL_THREAD;
}

#else //#if defined BOOST_INTERPROCESS_USE_PTHREAD_BARRIER

inline barrier::barrier(unsigned int count)
   : m_threshold(count), m_count(count), m_generation(0)
{
   if (count == 0)
      throw std::invalid_argument("count cannot be zero.");
}

inline barrier::~barrier(){}

inline bool barrier::wait()
{
   scoped_lock<interprocess_mutex> lock(m_mutex);
   unsigned int gen = m_generation;

   if (--m_count == 0){
      m_generation++;
      m_count = m_threshold;
      m_cond.notify_all();
      return true;
   }

   while (gen == m_generation){
      m_cond.wait(lock);
   }
   return false;
}

#endif   //#if defined BOOST_INTERPROCESS_USE_PTHREAD_BARRIER

}  // namespace interprocess

}  // namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif
