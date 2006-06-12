//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_SHARABLE_MUTEX_HPP
#define BOOST_INTERPROCESS_SHARABLE_MUTEX_HPP
        
#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/utility.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <assert.h>
#include <cstddef>

#include <boost/mpl/vector.hpp>
#include <boost/mpl/max_element.hpp>
#include <boost/mpl/transform_view.hpp>
#include <boost/mpl/sizeof.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/aligned_storage.hpp>
#include <boost/interprocess/exceptions.hpp>

namespace boost {

namespace interprocess {

/*!An object of class shared_read_write_mutex is a read_write_mutex that
   can be placed in shared memory so that it can be used to synchronize
   threads of different processes.*/
class interprocess_sharable_mutex
{
   public:

   enum sharable_mutex_policy
   {
//      system_default,                     //The default behavior of the system
      sharable_priority,                  //Prefer readers; can starve writers
      exclusive_priority,                 //Prefer writers; can starve readers
      no_priority,
      max_priority
   //   rw_alternating_many_reads,        //Alternate readers and writers; before a writer, release all queued readers 
   //   rw_alternating_single_read        //Alternate readers and writers; before a writer, release only on queued reader
   };

   interprocess_sharable_mutex(sharable_mutex_policy policy = sharable_priority)
   :  m_policy (policy)
   {  this->priv_init();  }

   ~interprocess_sharable_mutex()
   {  this->priv_fini();  }

   void lock_sharable()
   {  this->priv_sharable_lock(false, 0); }

   bool try_lock_sharable()
   {  return this->priv_sharable_lock(true, 0); }

   bool timed_lock_sharable(const boost::posix_time::ptime &pt)
   {  return this->priv_sharable_lock(true, &pt); }

   void unlock_sharable()
   {
      assert(m_policy < max_priority);
      switch(m_policy){
         case sharable_priority:
         case exclusive_priority:
            {
               se_priority_t *const m = this->priv_members<se_priority_t>();
               scoped_lock<interprocess_mutex> lock(m->mut);
               --m->shared_active;
               if(m->shared_active == 0 && m->exclusive_waiting > 0){
                  m->exclusive_cond.notify_one();
               }
            }
         break;
         case no_priority:
            priv_alt_priority_unlock();
         break;
         default:
            assert(0);
      }
   }

   void lock()
   {  this->priv_lock(false, 0);  }

   bool try_lock()
   {  return this->priv_lock(true, 0);  }

   bool timed_lock(const boost::posix_time::ptime &pt)
   {  return this->priv_lock(true, &pt);  }

   void unlock()
   {
      switch(m_policy){
         case sharable_priority:
            {
               se_priority_t *const m = this->priv_members<se_priority_t>();
               scoped_lock<interprocess_mutex> lock(m->mut);
               m->exclusive_active = 0;
               if(m->shared_waiting > 0){
                  m->shared_cond.notify_all();
               }
               else if (m->exclusive_waiting > 0){
                  m->exclusive_cond.notify_one();
               }
            }
         break;
         case exclusive_priority:
            {
               se_priority_t *const m = this->priv_members<se_priority_t>();
               scoped_lock<interprocess_mutex> lock(m->mut);
               m->exclusive_active = 0;
               if(m->exclusive_waiting > 0){
                  m->exclusive_cond.notify_one();
               }
               else if (m->shared_waiting > 0){
                  m->shared_cond.notify_all();
               }
            }
         break;
         case no_priority:
            priv_alt_priority_unlock();
         break;
         default:
            assert(0);
      }
   }
   
   sharable_mutex_policy get_policy() const
   {  return m_policy;  }

   private:

   //Anti-exception RAII
   struct counter_decrementer
   {
      counter_decrementer(std::size_t &count)
         :  m_count(count){}
      ~counter_decrementer()
         {  --m_count;  }
      std::size_t &m_count;
   };

   struct a_priority_wlock_rollback
   {
      a_priority_wlock_rollback(interprocess_sharable_mutex *lock)
         :  m_lock(lock){}

      ~a_priority_wlock_rollback()
      {
         if(m_lock){
            no_priority_t *const m = m_lock->priv_members<no_priority_t>();
            m->shared_count = -m->completed_shared_count;
            m->completed_shared_count = 0;
         }
      }

      void release(){   m_lock = 0;   }

      private:
      interprocess_sharable_mutex *m_lock;
   };

