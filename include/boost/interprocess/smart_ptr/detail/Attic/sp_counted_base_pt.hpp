#ifndef BOOST_INTERPROCESS_DETAIL_SP_COUNTED_BASE_PT_HPP_INCLUDED
#define BOOST_INTERPROCESS_DETAIL_SP_COUNTED_BASE_PT_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//
//  This file is the adaptation for shared memory memory mapped
//  files of boost/detail/sp_counted_base_pt.hpp
//
//
//  Copyright (c) 2001, 2002, 2003 Peter Dimov and Multi Media Ltd.
//  Copyright 2004-2005 Peter Dimov
//  Copyright 2006 Ion Gaztanaga
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <typeinfo>
#include <boost/interprocess/sync/posix/pthread_helpers.hpp>

namespace boost{

namespace interprocess{

namespace detail{

class sp_counted_base
{
   private:

   sp_counted_base( sp_counted_base const & );
   sp_counted_base & operator= ( sp_counted_base const & );

   long use_count_;        // #shared
   long weak_count_;       // #weak + (#shared != 0)

   mutable pthread_mutex_t m_mut;

   public:

   sp_counted_base(): use_count_( 1 ), weak_count_( 1 )
   {
      detail::mutexattr_wrapper mut_attr;
      detail::mutex_initializer mut(m_mut, mut_attr);
      mut.release();
   }

   ~sp_counted_base() // nothrow
   {
      int res = pthread_mutex_destroy(&m_mut);
      assert(res  == 0);(void)res;
   }

   // dispose() is called when use_count_ drops to zero, to release
   // the resources managed by *this.
/*
   virtual void dispose() = 0; // nothrow

   // destroy() is called when weak_count_ drops to zero.

   virtual void destroy() // nothrow
   {
      delete this;
   }

   virtual void * get_deleter( std::type_info const & ti ) = 0;
*/
   void add_ref_copy()
   {
      pthread_mutex_lock( &m_mut );
      ++use_count_;
      pthread_mutex_unlock( &m_mut );
   }

   bool add_ref_lock() // true on success
   {
      pthread_mutex_lock( &m_mut );
      bool r = use_count_ == 0? false: ( ++use_count_, true );
      pthread_mutex_unlock( &m_mut );
      return r;
   }

   bool ref_release() // nothrow
   {
      pthread_mutex_lock( &m_mut );
      long new_use_count = --use_count_;
      pthread_mutex_unlock( &m_mut );

      return new_use_count == 0;
   }

   void weak_add_ref() // nothrow
   {
      pthread_mutex_lock( &m_mut );
      ++weak_count_;
      pthread_mutex_unlock( &m_mut );
   }

   bool weak_release() // nothrow
   {
      pthread_mutex_lock( &m_mut );
      long new_weak_count = --weak_count_;
      pthread_mutex_unlock( &m_mut );

      return new_weak_count == 0;
   }

   long use_count() const // nothrow
   {
      pthread_mutex_lock( &m_mut );
      long r = use_count_;
      pthread_mutex_unlock( &m_mut );

      return r;
   }
};

} // namespace detail

} // namespace interprocess

} // namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif  // #ifndef BOOST_INTERPROCESS_DETAIL_SP_COUNTED_BASE_PT_HPP_INCLUDED
