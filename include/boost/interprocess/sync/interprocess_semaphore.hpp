//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_SEMAPHORE_HPP
#define BOOST_INTERPROCESS_SEMAPHORE_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/noncopyable.hpp>

#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
#  include <boost/interprocess/sync/win32/win32_sync_primitives.hpp>
#  include <boost/interprocess/sync/interprocess_mutex.hpp>
#  include <boost/interprocess/sync/interprocess_condition.hpp>
#else    //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

#include <fcntl.h>      //O_CREAT, O_*... 
#include <unistd.h>     //close
#include <string>       //std::string
#include <semaphore.h>  //sem_* family, SEM_VALUE_MAX
#include <sys/stat.h>   //mode_t, S_IRWXG, S_IRWXO, S_IRWXU,
#include <boost/interprocess/sync/posix/semaphore_wrapper.hpp>   //for shared_memory

#endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

/*!\file
   Describes a interprocess_semaphore class for inter-process synchronization
*/

namespace boost {

namespace interprocess {

/*!Wraps a interprocess_semaphore that can be placed in shared memory and can be 
   shared between processes. Allows timed lock tries*/
class interprocess_semaphore : private boost::noncopyable
{
 public:
   /*!Creates a interprocess_semaphore with the given initial count. 
      interprocess_exception if there is an error.*/
   interprocess_semaphore(int initialCount);

   /*!Destroys the interprocess_semaphore. Does not throw*/
   ~interprocess_semaphore();

   /*!Increments the interprocess_semaphore count. If there are processes/threads blocked waiting
      for the interprocess_semaphore, then one of these processes will return successfully from
      its wait function. If there is an error an interprocess_exception exception is thrown.*/
   void post();

   /*!Decrements the interprocess_semaphore. If the interprocess_semaphore value is not greater than zero,
      then the calling process/thread blocks until it can decrement the counter. 
      If there is an error an interprocess_exception exception is thrown.*/
   void wait();

   /*!Decrements the interprocess_semaphore if the interprocess_semaphore's value is greater than zero
      and returns true. If the value is not greater than zero returns false.
      If there is an error an interprocess_exception exception is thrown.*/
   bool try_wait();

   /*!Decrements the interprocess_semaphore if the interprocess_semaphore's value is greater
      than zero and returns true. Otherwise, waits for the interprocess_semaphore
      to the posted or the timeout expires. If the timeout expires, the
      function returns false. If the interprocess_semaphore is posted the function
      returns true. If there is an error throws sem_exception*/
   bool timed_wait(const boost::posix_time::ptime &abs_time);

   /*!Returns the interprocess_semaphore count*/
//   int get_count() const;

 private:
   #if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   interprocess_mutex       m_mut;
   interprocess_condition   m_cond;
   int         m_count;
   #else    //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   detail::semaphore_wrapper m_sem;
   #endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
};

}  //namespace interprocess {

}  //namespace boost {

#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
#  include <boost/interprocess/sync/win32/interprocess_semaphore.hpp>
#else 
#  include <boost/interprocess/sync/posix/interprocess_semaphore.hpp>
#endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_SEMAPHORE_HPP
