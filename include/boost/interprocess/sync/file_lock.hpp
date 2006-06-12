//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_FILE_LOCK_HPP
#define BOOST_INTERPROCESS_FILE_LOCK_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/noncopyable.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <assert.h>
#include <boost/interprocess/detail/os_file_functions.hpp>

/*!\file
   Describes file_lock
*/

namespace boost {

namespace interprocess {

/*!Wraps a file locking class, to implement mutual exclusion */
class file_lock : private boost::noncopyable
{
   public:

   file_lock(const char *name);

   ~file_lock();

   void lock();

   bool try_lock();

   bool timed_lock(const boost::posix_time::ptime &abs_time);

   void unlock();

   void lock_sharable();

   bool try_lock_sharable();

   bool timed_lock_sharable(const boost::posix_time::ptime &abs_time);

   void unlock_sharable();

   private:
   detail::OS_file_handle_t m_file_hnd;
};

inline file_lock::file_lock(const char *name)
{
   m_file_hnd = detail::open_existing_file(name);

   if(m_file_hnd == detail::invalid_file()){
      error_info err(system_error_code());
      throw interprocess_exception(err);
   }
}

inline file_lock::~file_lock()
{
   if(m_file_hnd != detail::invalid_file()){
      detail::close_file(m_file_hnd);
      m_file_hnd = detail::invalid_file();
   }
}

inline void file_lock::lock()
{
   if(!detail::acquire_file_lock(m_file_hnd)){
      error_info err(system_error_code());
      throw interprocess_exception(err);
   }
}

inline bool file_lock::try_lock()
{
   bool result;
   if(!detail::try_acquire_file_lock(m_file_hnd, result)){
      error_info err(system_error_code());
      throw interprocess_exception(err);
   }
   return result;
}

inline bool file_lock::timed_lock(const boost::posix_time::ptime &abs_time)
{
   bool result;
   if(!detail::timed_acquire_file_lock(m_file_hnd, result, abs_time)){
      error_info err(system_error_code());
      throw interprocess_exception(err);
   }
   return result;
}

inline void file_lock::unlock()
{
   if(!detail::release_file_lock(m_file_hnd)){
      error_info err(system_error_code());
      throw interprocess_exception(err);
   }
}

inline void file_lock::lock_sharable()
{
   if(!detail::acquire_file_lock_sharable(m_file_hnd)){
      error_info err(system_error_code());
      throw interprocess_exception(err);
   }
}

inline bool file_lock::try_lock_sharable()
{
   bool result;
   if(!detail::try_acquire_file_lock_sharable(m_file_hnd, result)){
      error_info err(system_error_code());
      throw interprocess_exception(err);
   }
   return result;
}

inline bool file_lock::timed_lock_sharable(const boost::posix_time::ptime &abs_time)
{
   bool result;
   if(!detail::timed_acquire_file_lock_sharable(m_file_hnd, result, abs_time)){
      error_info err(system_error_code());
      throw interprocess_exception(err);
   }
   return result;
}

inline void file_lock::unlock_sharable()
{
   if(!detail::release_file_lock_sharable(m_file_hnd)){
      error_info err(system_error_code());
      throw interprocess_exception(err);
   }
}

}  //namespace interprocess {

}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_FILE_LOCK_HPP