   bool priv_sharable_lock(bool timed, const boost::posix_time::ptime *pt)
   {
      assert(m_policy < max_priority);
      switch(m_policy){
         case sharable_priority:
            {
               se_priority_t *const m = this->priv_members<se_priority_t>();
               scoped_lock<interprocess_mutex> lock(m->mut);
               if(m->exclusive_active && !this->priv_se_priority_s_wait(lock, timed, pt)){
                  return false;
               }
               ++m->shared_active;
            }
         break;
         case exclusive_priority:
            {
               se_priority_t *const m = this->priv_members<se_priority_t>();
               scoped_lock<interprocess_mutex> lock(m->mut);
               if((m->exclusive_active || m->exclusive_waiting > 0) &&
                  !this->priv_se_priority_s_wait(lock, timed, pt)){
                  return false;
               }
               ++m->shared_active;
            }
         break;

         case no_priority:
            {
               no_priority_t *const m = this->priv_members<no_priority_t>();
               scoped_lock<interprocess_mutex> lock(m->exclusive_mutex, dont_lock);

               if(!this->priv_locking(lock, timed, pt)){
                  return false;
               }

               if(++m->shared_count == INT_MAX){
                  scoped_lock<interprocess_mutex> lock(m->shared_mutex, *pt);
                  if(!lock.locked()){
	                  ++m->completed_shared_count;
                     return false;
                  }
                  m->shared_count          -= m->completed_shared_count;
                  m->completed_shared_count = 0;
               }
               return true;
            }
         break;
         default:
            assert(0);
            return false;
      }
      return true;
   }

   bool priv_lock(bool timed, const boost::posix_time::ptime *pt)
   {   
      assert(m_policy < max_priority);
      switch(m_policy){
         case sharable_priority:
         case exclusive_priority:
            {
               se_priority_t *const m = this->priv_members<se_priority_t>();
               scoped_lock<interprocess_mutex> lock(m->mut);
               if((m->exclusive_active || m->shared_active > 0) && 
                  !this->priv_se_priority_e_wait(lock, timed, pt)){
                  return false;
               }
               m->exclusive_active = 1;
            }
         break;

         case no_priority:
            {
               no_priority_t *const m = this->priv_members<no_priority_t>();

               //Initialize scoped locks
               scoped_lock<interprocess_mutex> lock_exclusive(m->exclusive_mutex, dont_lock);
               scoped_lock<interprocess_mutex> lock_shared   (m->shared_mutex,    dont_lock);

               if(!this->priv_locking(lock_exclusive, timed, pt))return false;
               if(!this->priv_locking(lock_shared,    timed, pt))return false;

               if(m->exclusive_count == 0){
                  if(m->completed_shared_count > 0){
                     m->shared_count          -= m->completed_shared_count;
                     m->completed_shared_count = 0;
                  }
                  if(m->shared_count > 0){
                     //This logic is for timed a infinite waits
	                  m->completed_shared_count = -m->shared_count;
                     a_priority_wlock_rollback rollback(this);

	                  do{
                        if(!this->priv_waiting
                              (lock_shared, m->shared_cond, timed, pt)){
                           m->shared_count = 0;
                           return false;
                        }
	                  }while (m->completed_shared_count < 0);

                     m->shared_count = 0;
                     rollback.release();
                  }
               }

               ++m->exclusive_count;
               //Let mutexes locked
               lock_exclusive.release();
               lock_shared.release();
            }
         default:
            return false;
      }
      return true;
   }

   bool priv_se_priority_s_wait(scoped_lock<interprocess_mutex> &lock, bool timed, const boost::posix_time::ptime *pt)
   {
      se_priority_t *const m = this->priv_members<se_priority_t>();
      //We should block
      ++m->shared_waiting;
      counter_decrementer decrementer(m->shared_waiting);
      do{
         if(!this->priv_waiting(lock, m->shared_cond, timed, pt))
            return false;
      }while(m->exclusive_active);
      return true;
   }

   bool priv_se_priority_e_wait(scoped_lock<interprocess_mutex> &lock, bool timed, const boost::posix_time::ptime *pt)
   {
      se_priority_t *const m = this->priv_members<se_priority_t>();
      //We should block
      ++m->exclusive_waiting;
      counter_decrementer decrementer(m->exclusive_waiting);
      do{
         if(!this->priv_waiting(lock, m->exclusive_cond, timed, pt)){
            return false;
         }
      }while(m->exclusive_active || m->shared_active > 0);
      return true;
   }

   void priv_alt_priority_unlock()
   {
      no_priority_t *const m = this->priv_members<no_priority_t>();
      if (m->exclusive_count == 0){
         scoped_lock<interprocess_mutex> lock(m->shared_mutex);
         if(++m->completed_shared_count == 0) {
            m->shared_cond.notify_one();
         }
      }
      else{
         --m->exclusive_count;
         m->shared_mutex.unlock();
         m->exclusive_mutex.unlock();
      }
   }

