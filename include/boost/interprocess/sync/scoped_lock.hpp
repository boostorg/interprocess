//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_SCOPED_LOCK_HPP
#define BOOST_INTERPROCESS_SCOPED_LOCK_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/sync/lock_options.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/utility.hpp>
#include <boost/date_time/posix_time/ptime.hpp>

/*!\file
   Describes the scoped_lock and scoped_try_lock classes needed to use shared 
   interprocess_mutex familiy synchronization objects
*/

namespace boost {
namespace interprocess {

template <class Mutex>
class scoped_lock
{
   private:
   typedef scoped_lock<Mutex> this_type;
   scoped_lock(scoped_lock const&);
   scoped_lock& operator=  (scoped_lock const&);
   typedef bool this_type::*unspecified_bool_type;

   public:
   typedef Mutex mutex_type;

   explicit scoped_lock(mutex_type& m)
      : mp_mutex(&m), m_locked(false)
   {  mp_mutex->lock();   m_locked = true;  }

   scoped_lock(mutex_type& m, detail::dont_lock_t)
      : mp_mutex(&m), m_locked(false)
   {}

   scoped_lock(mutex_type& m, detail::try_to_lock_t)
      : mp_mutex(&m), m_locked(false)
   {  m_locked = mp_mutex->try_lock();   }

   scoped_lock(mutex_type& m, const boost::posix_time::ptime& abs_time)
      : mp_mutex(&m), m_locked(false)
   {  m_locked = mp_mutex->timed_lock(abs_time);  }

   ~scoped_lock()
   {
      try{  if(m_locked && mp_mutex)   mp_mutex->unlock();  }
      catch(...){}
   }

   void lock()
   {  
      if(!mp_mutex || m_locked)
         throw lock_exception();
      mp_mutex->lock();
      m_locked = true;
   }

   bool try_lock()
   {  
      if(!mp_mutex || m_locked)
         throw lock_exception();
      m_locked = mp_mutex->try_lock();
      return m_locked;
   }

   bool timed_lock(const boost::posix_time::ptime& abs_time)
   {  
      if(!mp_mutex || m_locked)
         throw lock_exception();
      m_locked = mp_mutex->timed_lock(abs_time);
      return m_locked;
   }

   void unlock()
   {
      if(!mp_mutex || !m_locked)
         throw lock_exception();
      mp_mutex->unlock();
      m_locked = false;
   }

   bool locked() const
   {  return m_locked;  }

   operator unspecified_bool_type() const
   {  return m_locked? &this_type::m_locked : 0;   }

   mutex_type* interprocess_mutex() const
   {  return  mp_mutex;  }

   mutex_type* release()
   {
      mutex_type *mut = mp_mutex;
      mp_mutex = 0;
      return mut;   
   }
 
   private:
   mutex_type *mp_mutex; 
   bool        m_locked;
};

} // namespace interprocess
} // namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif // BOOST_INTERPROCESS_SCOPED_LOCK_HPP
