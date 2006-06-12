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
// Parts of the pthread code come from Boost Threads code.
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

#ifndef BOOST_INTERPROCESS_MUTEX_HPP
#define BOOST_INTERPROCESS_MUTEX_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/noncopyable.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <assert.h>

#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
#  include <boost/interprocess/sync/win32/win32_sync_primitives.hpp>
#else    //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
#  include <pthread.h>
#  include <errno.h>   
#  include <boost/interprocess/sync/posix/pthread_helpers.hpp>   
#endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

/*!\file
   Describes interprocess_mutex class
*/

namespace boost {

namespace interprocess {

class interprocess_condition;

/*!Wraps a interprocess_mutex that can be placed in shared memory and can be 
   shared between processes. Allows timed lock tries*/
class interprocess_mutex : private boost::noncopyable
{
   friend class interprocess_condition;
   public:

   interprocess_mutex();

   ~interprocess_mutex();

   void lock();

   bool try_lock();

   bool timed_lock(const boost::posix_time::ptime &abs_time);

   void unlock();

   private:

   #if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   volatile long m_s;
   #else    //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   #ifdef _POSIX_TIMEOUTS
      pthread_mutex_t   m_mut;
   #else
      pthread_mutex_t   m_mut;
      pthread_cond_t    m_cond;
      bool              m_locked;
   #endif
   #endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
};

}  //namespace interprocess {

}  //namespace boost {

#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
#  include <boost/interprocess/sync/win32/interprocess_mutex.hpp>
#else 
#  include <boost/interprocess/sync/posix/interprocess_mutex.hpp>
#endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_MUTEX_HPP
