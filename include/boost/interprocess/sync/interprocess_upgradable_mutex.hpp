//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess/ for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_UPGRADABLE_MUTEX_HPP
#define BOOST_INTERPROCESS_UPGRADABLE_MUTEX_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/noncopyable.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <limits.h>


/*!\file
   Describes interprocess_upgradabel_mutex class
*/

namespace boost {

namespace interprocess {

/*!Wraps a interprocess_upgradable_mutex that can be placed in shared memory and can be 
   shared between processes. Allows timed lock tries*/
class interprocess_upgradable_mutex : private boost::noncopyable
{
   friend class interprocess_condition;
   public:

   interprocess_upgradable_mutex();

   ~interprocess_upgradable_mutex();

   //Exlusive locking

   void lock();

   bool try_lock();

   bool timed_lock(const boost::posix_time::ptime &abs_time);

   void unlock();

   //Upgradable locking

   void lock_upgradable();

   bool try_lock_upgradable();

   bool timed_lock_upgradable(const boost::posix_time::ptime &abs_time);

   void unlock_upgradable();

   //Sharable locking

   void lock_sharable();

   bool try_lock_sharable();

   bool timed_lock_sharable(const boost::posix_time::ptime &abs_time);

   void unlock_sharable();

   //Downgrading

   void unlock_and_lock_upgradable();

   void unlock_and_lock_sharable();

   void unlock_upgradable_and_lock_sharable();

   //Upgrading

   void unlock_upgradable_and_lock();

   bool try_unlock_upgradable_and_lock();

   bool timed_unlock_upgradable_and_lock(const boost::posix_time::ptime &abs_time);

   bool try_unlock_sharable_and_lock();

   bool try_unlock_sharable_and_lock_upgradable();

   private:
   typedef scoped_lock<interprocess_mutex> scoped_lock_t;

   //Pack all the control data in a word to be able
   //to use atomic instructions in the future
   struct control_word_t
   {
      unsigned exclusive_in         : 1;
      unsigned upgradable_in        : 1;
      unsigned num_upr_shar         : sizeof(unsigned)*CHAR_BIT-2;
   }                       m_ctrl;

   interprocess_mutex      m_mut;
   interprocess_condition  m_first_gate;
   interprocess_condition  m_second_gate;

   private:
   //Rollback structures for exceptions or failure return values
   struct exclusive_rollback
   {
      exclusive_rollback(control_word_t         &ctrl
                        ,interprocess_condition &first_gate)
         :  mp_ctrl(&ctrl), m_first_gate(first_gate)
      {}

      void release()
      {  mp_ctrl = 0;   }

      ~exclusive_rollback()
      {
         if(mp_ctrl){
            mp_ctrl->exclusive_in = 0;
            m_first_gate.notify_all();
         }
      }
      control_word_t          *mp_ctrl;
      interprocess_condition  &m_first_gate;
   };

   struct upgradable_to_exclusive_rollback
   {
      upgradable_to_exclusive_rollback(control_word_t         &ctrl)
         :  mp_ctrl(&ctrl)
      {}

      void release()
      {  mp_ctrl = 0;   }

