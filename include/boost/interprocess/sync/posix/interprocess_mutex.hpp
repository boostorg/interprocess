//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2007. Distributed under the Boost
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

#include <boost/interprocess/sync/posix/ptime_to_timespec.hpp>
#include <boost/interprocess/detail/posix_time_types_wrk.hpp>
#include <boost/interprocess/exceptions.hpp>

namespace boost {

namespace interprocess {

#ifdef BOOST_INTERPROCESS_POSIX_TIMEOUTS

inline interprocess_mutex::interprocess_mutex()
{
   detail::mutexattr_wrapper mut_attr;
   detail::mutex_initializer mut(m_mut, mut_attr);
   mut.release();
}

inline interprocess_mutex::~interprocess_mutex() 
{
   int res = pthread_mutex_destroy(&m_mut);
   assert(res  == 0);(void)res;
}

inline void interprocess_mutex::lock()
{
   if (pthread_mutex_lock(&m_mut) != 0) 
      throw lock_exception();
}

inline bool interprocess_mutex::try_lock()
{
   int res = pthread_mutex_trylock(&m_mut);
   if (res == EDEADLK) throw lock_exception();
   assert(res == 0 || res == EBUSY);
   return res == 0;
}

inline bool interprocess_mutex::timed_lock(const boost::posix_time::ptime &abs_time)
{
   timespec ts = detail::ptime_to_timespec(abs_time);
   int res = pthread_mutex_timedlock(&m_mut, &ts);
   if (res == EDEADLK || (res != 0 && res != ETIMEDOUT))
      throw lock_exception();
   return res == 0;
}

inline void interprocess_mutex::unlock()
{
   int res = 0;
   res = pthread_mutex_unlock(&m_mut);
//   if (res == EPERM) throw lock_exception();
   assert(res == 0);
}

#else //#ifdef BOOST_INTERPROCESS_POSIX_TIMEOUTS

inline interprocess_mutex::interprocess_mutex()
    : m_locked(false)
{ 
   //Mutex init
   detail::mutexattr_wrapper mut_attr;
   detail::mutex_initializer mut(m_mut, mut_attr);

   //Condition init
   detail::condattr_wrapper  cond_attr;
   detail::condition_initializer cond(m_cond, cond_attr);

   mut.release();
   cond.release();
}

inline interprocess_mutex::~interprocess_mutex()
{
    assert(!m_locked);
    int res = 0;
    res = pthread_mutex_destroy(&m_mut);
    assert(res == 0);

    res = pthread_cond_destroy(&m_cond);
    assert(res == 0);
}

inline void interprocess_mutex::lock()
{
    int res = 0;
    res = pthread_mutex_lock(&m_mut);
    assert(res == 0);

    while (m_locked){
        res = pthread_cond_wait(&m_cond, &m_mut);
        assert(res == 0);
    }

    assert(!m_locked);
    m_locked = true;

    res = pthread_mutex_unlock(&m_mut);
    assert(res == 0);
}

inline bool interprocess_mutex::try_lock()
{
    int res = 0;
    res = pthread_mutex_lock(&m_mut);
    assert(res == 0);

    bool ret = false;
    if (!m_locked)
    {
        m_locked = true;
        ret = true;
    }

    res = pthread_mutex_unlock(&m_mut);
    assert(res == 0);
    return ret;
}

inline bool interprocess_mutex::timed_lock(const boost::posix_time::ptime &abs_time)
{
    int res = 0;
    res = pthread_mutex_lock(&m_mut);
    assert(res == 0);

    timespec ts = detail::ptime_to_timespec(abs_time);

    while (m_locked)
    {
        res = pthread_cond_timedwait(&m_cond, &m_mut, &ts);
        assert(res == 0 || res == ETIMEDOUT);

        if (res == ETIMEDOUT)
            break;
    }

    bool ret = false;
    if (!m_locked)
    {
        m_locked = true;
        ret = true;
    }

    res = pthread_mutex_unlock(&m_mut);
    assert(res == 0);
    return ret;
}

inline void interprocess_mutex::unlock()
{
    int res = 0;
    res = pthread_mutex_lock(&m_mut);
    assert(res == 0);

    assert(m_locked);
    m_locked = false;

    res = pthread_cond_signal(&m_cond);
    assert(res == 0);

    res = pthread_mutex_unlock(&m_mut);
    assert(res == 0);
}

#endif   //#ifdef BOOST_INTERPROCESS_POSIX_TIMEOUTS

}  //namespace interprocess {

}  //namespace boost {