   private:

   bool priv_locking(scoped_lock<interprocess_mutex> &lock,
                        bool timed,
                        const boost::posix_time::ptime *pt)
   {
      return timed ? (!pt ? lock.try_lock() : lock.timed_lock(*pt))
                   : (lock.lock(), true);
   }

   bool priv_waiting(scoped_lock<interprocess_mutex> &lock,
                        interprocess_condition          &cond,
                        bool timed,
                        const boost::posix_time::ptime *pt)
   {
      return timed ? (!pt ? false : cond.timed_wait(lock, *pt))
                   : (cond.wait(lock), true);
   }

   struct se_priority_t
   {
      se_priority_t()
         :  mut(), shared_cond(),   exclusive_cond()
         ,  shared_active(0),       shared_waiting(0)
         ,  exclusive_active(0),    exclusive_waiting(0)
      {}

      interprocess_mutex                         mut;
      interprocess_condition                     shared_cond;
      interprocess_condition                     exclusive_cond;
      std::size_t                   shared_active;
      std::size_t                   shared_waiting;
      std::size_t                   exclusive_active;
      std::size_t                   exclusive_waiting;
   };

   struct no_priority_t
   {
      no_priority_t()
         :  exclusive_mutex(),   shared_mutex(),   shared_cond()
         ,  completed_shared_count(0), shared_count(0), exclusive_count(0)
      {}

      //shared by all methods
      interprocess_mutex                         exclusive_mutex;
      interprocess_mutex                         shared_mutex;
      interprocess_condition                     shared_cond;
      int                           completed_shared_count;
      int                           shared_count;
      int                           exclusive_count;
   };

   //Let's make a smart "union" type
   typedef boost::mpl::vector<se_priority_t, no_priority_t> types;
   //Let's get the size of the "union"
   typedef boost::mpl::max_element
      <boost::mpl::transform_view   <types
                                    ,boost::mpl::sizeof_<boost::mpl::_1> 
                                    > >::type iter;
   //This is the instance of the variant
   typedef boost::aligned_storage
      <boost::mpl::deref<iter>::type::value>::type    raw_data_t;

   template<class T>
   const T * const priv_members() const
   {  return static_cast<const T* const>(static_cast<const void*const>(&m_raw));  }

   template<class T>
   T * const priv_members()
   {  return static_cast<T* const>(static_cast<void*const>(&m_raw));  }

   void priv_init()
   {
      switch(m_policy){
         case sharable_priority:
         case exclusive_priority:
            new (&m_raw)se_priority_t();
         break;
         case no_priority:
            new (&m_raw)no_priority_t();
         break;
         default:
         {  throw interprocess_exception(error_info(bad_sharable_lock)); }
      }
   }

   void priv_fini()
   {
      switch(m_policy){
         case sharable_priority:
         case exclusive_priority:
            this->priv_members<se_priority_t>()->~se_priority_t();
         break;
         case no_priority:
            this->priv_members<no_priority_t>()->~no_priority_t();
         break;
         default:
            //This sharable lock type is unknown
            assert(0);
      }
   }

