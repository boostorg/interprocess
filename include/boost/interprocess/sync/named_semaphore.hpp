//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_NAMED_SEMAPHORE_HPP
#define BOOST_INTERPROCESS_NAMED_SEMAPHORE_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/posix_time_types_wrk.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/detail/managed_open_or_create_impl.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>

/*!\file
   Describes a named semaphore class for inter-process synchronization
*/

namespace boost {

namespace interprocess {

/*!A interprocess_semaphore with a global name, so it can be found from different 
   processes. Allows several resource sharing patterns and efficient 
   acknowledgment mechanisms.*/
class named_semaphore
{
   /// @cond
   //Non-copyable
   named_semaphore();
   named_semaphore(const named_semaphore &);
   named_semaphore &operator=(const named_semaphore &);
   /// @endcond
   public:
   /*!Creates a global interprocess_semaphore with a name, and an initial count. 
      It will return an false if the interprocess_semaphore is already created.*/
   named_semaphore(create_only_t, const char *name, int initialCount);

   /*!Opens or creates a global interprocess_semaphore with a name, and an initial count. 
      If the interprocess_semaphore is created, this call is equivalent to create(). 
      If the interprocess_semaphore is already created, this call is equivalent to open()
      and initialCount is ignored.*/
   named_semaphore(open_or_create_t, const char *name, int initialCount);

   /*!Opens a global interprocess_semaphore with a name if that interprocess_semaphore is previously.
      created. If it is not previously created this function return false.*/
   named_semaphore(open_only_t, const char *name);

   /*!Destroys the named semaphore. Does not throw*/
   ~named_semaphore();

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

   /*!Erases a named semaphore from the system*/
   static bool remove(const char *name);

   /// @cond
   private:
   detail::managed_open_or_create_impl<shared_memory_object> m_shmem;
   class construct_func_t;
   /// @endcond
};

/// @cond
class named_semaphore::construct_func_t
{
   public:
   enum CreationType {  open_only, open_or_create, create_only  };

   construct_func_t(CreationType type, int init_count)
      :  m_creation_type(type), m_init_count(init_count){}

   bool operator()(void *address, std::size_t, bool created) const
   {   
      switch(m_creation_type){
         case open_only:
            return true;
         break;
         case create_only:
         case open_or_create:
            if(created){
               new(address)interprocess_semaphore(m_init_count);
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
   CreationType   m_creation_type;
   int            m_init_count;
};
/// @endcond

inline named_semaphore::~named_semaphore()
{}

inline named_semaphore::named_semaphore
   (create_only_t, const char *name, int initialCount)
   :  m_shmem  (create_only
               ,name
               ,sizeof(interprocess_semaphore) +
                  detail::managed_open_or_create_impl<shared_memory_object>::
                     ManagedOpenOrCreateUserOffset
               ,read_write
               ,0
               ,construct_func_t(construct_func_t::create_only, initialCount))
{}

inline named_semaphore::named_semaphore
   (open_or_create_t, const char *name, int initialCount)
   :  m_shmem  (open_or_create
               ,name
               ,sizeof(interprocess_semaphore) +
                  detail::managed_open_or_create_impl<shared_memory_object>::
                     ManagedOpenOrCreateUserOffset
               ,read_write
               ,0
               ,construct_func_t(construct_func_t::open_or_create, initialCount))
{}

inline named_semaphore::named_semaphore
   (open_only_t, const char *name)
   :  m_shmem  (open_only
               ,name
               ,read_write
               ,0
               ,construct_func_t(construct_func_t::open_only, 0))
{}

inline void named_semaphore::post()
{  static_cast<interprocess_semaphore*>(m_shmem.get_address())->post();   }

inline void named_semaphore::wait()
{  static_cast<interprocess_semaphore*>(m_shmem.get_address())->wait();   }

inline bool named_semaphore::try_wait()
{  return static_cast<interprocess_semaphore*>(m_shmem.get_address())->try_wait();   }

inline bool named_semaphore::timed_wait(const boost::posix_time::ptime &abs_time)
{  return static_cast<interprocess_semaphore*>(m_shmem.get_address())->timed_wait(abs_time);   }

inline bool named_semaphore::remove(const char *name)
{  return shared_memory_object::remove(name); }

}  //namespace interprocess {

}  //namespace boost {


#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_NAMED_SEMAPHORE_HPP
