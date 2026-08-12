//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_SPIN_CONDITION_HPP
#define BOOST_INTERPROCESS_DETAIL_SPIN_CONDITION_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/sync/cv_status.hpp>
#include <boost/interprocess/sync/spin/mutex.hpp>
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/os_thread_functions.hpp>
#include <boost/interprocess/timed_utils.hpp>
#include <boost/interprocess/sync/spin/wait.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/cstdint.hpp>

namespace boost {
namespace interprocess {
namespace ipcdetail {

class spin_condition
{
   spin_condition(const spin_condition &);
   spin_condition &operator=(const spin_condition &);

   public:
   spin_condition()
   {
      //Note that this class is initialized to zero.
      //So zeroed memory can be interpreted as an initialized
      //condition variable
      m_command      = SLEEP;
      m_num_waiters  = 0;
   }

   ~spin_condition()
   {
      //Notify all waiting threads
      //to allow POSIX semantics on condition destruction
      this->notify_all();
   }

   void notify_one()
   {  this->notify(NOTIFY_ONE);  }

   void notify_all()
   {  this->notify(NOTIFY_ALL);  }

   template <typename L>
   void wait(L& lock)
   {
      if (!lock)
         throw lock_exception();
      this->do_timed_wait_impl<false>(ustime(0u), *lock.mutex());
   }

   template <typename L, typename Pr>
   void wait(L& lock, Pr pred)
   {
      if (!lock)
         throw lock_exception();

      while (!pred())
         this->do_timed_wait_impl<false>(ustime(0u), *lock.mutex());
   }

   template <typename L, typename TimePoint>
   bool timed_wait(L& lock, const TimePoint &abs_time)
   {
      if (!lock)
         throw lock_exception();
      //Handle infinity absolute time here to avoid complications in do_timed_wait
      if(is_pos_infinity(abs_time)){
         this->wait(lock);
         return true;
      }
      return this->do_timed_wait_impl<true>(abs_time, *lock.mutex());
   }

   template <typename L, typename TimePoint, typename Pr>
   bool timed_wait(L& lock, const TimePoint &abs_time, Pr pred)
   {
      if (!lock)
         throw lock_exception();
      //Handle infinity absolute time here to avoid complications in do_timed_wait
      if(is_pos_infinity(abs_time)){
         this->wait(lock, pred);
         return true;
      }
      while (!pred()){
         if (!this->do_timed_wait_impl<true>(abs_time, *lock.mutex()))
            return pred();
      }
      return true;
   }

   template <typename L, class TimePoint>
   cv_status wait_until(L& lock, const TimePoint &abs_time)
   {  return this->timed_wait(lock, abs_time) ? cv_status::no_timeout : cv_status::timeout; }

   template <typename L, class TimePoint, typename Pr>
   bool wait_until(L& lock, const TimePoint &abs_time, Pr pred)
   {  return this->timed_wait(lock, abs_time, pred); }

   template <typename L, class Duration>
   cv_status wait_for(L& lock, const Duration &dur)
   {  return this->wait_until(lock, duration_to_ustime(dur)); }

   template <typename L, class Duration, typename Pr>
   bool wait_for(L& lock, const Duration &dur, Pr pred)
   {  return this->wait_until(lock, duration_to_ustime(dur), pred); }

   private:

   template<bool TimeoutEnabled, class InterprocessMutex, class TimePoint>
   bool do_timed_wait_impl(const TimePoint &abs_time, InterprocessMutex &mut)
   {
      typedef boost::interprocess::scoped_lock<spin_mutex> InternalLock;
      //The enter mutex guarantees that while executing a notification,
      //no other thread can execute the do_timed_wait method.
      {
         //---------------------------------------------------------------
         InternalLock lock;
         get_lock(bool_<TimeoutEnabled>(), m_enter_mut, lock, abs_time);

         if(!lock)
            return false;
         //---------------------------------------------------------------
         //We increment the waiting thread count protected so that it will be
         //always constant when another thread enters the notification logic.
         //The increment marks this thread as "waiting on spin_condition".
         //Relaxed: the increment is published to notifiers by the release of
         //the external mutex, that the notifier has to acquire
         atomic_add32_relaxed(const_cast<boost::uint32_t*>(&m_num_waiters), 1u);

         //We unlock the external mutex atomically with the increment
         mut.unlock();
      }

      //By default, we suppose that no timeout has happened
      bool timed_out  = false, unlock_enter_mut= false;

      //Loop until a notification indicates that the thread should
      //exit or timeout occurs
      while(1){
         //The thread sleeps/spins until a spin_condition commands a notification
         //Notification occurred, we will lock the checking mutex so that
         //An acquire read is enough this read is only
         //a hint, the compare and swap below is what consumes the command
         spin_wait swait;
         while(atomic_read32_acquire(&m_command) == SLEEP){
            swait.yield();

            //Check for timeout
            if(TimeoutEnabled){
               typedef typename microsec_clock<TimePoint>::time_point time_point;
               time_point now = get_now<TimePoint>(bool_<TimeoutEnabled>());

               if(now >= abs_time){
                  //If we can lock the mutex it means that no notification
                  //is being executed in this spin_condition variable
                  timed_out = m_enter_mut.try_lock();

                  //If locking fails, indicates that another thread is executing
                  //notification, so we play the notification game
                  if(!timed_out){
                     //There is an ongoing notification, we will try again later
                     continue;
                  }
                  //No notification in execution, since enter mutex is locked.
                  //We will execute time-out logic, so we will decrement count,
                  //release the enter mutex and return false.
                  break;
               }
            }
         }

         //If a timeout occurred, the mutex will not execute checking logic
         if(TimeoutEnabled && timed_out){
            //Decrement wait count. Relaxed is enough: this thread owns the
            //enter mutex (the try_lock above succeeded) and the release of
            //that mutex publishes the decrement to the next owner
            atomic_sub32_relaxed(const_cast<boost::uint32_t*>(&m_num_waiters), 1u);
            unlock_enter_mut = true;
            break;
         }
         else{
            //This compare and swap consumes the notification and, with it,
            //inherits the ownership of the enter mutex that notify() left
            //locked, so it needs acquire semantics to order this thread after
            //the notifier's locking of that mutex (otherwise the unlock below
            //could be ordered before the notifier's lock, losing the mutex).
            //It consumes on BOTH outcomes: succeeding (NOTIFY_ONE taken) and
            //failing reading NOTIFY_ALL, so the acquire-release variant is
            //used, the only one that also orders the failed compare with
            //acquire semantics (atomic_cas32_acquire only guarantees it when
            //the swap succeeds)
            boost::uint32_t result = atomic_cas32_acq_rel
                           (const_cast<boost::uint32_t*>(&m_command), SLEEP, NOTIFY_ONE);
            if(result == SLEEP){
               //Other thread has been notified and since it was a NOTIFY one
               //command, this thread must sleep again
               continue;
            }
            else if(result == NOTIFY_ONE){
               //If it was a NOTIFY_ONE command, only this thread should
               //exit. This thread has atomically marked command as sleep before
               //so no other thread will exit.
               //Decrement wait count. Relaxed is enough: this thread now owns
               //the enter mutex and the release of that mutex publishes the
               //decrement to the next owner
               unlock_enter_mut = true;
               atomic_sub32_relaxed(const_cast<boost::uint32_t*>(&m_num_waiters), 1u);
               break;
            }
            else{
               //If it is a NOTIFY_ALL command, all threads should return
               //from do_timed_wait function. Decrement wait count. Relaxed is
               //enough: a read-modify-write always reads the latest value in
               //the modification order, so exactly one thread, the last one,
               //will read 1 here even if the decrements are concurrent, and
               //the release of the enter mutex by that last thread publishes
               //all of them to the next owner
               unlock_enter_mut = 1 == atomic_sub32_relaxed(const_cast<boost::uint32_t*>(&m_num_waiters), 1u);
               //Check if this is the last thread of notify_all waiters
               //Only the last thread will release the mutex
               if(unlock_enter_mut){
                  //No compare is needed: notifiers are blocked on the enter
                  //mutex this thread now owns and no other waiter modifies the
                  //command, so it is necessarily still NOTIFY_ALL here. The
                  //store is published by the release of the enter mutex below,
                  //so the weakest available store is enough
                  atomic_write32_release(const_cast<boost::uint32_t*>(&m_command), SLEEP);
               }
               break;
            }
         }
      }

      //Unlock the enter mutex if it is a single notification, if this is
      //the last notified thread in a notify_all or a timeout has occurred
      if(unlock_enter_mut){
         m_enter_mut.unlock();
      }

      //Lock external again before returning from the method
      mut.lock();
      return !timed_out;
   }

   template <class TimePoint>
   static typename microsec_clock<TimePoint>::time_point get_now(bool_<true>)
   {  return microsec_clock<TimePoint>::universal_time();  }

   template <class TimePoint>
   static typename microsec_clock<TimePoint>::time_point get_now(bool_<false>)
   {  return typename microsec_clock<TimePoint>::time_point();  }

   template <class Mutex, class Lock, class TimePoint>
   static void  get_lock(bool_<true>, Mutex &m, Lock &lck, const TimePoint &abs_time)
   { 
      Lock dummy(m, abs_time);
      lck = boost::move(dummy);
   }

   template <class Mutex, class Lock, class TimePoint>
   static void get_lock(bool_<false>, Mutex &m, Lock &lck, const TimePoint &)
   { 
      Lock dummy(m);
      lck = boost::move(dummy);
   }

   void notify(boost::uint32_t command)
   {
      //This mutex guarantees that no other thread can enter to the
      //do_timed_wait method logic, so that thread count will be
      //constant until the function writes a NOTIFY_ALL command.
      //It also guarantees that no other notification can be signaled
      //on this spin_condition before this one ends
      m_enter_mut.lock();

      //Return if there are no waiters. An acquire read is enough: the count
      //is only modified owning the enter mutex, so the acquire performed
      //when locking it above already made the latest value visible
      if(!atomic_read32_acquire(&m_num_waiters)) {
         m_enter_mut.unlock();
         return;
      }

      //Notify that all threads should execute wait logic. Release semantics
      //are needed: a successful swap publishes the command and hands the
      //ownership of the still locked enter mutex over to the waiters that
      //consume it, which will be the ones releasing it. The compare can only
      //fail while another notification is being consumed, and that can't
      //happen here: the enter mutex is owned and any consumed command is
      //restored to SLEEP before its ownership is released
      spin_wait swait;
      while(SLEEP != atomic_cas32_release(const_cast<boost::uint32_t*>(&m_command), command, SLEEP)){
         swait.yield();
      }
      //The enter mutex will rest locked until the last waiting thread unlocks it
   }

   //Protocol invariants:
   //
   // - m_command transitions from SLEEP to NOTIFY_ONE/NOTIFY_ALL while
   //   the enter mutex is owned, and it is restored to SLEEP before that
   //   ownership is released, so a free enter mutex implies a SLEEP command.
   //
   // - notify() returns with the enter mutex still locked: its ownership is
   //   handed over, through m_command, to the waiters that consume the
   //   notification, and the last of them releases it. The locking and the
   //   releasing threads are different, so the mutex really works as a
   //   binary semaphore there.
   //
   // - A waiter can only exit through the timeout path after try_locking the
   //   enter mutex, which fails while a notification is in flight, so its
   //   only exit then is consuming the command: a notification posted when
   //   the waiter count is not zero is never lost.
   //
   // - m_num_waiters is only modified owning the enter mutex (directly or
   //   through the handoff above), so the release/acquire pair of that mutex
   //   publishes it and relaxed read-modify-writes are enough.
   enum { SLEEP = 0, NOTIFY_ONE, NOTIFY_ALL };
   spin_mutex  m_enter_mut;
   volatile boost::uint32_t    m_command;
   volatile boost::uint32_t    m_num_waiters;
};

}  //namespace ipcdetail
}  //namespace interprocess
}  //namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_DETAIL_SPIN_CONDITION_HPP