   const sharable_mutex_policy   m_policy;
   raw_data_t                    m_raw;
};

/*
class interprocess_sharable_mutex
{
   public:

   enum sharable_mutex_policy
   {
      sharable_priority,               //Prefer readers; can starve writers
      exclusive_priority,               //Prefer writers; can starve readers
      no_priority,
      max_priority
   //   rw_alternating_many_reads,        //Alternate readers and writers; before a writer, release all queued readers 
   //   rw_alternating_single_read        //Alternate readers and writers; before a writer, release only on queued reader
   };

   interprocess_sharable_mutex(sharable_mutex_policy policy = sharable_priority)
   :  m_policy (policy)
   {  this->init();  }

   ~interprocess_sharable_mutex()
   {  this->fini();  }

   void lock_sharable()
   {  this->do_sharable_lock(false, 0); }

   bool try_lock_sharable()
   {  return this->do_sharable_lock(true, 0); }

   bool timed_lock_sharable(const boost::posix_time::ptime &pt)
   {  return this->do_sharable_lock(true, &pt); }

   void unlock_sharable()
   {
      assert(m_policy < max_priority);
      switch(m_policy){
         case sharable_priority:
         case exclusive_priority:
            {
               se_priority_t *const m = this->get_se_priority_t();
               scoped_lock<interprocess_mutex> lock(m->interprocess_mutex);
               --m->shared_active;
               if(m->shared_active == 0 && m->exclusive_waiting > 0){
                  m->exclusive_cond.notify_one();
               }
            }
         break;
         case no_priority:
            do_alt_priority_unlock();
         break;
         default:
            assert(0);
      }
   }

   void lock()
   {  this->do_lock(false, 0);  }

   bool try_lock()
   {  return this->do_lock(true, 0);  }

   bool timed_lock(const boost::posix_time::ptime &pt)
   {  return this->do_lock(true, &pt);  }

   void unlock()
   {
      switch(m_policy){
         case sharable_priority:
            {
               se_priority_t *const m = this->get_se_priority_t();
               scoped_lock<interprocess_mutex> lock(m->interprocess_mutex);
               m->exclusive_active = 0;
               if(m->shared_waiting > 0){
                  m->shared_cond.notify_all();
               }
               else if (m->exclusive_waiting > 0){
                  m->exclusive_cond.notify_one();
               }
            }
         break;
         case exclusive_priority:
            {
               se_priority_t *const m = this->get_se_priority_t();
               scoped_lock<interprocess_mutex> lock(m->interprocess_mutex);
               m->exclusive_active = 0;
               if(m->exclusive_waiting > 0){
                  m->exclusive_cond.notify_one();
               }
               else if (m->shared_waiting > 0){
                  m->shared_cond.notify_all();
               }
            }
         break;
         case no_priority:
            do_alt_priority_unlock();
         break;
         default:
            assert(0);
      }
   }
   
   sharable_mutex_policy get_policy() const
   {  return m_policy;  }

   private:

   //Anti-exception RAII
   struct counter_decrementer
   {
      counter_decrementer(std::size_t &count)
         : m_count(count){}
      ~counter_decrementer()
         {  --m_count;  }
      std::size_t &m_count;
   };

   struct a_priority_wlock_rollback
   {
      a_priority_wlock_rollback(interprocess_sharable_mutex *lock)
         :  m_lock(lock){}

      ~a_priority_wlock_rollback()
      {
         if(m_lock){
            no_priority_t *const m = m_lock->get_members<no_priority_t>();
            m->shared_count = -m->completed_shared_count;
            m->completed_shared_count = 0;
         }
      }

      void release(){   m_lock = 0;   }

      private:
      interprocess_sharable_mutex *m_lock;
   };

   bool do_sharable_lock(bool timed, const boost::posix_time::ptime *pt)
   {
      assert(m_policy < max_priority);
      switch(m_policy){
         case sharable_priority:
            {
               se_priority_t *const m = this->get_se_priority_t();
               scoped_lock<interprocess_mutex> lock(m->interprocess_mutex);
               if(m->exclusive_active && !this->do_se_priority_s_wait(lock, timed, pt)){
                  return false;
               }
               ++m->shared_active;
            }
         break;
         case exclusive_priority:
            {
               se_priority_t *const m = this->get_se_priority_t();
               scoped_lock<interprocess_mutex> lock(m->interprocess_mutex);
               if((m->exclusive_active || m->exclusive_waiting > 0) &&
                  !this->do_se_priority_s_wait(lock, timed, pt)){
                  return false;
               }
               ++m->shared_active;
            }
         break;

         case no_priority:
            {
               no_priority_t *const m = this->get_members<no_priority_t>();
               if(timed && !pt){
                  scoped_lock<interprocess_mutex> lock(m->exclusive_mutex, try_to_lock);
                  if(!lock.locked())   return false;
                  
                  if(++m->shared_count == INT_MAX){
                     //Is better to throw in this case?
                     //m->shared_mutex.do_lock();
                     scoped_lock<interprocess_mutex> lock(m->shared_mutex);
                     m->shared_count          -= m->completed_shared_count;
                     m->completed_shared_count = 0;
                  }
               }
               else if(timed){
                  scoped_lock<interprocess_mutex> lock(m->exclusive_mutex, *pt);
                  if(!lock.locked()){
                     return false;
                  }
                  if(++m->shared_count == INT_MAX){
                     //Is better to throw in this case?
                     //m->shared_mutex.do_lock();
                     scoped_lock<interprocess_mutex> lock(m->shared_mutex, *pt);
                     if(!lock.locked()){
	                     ++m->completed_shared_count;
                        return false;
                     }
                     m->shared_count          -= m->completed_shared_count;
                     m->completed_shared_count = 0;
                     return true;
                  }
               }
               else{
                  scoped_lock<interprocess_mutex>   lock(m->exclusive_mutex);
                  if(++m->shared_count == INT_MAX){
                     //Is better to throw in this case?
                     //m->shared_mutex.do_lock();
                     scoped_lock<interprocess_mutex> lock(m->shared_mutex);
                     m->shared_count          -= m->completed_shared_count;
                     m->completed_shared_count = 0;
                     return true;
                  }
               }
            }
         break;
         default:
            assert(0);
            return false;
      }
      return true;
   }

   bool do_lock(bool timed, const boost::posix_time::ptime *pt)
   {   
      assert(m_policy < max_priority);
      switch(m_policy){
         case sharable_priority:
         case exclusive_priority:
            {
               se_priority_t *const m = this->get_se_priority_t();
               scoped_lock<interprocess_mutex> lock(m->interprocess_mutex);
               if((m->exclusive_active || m->shared_active > 0) && 
                  !this->do_se_priority_e_wait(lock, timed, pt)){
                  return false;
               }
               m->exclusive_active = 1;
            }
         break;

         case no_priority:
            {
               no_priority_t *const m = this->get_members<no_priority_t>();
               //Initialize scoped locks
               scoped_lock<interprocess_mutex> lock_exclusive(m->exclusive_mutex, dont_lock);
               scoped_lock<interprocess_mutex> lock_shared   (m->shared_mutex, dont_lock);

               //Do initialization depending on lock type
               if(timed && !pt){
                  if(!lock_exclusive.try_lock())return false;
                  if(!lock_shared.try_lock())   return false;
               }
               else if(timed){
                  if(!lock_exclusive.timed_lock(*pt))return false;
                  if(!lock_shared.timed_lock(*pt))   return false;
               }
               else{
                  lock_exclusive.lock();
                  lock_shared.lock();
               }

               if(m->exclusive_count == 0){
                  if(m->completed_shared_count > 0){
                     m->shared_count          -= m->completed_shared_count;
                     m->completed_shared_count = 0;
                  }
                  if(m->shared_count > 0){
                     //This logic is for timed a infinite waits
	                  m->completed_shared_count = -m->shared_count;
                     a_priority_wlock_rollback rollback(this);

	                  do{
                        if(timed && !pt){
                           m->shared_count = 0;
                           return false;
                        }
                        else if(timed){
                           if(!m->shared_cond.timed_wait(lock_shared, *pt)){
                              m->shared_count = 0;
                              return false;
                           }
                        }
                        else{
                           m->shared_cond.wait(lock_shared);
                        }
	                  }while (m->completed_shared_count < 0);

                     m->shared_count = 0;
                     rollback.release();
                  }
               }

               ++m->exclusive_count;
               //Let mutexes locked
               lock_exclusive.release();
               lock_shared.release();
               return true;
            }
         break;
         default:
            assert(0);
            return false;
      }
      return true;
   }

   bool do_se_priority_s_wait(scoped_lock<interprocess_mutex> &lock, bool timed, const boost::posix_time::ptime *pt)
   {
      if(timed && !pt) return false;
      se_priority_t *const m = this->get_se_priority_t();
      //We should block
      ++m->shared_waiting;
      counter_decrementer decrementer(m->shared_waiting);
      do{
         if(timed){
            if(!m->shared_cond.timed_wait(lock, *pt))
               return false;
         }
         else{
            m->shared_cond.wait(lock);
         }
      }while(m->exclusive_active);
      return true;
   }

   bool do_se_priority_e_wait(scoped_lock<interprocess_mutex> &lock, bool timed, const boost::posix_time::ptime *pt)
   {
      if(timed && !pt) return false;
      se_priority_t *const m = this->get_se_priority_t();
      //We should block
      ++m->exclusive_waiting;
      counter_decrementer decrementer(m->exclusive_waiting);
      do{
         if(timed && !m->exclusive_cond.timed_wait(lock, *pt)){
            return false;
         }
         else{
            m->exclusive_cond.wait(lock);
         }
      }while(m->exclusive_active || m->shared_active > 0);
      return true;
   }

   void do_alt_priority_unlock()
   {
      no_priority_t *const m = this->get_members<no_priority_t>();
      if (m->exclusive_count == 0){
         scoped_lock<interprocess_mutex> lock(m->shared_mutex);
         if(++m->completed_shared_count == 0) {
            m->shared_cond.notify_one();
         }
      }
      else{
         --m->exclusive_count;
         m->shared_mutex.unlock();
         m->exclusive_mutex.unlock();
      }
   }

   private:

   struct se_priority_t
   {
      se_priority_t()
         :  shared_active(0),    shared_waiting(0), 
            exclusive_waiting(0),exclusive_active(0)
      {}

      interprocess_mutex                         interprocess_mutex;
      interprocess_condition                     shared_cond;
      interprocess_condition                     exclusive_cond;
      std::size_t                   shared_active;
      std::size_t                   shared_waiting;
      std::size_t                   exclusive_active;
      std::size_t                   exclusive_waiting;
   };

   struct no_priority_t
   {
      no_priority_t()
         :  completed_shared_count(0), shared_count(0), exclusive_count(0)
      {}

      //shared by all methods
      interprocess_mutex                         exclusive_mutex;
      interprocess_mutex                         shared_mutex;
      interprocess_condition                     shared_cond;
      int                           completed_shared_count;
      int                           shared_count;
      int                           exclusive_count;
   };

   typedef boost::mpl::vector<se_priority_t, no_priority_t> types;
   typedef boost::mpl::max_element
      <boost::mpl::transform_view   <types
                                    ,boost::mpl::sizeof_<boost::mpl::_1> 
                                    > >::type iter;

   typedef boost::aligned_storage
      <boost::mpl::deref<iter>::type::value>::type    raw_data_t;

   const se_priority_t *get_se_priority_t() const
   {  return static_cast<const se_priority_t*>(static_cast<const void*>(&m_raw));  }

   se_priority_t *get_se_priority_t()
   {  return static_cast<se_priority_t*>(static_cast<void*>(&m_raw));  }

   const no_priority_t *get_members<no_priority_t>() const
   {  return static_cast<const no_priority_t*>(static_cast<const void*>(&m_raw));  }

   no_priority_t *get_members<no_priority_t>()
   {  return static_cast<no_priority_t*>(static_cast<void*>(&m_raw));  }

   void init()
   {
      switch(m_policy){
         case sharable_priority:
         case exclusive_priority:
            new (&m_raw)se_priority_t();
         break;
         case no_priority:
            new (&m_raw)no_priority_t();
         break;
         default:
         {  throw interprocess_exception(error_info(bad_sharable_lock)); }
      }
   }

   void fini()
   {
      switch(m_policy){
         case sharable_priority:
         case exclusive_priority:
            this->get_se_priority_t()->~se_priority_t();
         break;
         case no_priority:
            this->get_members<no_priority_t>()->~no_priority_t();
         break;
         default:
            //This sharable lock type is unknown
            assert(0);
      }
   }

   const sharable_mutex_policy   m_policy;
   raw_data_t                    m_raw;
};
*/
/*
class interprocess_sharable_mutex
{
   public:

   enum sharable_mutex_policy
   {
      sharable_priority,               //Prefer readers; can starve writers
      exclusive_priority,               //Prefer writers; can starve readers
      no_priority,
      max_priority
   //   rw_alternating_many_reads,        //Alternate readers and writers; before a writer, release all queued readers 
   //   rw_alternating_single_read        //Alternate readers and writers; before a writer, release only on queued reader
   };

   interprocess_sharable_mutex(sharable_mutex_policy policy = sharable_priority)
   :  m_policy (policy), 
      m_r_active_completed_shared_count(0), m_r_wait_shared_count(0), m_w_wait_exclusive_count(0), m_w_active(0),
      m_completed_shared_count(0), m_shared_count(0), m_exclusive_count(0)
   {}

   ~interprocess_sharable_mutex()
      {}

   void lock_sharable()
   {  this->do_sharable_lock(false, 0); }

   bool try_lock_sharable()
   {  return this->do_sharable_lock(true, 0); }

   bool timed_lock_sharable(const boost::posix_time::ptime &pt)
   {  return this->do_sharable_lock(true, &pt); }

   void unlock_sharable()
   {
      assert(m_policy < max_priority);
      switch(m_policy){
         case sharable_priority:
         case exclusive_priority:
            {
               scoped_lock<interprocess_mutex> lock(m_mutex_exclusive_mutex);
               --m_r_active_completed_shared_count;
               if(m_r_active_completed_shared_count == 0 && m_w_wait_exclusive_count > 0){
                  exclusive_cond.notify_one();
               }
            }
         break;
         case no_priority:
            {
               do_alt_priority_unlock();
            }
         break;
         default:
            assert(0);
      }
   }

   void lock()
   {  this->do_lock(false, 0);  }

   bool try_lock()
   {  return this->do_lock(true, 0);  }

   bool timed_lock(const boost::posix_time::ptime &pt)
   {  return this->do_lock(true, &pt);  }

   void unlock()
   {
      switch(m_policy){
         case sharable_priority:
            {
               scoped_lock<interprocess_mutex> lock(m_mutex_exclusive_mutex);
               m_w_active = 0;
               if(m_r_wait_shared_count > 0){
                  m_reader_shared_cond.notify_all();
               }
               else if (m_w_wait_exclusive_count > 0){
                  exclusive_cond.notify_one();
               }
            }
         break;
         case exclusive_priority:
            {
               scoped_lock<interprocess_mutex> lock(m_mutex_exclusive_mutex);
               m_w_active = 0;
               if(m_w_wait_exclusive_count > 0){
                  exclusive_cond.notify_one();
               }
               else if (m_r_wait_shared_count > 0){
                  m_reader_shared_cond.notify_all();
               }
            }
         break;
         case no_priority:
            do_alt_priority_unlock();
         break;
         default:
            assert(0);
      }
   }
   
   sharable_mutex_policy get_policy() const
   {  return m_policy;  }

   private:

   //Anti-exception RAII
   struct counter_decrementer
   {
      counter_decrementer(std::size_t &count)
         : m_count(count){}
      ~counter_decrementer()
         {  --m_count;  }
      std::size_t &m_count;
   };

   struct a_priority_wlock_rollback
   {
      a_priority_wlock_rollback(interprocess_sharable_mutex *lock)
      :  m_lock(lock){}

      ~a_priority_wlock_rollback()
      {
         if(m_lock){
            m_lock->m_shared_count = -m_lock->m_completed_shared_count;
            m_lock->m_completed_shared_count = 0;
         }
      }

      void release(){   m_lock = 0;   }

      private:
      interprocess_sharable_mutex *m_lock;
   };


   bool do_sharable_lock(bool timed, const boost::posix_time::ptime *pt)
   {
      assert(m_policy < max_priority);
      switch(m_policy){
         case sharable_priority:
            {
               scoped_lock<interprocess_mutex> lock(m_mutex_exclusive_mutex);
               if(m_w_active && !this->do_se_priority_s_wait(lock, timed, pt)){
                  return false;
               }
               ++m_r_active_completed_shared_count;
            }
         break;
         case exclusive_priority:
            {
               scoped_lock<interprocess_mutex> lock(m_mutex_exclusive_mutex);
               if((m_w_active || m_w_wait_exclusive_count > 0) &&
                  !this->do_se_priority_s_wait(lock, timed, pt)){
                  return false;
               }
               ++m_r_active_completed_shared_count;
            }
         break;

         case no_priority:
            {
               if(timed && !pt){
                  scoped_lock<interprocess_mutex> lock
                     (m_mutex_exclusive_mutex, try_to_lock);
                  if(!lock.locked())   return false;
                  
                  if(++m_shared_count == INT_MAX){
                     //Is better to throw in this case?
                     //m_mutex_shared_completed.do_lock();
                     scoped_lock<interprocess_mutex> lock(m_mutex_shared_completed);
                     m_shared_count          -= m_completed_shared_count;
                     m_completed_shared_count = 0;
                  }
               }
               else if(timed){
                  scoped_lock<interprocess_mutex> lock
                     (m_mutex_exclusive_mutex, *pt);
                  if(!lock.locked()) return false;
                  if(++m_shared_count == INT_MAX){
                     //Is better to throw in this case?
                     //m_mutex_shared_completed.do_lock();
                     scoped_lock<interprocess_mutex> lock
                        (m_mutex_shared_completed, *pt);
                     if(!lock.locked()){
	                     ++m_completed_shared_count;
                        return false;
                     }
                     m_shared_count          -= m_completed_shared_count;
                     m_completed_shared_count = 0;
                     return true;
                  }
               }
               else{
                  scoped_lock<interprocess_mutex> lock
                     (m_mutex_exclusive_mutex);
                  if(++m_shared_count == INT_MAX){
                     //Is better to throw in this case?
                     //m_mutex_shared_completed.do_lock();
                     scoped_lock<interprocess_mutex> lock
                        (m_mutex_shared_completed);
                     m_shared_count          -= m_completed_shared_count;
                     m_completed_shared_count = 0;
                     return true;
                  }
               }
            }
         break;
         default:
            assert(0);
            return false;
      }
      return true;
   }

   bool do_lock(bool timed, const boost::posix_time::ptime *pt)
   {   
      assert(m_policy < max_priority);
      switch(m_policy){
         case sharable_priority:
         case exclusive_priority:
            {
               scoped_lock<interprocess_mutex> lock(m_mutex_exclusive_mutex);
               if((m_w_active || m_r_active_completed_shared_count > 0) && 
                  !this->do_se_priority_e_wait(lock, timed, pt)){
                  return false;
               }
               m_w_active = 1;
            }
         break;

         case no_priority:
            {
               //Initialize scoped locks
               scoped_lock<interprocess_mutex> lock_exclusive
                  (m_mutex_exclusive_mutex, dont_lock);
               scoped_lock<interprocess_mutex> lock_shared
                  (m_mutex_shared_completed, dont_lock);
               //Do initialization depending on lock type
               if(timed && !pt){
                  if(!lock_exclusive.try_lock())return false;
                  if(!lock_shared.try_lock())   return false;
               }
               else if(timed){
                  if(!lock_exclusive.timed_lock(*pt))return false;
                  if(!lock_shared.timed_lock(*pt))   return false;
               }
               else{
                  lock_exclusive.lock();
                  lock_shared.lock();
               }
               //Now do common logic
               if(!do_alt_priority_e_wait(lock_shared, timed, pt))
                  return false;
               //We left the mutexes locked
               lock_shared.release();
               lock_exclusive.release();
               return true;
            }
         break;
         default:
            assert(0);
            return false;
      }
      return true;
   }

   bool do_se_priority_s_wait(scoped_lock<interprocess_mutex> &lock, bool timed, const boost::posix_time::ptime *pt)
   {
      if(timed && !pt) return false;
      //We should block
      ++m_r_wait_shared_count;
      counter_decrementer decrementer(m_r_wait_shared_count);
      do{
         if(timed){
            if(!m_reader_shared_cond.timed_wait(lock, *pt))
               return false;
         }
         else{
            m_reader_shared_cond.wait(lock);
         }
      }while(m_w_active);
      return true;
   }

   bool do_se_priority_e_wait(scoped_lock<interprocess_mutex> &lock, bool timed, const boost::posix_time::ptime *pt)
   {
      if(timed && !pt) return false;
      //We should block
      ++m_w_wait_exclusive_count;
      counter_decrementer decrementer(m_w_wait_exclusive_count);
      do{
         if(timed && !exclusive_cond.timed_wait(lock, *pt)){
            return false;
         }
         else{
            exclusive_cond.wait(lock);
         }
      }while(m_w_active || m_r_active_completed_shared_count > 0);
      return true;
   }

   bool do_alt_priority_e_wait(scoped_lock<interprocess_mutex> &lock_shared, bool timed, const boost::posix_time::ptime *pt)
   {
      //This would be a try lock
      if(timed && !pt) return false;

      if(m_exclusive_count == 0){
         if(m_completed_shared_count > 0){
            m_shared_count          -= m_completed_shared_count;
            m_completed_shared_count = 0;
         }
         if(m_shared_count > 0){
            //This logic is for timed a infinite waits
	         m_completed_shared_count = -m_shared_count;
            a_priority_wlock_rollback rollback(this);
	         do{
               if(timed){
                  if(!m_reader_shared_cond.timed_wait(lock_shared, *pt)){
                     return false;
                  }
               }
               else{
                  m_reader_shared_cond.wait(lock_shared);
               }
	         }while (m_completed_shared_count < 0);
            m_shared_count = 0;
            rollback.release();
         }
      }
//      else{
//         //Is this case correct?
//         lock_shared.release();
//         lock_exclusive.release();
//         return false;
//      }

      ++m_exclusive_count;
      return true;
   }

   void do_alt_priority_unlock()
   {
      if (m_exclusive_count == 0){
         scoped_lock<interprocess_mutex> lock(m_mutex_exclusive_mutex);
         if(++m_completed_shared_count == 0) {
            m_reader_shared_cond.notify_one();
         }
      }
      else{
         --m_exclusive_count;
         m_mutex_shared_completed.unlock();
         m_mutex_exclusive_mutex.unlock();
      }
   }

   private:

   const sharable_mutex_policy   m_policy;
   //shared by all methods
   interprocess_mutex                         m_mutex_exclusive_mutex;
   interprocess_condition                     m_reader_shared_cond;
   std::size_t                   m_r_active_completed_shared_count;
   std::size_t                   m_r_wait_shared_count;
   std::size_t                   m_w_wait_exclusive_count;
   //Only for reader-writer
   interprocess_condition                     exclusive_cond;
   std::size_t                   m_w_active;
   //Only for no_priority
   interprocess_mutex                         m_mutex_shared_completed;
   int                           m_completed_shared_count;
   int                           m_shared_count;
   int                           m_exclusive_count;
};

*/


}  // namespace interprocess

}  // namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif

