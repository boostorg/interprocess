#ifndef BOOST_INTERPROCESS_DETAIL_SP_COUNTED_BASE_ATOMIC_HPP_INCLUDED
#define BOOST_INTERPROCESS_DETAIL_SP_COUNTED_BASE_ATOMIC_HPP_INCLUDED

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
# pragma once
#endif

//  Copyright (c) 2001, 2002, 2003 Peter Dimov and Multi Media Ltd.
//  Copyright 2004-2005 Peter Dimov
//  Copyright 2007-2012 Ion Gaztanaga
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//
//  Lock-free algorithm by Alexander Terekhov
//
//  Thanks to Ben Hitchings for the #weak + (#shared != 0)
//  formulation
//

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/detail/atomic.hpp>
#include <typeinfo>

namespace boost {

namespace interprocess {

namespace ipcdetail {

class sp_counted_base
{
private:

    sp_counted_base( sp_counted_base const & );
    sp_counted_base & operator= ( sp_counted_base const & );

    boost::uint32_t use_count_;        // #shared
    boost::uint32_t weak_count_;       // #weak + (#shared != 0)

public:

    sp_counted_base(): use_count_( 1 ), weak_count_( 1 )
    {}

    ~sp_counted_base() // nothrow
    {}

    //Taking one more reference publishes nothing and reads nothing that has to
    //be ordered: the caller already owns a reference, so the object can't go
    //away meanwhile. A relaxed increment is enough
    void add_ref_copy()
    {
        ipcdetail::atomic_inc32_relaxed( &use_count_ );
    }

    bool add_ref_lock() // true on success
    {
        for( ;; )
        {
            boost::uint32_t tmp = static_cast< boost::uint32_t const volatile& >( use_count_ );
            if( tmp == 0 ) return false;
            //On success this thread starts using the object, so it must acquire
            //what the thread that created it released. Nothing is published
            //here, and a failed swap only leads to another round of the loop
            if( ipcdetail::atomic_cas32_acquire( &use_count_, tmp + 1, tmp ) == tmp )
               return true;
        }
    }

   //Dropping a reference must be a release, so that the thread that destroys
   //the object sees the work of all the threads that dropped theirs before,
   //and an acquire for the thread that finds the last one, so that it sees
   //that work before destroying. That is exactly the read-modify-write order
   //of atomic_dec32: a plain release would leave the destroying thread without
   //the acquire and race with the others
   bool ref_release() // nothrow
   { return 1 == ipcdetail::atomic_dec32( &use_count_ );  }

   void weak_add_ref() // nothrow
   { ipcdetail::atomic_inc32_relaxed( &weak_count_ ); }

   //Same reasoning as ref_release
   bool weak_release() // nothrow
   { return 1 == ipcdetail::atomic_dec32( &weak_count_ ); }

   long use_count() const // nothrow
   { return (long)static_cast<boost::uint32_t const volatile &>( use_count_ ); }
};

} // namespace ipcdetail

} // namespace interprocess

} // namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif  // #ifndef BOOST_INTERPROCESS_DETAIL_SP_COUNTED_BASE_ATOMIC_HPP_INCLUDED