      ~upgradable_to_exclusive_rollback()
      {
         if(mp_ctrl){
            //Recover upgradable lock
            mp_ctrl->upgradable_in = 1;
            ++mp_ctrl->num_upr_shar;   
            //Execute the second half of exclusive locking
            mp_ctrl->exclusive_in = 0;
         }
      }
      control_word_t          *mp_ctrl;
   };
};

inline interprocess_upgradable_mutex::interprocess_upgradable_mutex()
{
   this->m_ctrl.exclusive_in  = 0;
   this->m_ctrl.upgradable_in = 0;
   this->m_ctrl.num_upr_shar   = 0;
}

inline interprocess_upgradable_mutex::~interprocess_upgradable_mutex()
{}

inline void interprocess_upgradable_mutex::lock()
{
   scoped_lock_t lock(m_mut);

   //The exclusive lock must block in the first gate
   //if an exclusive or upgradable lock has been acquired
    while (this->m_ctrl.exclusive_in || this->m_ctrl.upgradable_in){
        this->m_first_gate.wait(lock);
   }

   //Mark that exclusive lock has been acquired
   this->m_ctrl.exclusive_in = 1;

   //Prepare rollback
   exclusive_rollback rollback(this->m_ctrl, this->m_first_gate);

   //Now wait until all readers are gone
    while (this->m_ctrl.num_upr_shar){
        this->m_second_gate.wait(lock);
   }
   rollback.release();
}

inline bool interprocess_upgradable_mutex::try_lock()
{
   scoped_lock_t lock(m_mut, try_to_lock);

   //If we can't lock or any has there is any exclusive, upgradable 
   //or sharable mark return false;
   if(!lock.locked() 
      || this->m_ctrl.exclusive_in 
      || this->m_ctrl.num_upr_shar){
      return false;
   }
   this->m_ctrl.exclusive_in = 1;
   return true;
}

inline bool interprocess_upgradable_mutex::timed_lock
   (const boost::posix_time::ptime &abs_time)
{
   scoped_lock_t lock(m_mut, abs_time);
   if(!lock.locked())   return false;

   //The exclusive lock must block in the first gate
   //if an exclusive or upgradable lock has been acquired
    while (this->m_ctrl.exclusive_in || this->m_ctrl.upgradable_in){
        if(!this->m_first_gate.timed_wait(lock, abs_time))
         return false;
   }

   //Mark that exclusive lock has been acquired
   this->m_ctrl.exclusive_in = 1;

   //Prepare rollback
   exclusive_rollback rollback(this->m_ctrl, this->m_first_gate);

   //Now wait until all readers are gone
    while (this->m_ctrl.num_upr_shar){
        if(!this->m_second_gate.timed_wait(lock, abs_time)){
         return false;
      }
   }
   rollback.release();
   return true;
}

inline void interprocess_upgradable_mutex::unlock()
{
   scoped_lock_t lock(m_mut);
   this->m_ctrl.exclusive_in = 0;
   this->m_first_gate.notify_all();
}

//Upgradable locking

inline void interprocess_upgradable_mutex::lock_upgradable()
{
   scoped_lock_t lock(m_mut);

   //The upgradable lock must block in the first gate
   //if an exclusive or upgradable lock has been acquired
   //or there are too many sharable locks
    while(this->m_ctrl.exclusive_in || this->m_ctrl.upgradable_in
         || ((~this->m_ctrl.num_upr_shar) == 0)){
        this->m_first_gate.wait(lock);
   }

   //Mark that upgradable lock has been acquired
   //And add upgradable to the sharable count
   this->m_ctrl.upgradable_in = 1;
   ++this->m_ctrl.num_upr_shar;
}

inline bool interprocess_upgradable_mutex::try_lock_upgradable()
{
   scoped_lock_t lock(m_mut, try_to_lock);

   //The upgradable lock must fail
   //if an exclusive or upgradable lock has been acquired
   //or there are too many sharable locks
    if(!lock.locked() 
      || this->m_ctrl.exclusive_in 
      || this->m_ctrl.upgradable_in 
      || ((~this->m_ctrl.num_upr_shar) == 0)){
      return false;
   }

   //Mark that upgradable lock has been acquired
   //And add upgradable to the sharable count
   this->m_ctrl.upgradable_in = 1;
   ++this->m_ctrl.num_upr_shar;
}

inline bool interprocess_upgradable_mutex::timed_lock_upgradable
   (const boost::posix_time::ptime &abs_time)
{
   scoped_lock_t lock(m_mut, abs_time);
   if(!lock.locked())   return false;

   //The upgradable lock must block in the first gate
   //if an exclusive or upgradable lock has been acquired
   //or there are too many sharable locks
    while(this->m_ctrl.exclusive_in 
         || this->m_ctrl.upgradable_in
         || ((~this->m_ctrl.num_upr_shar) == 0)){
        if(!this->m_first_gate.timed_wait(lock, abs_time)){
         return false;
      }
   }

   //Mark that upgradable lock has been acquired
   //And add upgradable to the sharable count
   this->m_ctrl.upgradable_in = 1;
   ++this->m_ctrl.num_upr_shar;
   return true;
}

inline void interprocess_upgradable_mutex::unlock_upgradable()
{
   scoped_lock_t lock(m_mut);
   //Mark that upgradable lock has been acquired
   //And add upgradable to the sharable count
   this->m_ctrl.upgradable_in = 0;
   --this->m_ctrl.num_upr_shar;
   this->m_first_gate.notify_all();
}

//Sharable locking

inline void interprocess_upgradable_mutex::lock_sharable()
{
   scoped_lock_t lock(m_mut);

   //The sharable lock must block in the first gate
   //if an exclusive lock has been acquired
   //or there are too many sharable locks
    while(this->m_ctrl.exclusive_in
        || ((~this->m_ctrl.num_upr_shar) == 0)){
        this->m_first_gate.wait(lock);
   }

   //Increment sharable count
   ++this->m_ctrl.num_upr_shar;
}

inline bool interprocess_upgradable_mutex::try_lock_sharable()
{
   scoped_lock_t lock(m_mut, try_to_lock);

   //The sharable lock must fail
   //if an exclusive lock has been acquired
   //or there are too many sharable locks
    if(!lock.locked()
      || this->m_ctrl.exclusive_in
      || ((~this->m_ctrl.num_upr_shar) == 0)){
        return false;
   }

   //Increment sharable count
   ++this->m_ctrl.num_upr_shar;
   return true;
}

inline bool interprocess_upgradable_mutex::timed_lock_sharable
   (const boost::posix_time::ptime &abs_time)
{
   scoped_lock_t lock(m_mut, abs_time);
   if(!lock.locked())   return false;

   //The sharable lock must block in the first gate
   //if an exclusive lock has been acquired
   //or there are too many sharable locks
    while (this->m_ctrl.exclusive_in
         || ((~this->m_ctrl.num_upr_shar) == 0)){
        if(!this->m_first_gate.timed_wait(lock, abs_time)){
         return false;
      }
   }

   //Increment sharable count
   ++this->m_ctrl.num_upr_shar;
   return true;
}

inline void interprocess_upgradable_mutex::unlock_sharable()
{
   scoped_lock_t lock(m_mut);
   //Decrement sharable count
   --this->m_ctrl.num_upr_shar;
    if (this->m_ctrl.num_upr_shar == 0){
        this->m_second_gate.notify_one();
   }
   //Check if there are blocked sharable because of
   //there were too many sharable
    else if((~(this->m_ctrl.num_upr_shar+1)) == 0){
        this->m_first_gate.notify_all();
    }
}

//Downgrading

inline void interprocess_upgradable_mutex::unlock_and_lock_upgradable()
{
   scoped_lock_t lock(m_mut);
   //Unmark it as exclusive
   this->m_ctrl.exclusive_in     = 0;
   //Mark it as upgradable
   this->m_ctrl.upgradable_in    = 1;
   //The sharable count should be 0 so increment it
   this->m_ctrl.num_upr_shar   = 1;
   //Notify readers that they can enter
    m_first_gate.notify_all();
}

inline void interprocess_upgradable_mutex::unlock_and_lock_sharable()
{
   scoped_lock_t lock(m_mut);
   //Unmark it as exclusive
   this->m_ctrl.exclusive_in   = 0;
   //The sharable count should be 0 so increment it
   this->m_ctrl.num_upr_shar   = 1;
   //Notify readers that they can enter
    m_first_gate.notify_all();
}

inline void interprocess_upgradable_mutex::unlock_upgradable_and_lock_sharable()
{
   scoped_lock_t lock(m_mut);
   //Unmark it as upgradable (we don't have to decrement count)
   this->m_ctrl.upgradable_in    = 0;
   //Notify readers/upgradable that they can enter
    m_first_gate.notify_all();
}

//Upgrading

inline void interprocess_upgradable_mutex::unlock_upgradable_and_lock()
{
   scoped_lock_t lock(m_mut);
   //Simulate unlock_upgradable() without
   //notifying sharables.
   this->m_ctrl.upgradable_in = 0;
   --this->m_ctrl.num_upr_shar;   
   //Execute the second half of exclusive locking
   this->m_ctrl.exclusive_in = 1;

   //Prepare rollback
   upgradable_to_exclusive_rollback rollback(m_ctrl);

    while (this->m_ctrl.num_upr_shar){
        this->m_second_gate.wait(lock);
   }
   rollback.release();
}

inline bool interprocess_upgradable_mutex::try_unlock_upgradable_and_lock()
{
   scoped_lock_t lock(m_mut, try_to_lock);
   //Check if there are no readers
    if(!lock.locked()
      || this->m_ctrl.num_upr_shar != 1){
        return false;
   }
   //Now unlock upgradable and mark exclusive
   this->m_ctrl.upgradable_in = 0;
   --this->m_ctrl.num_upr_shar;   
   this->m_ctrl.exclusive_in = 1;
   return true;
}

inline bool interprocess_upgradable_mutex::timed_unlock_upgradable_and_lock
   (const boost::posix_time::ptime &abs_time)
{
   scoped_lock_t lock(m_mut, abs_time);
   if(!lock.locked())   return false;

   //Simulate unlock_upgradable() without
   //notifying sharables.
   this->m_ctrl.upgradable_in = 0;
   --this->m_ctrl.num_upr_shar;   
   //Execute the second half of exclusive locking
   this->m_ctrl.exclusive_in = 1;

   //Prepare rollback
   upgradable_to_exclusive_rollback rollback(m_ctrl);

    while (this->m_ctrl.num_upr_shar){
        if(!this->m_second_gate.timed_wait(lock, abs_time)){
         return false;
      }
   }
   rollback.release();
   return true;
}

inline bool interprocess_upgradable_mutex::try_unlock_sharable_and_lock()
{
   scoped_lock_t lock(m_mut, try_to_lock);

   //If we can't lock or any has there is any exclusive, upgradable 
   //or sharable mark return false;
   if(!lock.locked() 
      || this->m_ctrl.exclusive_in 
      || this->m_ctrl.num_upr_shar == 1){
      return false;
   }
   this->m_ctrl.exclusive_in = 1;
   return true;
}

inline bool interprocess_upgradable_mutex::try_unlock_sharable_and_lock_upgradable()
{
   scoped_lock_t lock(m_mut, try_to_lock);

   //The upgradable lock must fail
   //if an exclusive or upgradable lock has been acquired
    if(!lock.locked()
      || this->m_ctrl.exclusive_in 
      || this->m_ctrl.upgradable_in){
      return false;
   }

   //Mark that upgradable lock has been acquired
   this->m_ctrl.upgradable_in = 1;
   return true;
}

}  //namespace interprocess {

}  //namespace boost {


#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_UPGRADABLE_MUTEX_HPP
