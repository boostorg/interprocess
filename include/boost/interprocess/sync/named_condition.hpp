//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_NAMED_CONDITION_HPP
#define BOOST_NAMED_CONDITION_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/detail/managed_open_or_create_impl.hpp>
#include <boost/interprocess/detail/posix_time_types_wrk.hpp>

/*!\file
   Describes process-shared variables interprocess_condition class
*/

namespace boost {

namespace interprocess {

class named_condition
{
   /// @cond
   //Non-copyable
   named_condition();
   named_condition(const named_condition &);
   named_condition &operator=(const named_condition &);
   /// @endcond
   public:
   /*!Creates a global condition with a name.*/
   named_condition(detail::create_only_t create_only, const char *name);

   /*!Opens or creates a global condition with a name. 
      If the condition is created, this call is equivalent to create(). 
      If the condition is already created, this call is equivalent to open(). 
      Does not throw*/
   named_condition(detail::open_or_create_t open_or_create, const char *name);

   /*!Opens a global condition with a name if that condition is previously.
      created. If it is not previously created this function return false.
      Does not throw*/
   named_condition(detail::open_only_t open_only, const char *name);

   /*!Destroys *this*/
   ~named_condition();

   /*!If there is a thread waiting on *this, change that 
      thread's state to ready. Otherwise there is no effect.*/
   void notify_one();

   /*!Change the state of all threads waiting on *this to ready.
      If there are no waiting threads, notify_all() has no effect.*/
   void notify_all();

   /*!Releases the lock on the interprocess_mutex object associated with lock, blocks 
      the current thread of execution until readied by a call to 
      this->notify_one() or this->notify_all(), and then reacquires the lock.*/
   template <typename L>
   void wait(L& lock);

   /*!The same as: while (!pred()) wait(lock)*/
   template <typename L, typename Pr>
   void wait(L& lock, Pr pred);

   /*!Releases the lock on the interprocess_mutex object associated with lock, blocks 
      the current thread of execution until readied by a call to 
      this->notify_one() or this->notify_all(), or until time abs_time is reached, 
      and then reacquires the lock.
      Returns: false if time abs_time is reached, otherwise true.*/
   template <typename L>
   bool timed_wait(L& lock, const boost::posix_time::ptime &abs_time);

   /*!The same as:   while (!pred()) { 
                        if (!timed_wait(lock, abs_time)) return pred(); 
                     } return true;*/
   template <typename L, typename Pr>
   bool timed_wait(L& lock, const boost::posix_time::ptime &abs_time, Pr pred);

   static bool remove(const char *name);

   /// @cond
   private:

   interprocess_condition *condition() const
   {  return static_cast<interprocess_condition*>(m_shmem.get_address()); }

   detail::managed_open_or_create_impl<shared_memory_object> m_shmem;

   class construct_func_t;
   /// @endcond
};

/// @cond
class named_condition::construct_func_t
{
   public:
   enum CreationType {  open_only, open_or_create, create_only  };

   construct_func_t(CreationType type)
      :  m_creation_type(type){}

   bool operator()(void *address, std::size_t size, bool created) const
   {   
      switch(m_creation_type){
         case open_only:
            return true;
         break;
         case create_only:
         case open_or_create:
            if(created){
               new(address)interprocess_condition;
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
/// @endcond

inline named_condition::~named_condition()
{}

inline named_condition::named_condition(detail::create_only_t, const char *name)
   :  m_shmem  (create_only
               ,name
               ,sizeof(interprocess_condition) +
                  detail::managed_open_or_create_impl<shared_memory_object>::
                     ManagedOpenOrCreateUserOffset
               ,read_write
               ,0
               ,construct_func_t(construct_func_t::create_only))
{}

inline named_condition::named_condition(detail::open_or_create_t, const char *name)
   :  m_shmem  (open_or_create
               ,name
               ,sizeof(interprocess_condition) +
                  detail::managed_open_or_create_impl<shared_memory_object>::
                     ManagedOpenOrCreateUserOffset
               ,read_write
               ,0
               ,construct_func_t(construct_func_t::open_or_create))
{}

inline named_condition::named_condition(detail::open_only_t, const char *name)
   :  m_shmem  (open_only
               ,name
               ,read_write
               ,0
               ,construct_func_t(construct_func_t::open_only))
{}


inline void named_condition::notify_one()
{  this->condition()->notify_one();  }

inline void named_condition::notify_all()
{  this->condition()->notify_all();  }

template <typename L>
inline void named_condition::wait(L& lock)
{
   if (!lock)
      throw lock_exception();
   this->condition()->do_wait(*lock.mutex()->mutex());
}

template <typename L, typename Pr>
inline void named_condition::wait
   (L& lock, Pr pred)
{
   if (!lock)
      throw lock_exception();

   while (!pred())
      this->condition()->do_wait(*lock.mutex()->mutex());
}

template <typename L>
inline bool named_condition::timed_wait
   (L& lock, const boost::posix_time::ptime &abs_time)
{
   if (!lock)
         throw lock_exception();

   return this->condition()->do_timed_wait(abs_time, *lock.mutex()->mutex());
}

template <typename L, typename Pr>
inline bool named_condition::timed_wait
   (L& lock, const boost::posix_time::ptime &abs_time, Pr pred)
{
   if (!lock)
         throw lock_exception();

   while (!pred()){
         if (!this->condition()->do_timed_wait(abs_time, *lock.mutex()->mutex()))
            return pred();
   }

   return true;
}

inline bool named_condition::remove(const char *name)
{  return shared_memory_object::remove(name); }

}  //namespace interprocess
}  // namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif // BOOST_NAMED_CONDITION_HPP
