//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_NAMED_MUTEX_HPP
#define BOOST_INTERPROCESS_NAMED_MUTEX_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/noncopyable.hpp>

/*!\file
   Describes a named mutex class for inter-process synchronization
*/

namespace boost {

namespace interprocess {

/*!A interprocess_mutex with a global name, so it can be found from different 
   processes. This interprocess_mutex can't be placed in shared memory, and
   each process should have it's own interprocess_mutex.*/
class named_mutex : private boost::noncopyable
{
 public:
   /*!Creates a global interprocess_mutex with a name.*/
   named_mutex(detail::create_only_t create_only, const char *name);

   /*!Opens or creates a global interprocess_mutex with a name. 
      If the interprocess_mutex is created, this call is equivalent to create(). 
      If the interprocess_mutex is already created, this call is equivalent to open(). 
      Does not throw*/
   named_mutex(detail::open_or_create_t open_or_create, const char *name);

   /*!Opens a global interprocess_mutex with a name if that interprocess_mutex is previously.
      created. If it is not previously created this function return false.
      Does not throw*/
   named_mutex(detail::open_only_t open_only, const char *name);

   /*!Destroys named interprocess_mutex. Does not throw*/
   ~named_mutex();

   /*!Unlocks a previously locked interprocess_mutex.*/
   void unlock();

   /*!Locks interprocess_mutex, sleeps when interprocess_mutex is already locked.
      Throws interprocess_exception if a severe error is found*/
   void lock();

   /*! Tries to lock the interprocess_mutex, returns false when interprocess_mutex 
      is already locked, returns true when success.
      Throws interprocess_exception if a severe error is found*/
   bool try_lock();

   /*! Tries to lock the interprocess_mutex until time abs_time,
      Returns false when timeout expires, returns true when locks.
      Throws interprocess_exception if a severe error is found*/
   bool timed_lock(const boost::posix_time::ptime &abs_time);

   /*! Erases a named mutex from the system*/
   static bool remove(const char *name);

   private:
   shared_memory        m_shmem;

   class construct_func_t;
};

class named_mutex::construct_func_t
{
   public:
   enum CreationType {  open_only, open_or_create, create_only  };

   construct_func_t(CreationType type)
      :  m_creation_type(type){}

   bool operator()(const mapped_region &region, bool created) const
   {   
      switch(m_creation_type){
         case open_only:
            return true;
         break;
         case create_only:
         case open_or_create:
            if(created){
               new(region.get_address())interprocess_mutex;
            }
            return true;
         break;

         default:
            return false;
         break;
      }
      return true;
   }
   private:
   CreationType       m_creation_type;
};

inline named_mutex::~named_mutex()
{}

inline named_mutex::named_mutex(detail::create_only_t, const char *name)
   :  m_shmem  (create_only
               ,name
               ,sizeof(interprocess_mutex)
               ,memory_mapping::rw_mode
               ,0
               ,construct_func_t(construct_func_t::create_only))
{}

inline named_mutex::named_mutex(detail::open_or_create_t, const char *name)
   :  m_shmem  (open_or_create
               ,name
               ,sizeof(interprocess_mutex)
               ,memory_mapping::rw_mode
               ,0
               ,construct_func_t(construct_func_t::open_or_create))
{}

inline named_mutex::named_mutex(detail::open_only_t, const char *name)
   :  m_shmem  (open_only
               ,name
               ,memory_mapping::rw_mode
               ,0
               ,construct_func_t(construct_func_t::open_only))
{}

inline void named_mutex::lock()
{  static_cast<interprocess_mutex*>(m_shmem.get_address())->lock();  }

inline void named_mutex::unlock()
{  static_cast<interprocess_mutex*>(m_shmem.get_address())->unlock();  }

inline bool named_mutex::try_lock()
{  return static_cast<interprocess_mutex*>(m_shmem.get_address())->try_lock();  }

inline bool named_mutex::timed_lock(const boost::posix_time::ptime &abs_time)
{  return static_cast<interprocess_mutex*>(m_shmem.get_address())->timed_lock(abs_time);  }

inline bool named_mutex::remove(const char *name)
{  return shared_memory::remove(name); }

}  //namespace interprocess {

}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_NAMED_MUTEX_HPP
