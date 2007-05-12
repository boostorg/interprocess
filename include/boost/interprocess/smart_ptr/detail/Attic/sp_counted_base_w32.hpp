#ifndef BOOST_INTERPROCESS_DETAIL_SP_COUNTED_BASE_W32_HPP_INCLUDED
#define BOOST_INTERPROCESS_DETAIL_SP_COUNTED_BASE_W32_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//
//  This file is the adaptation for shared memory memory mapped
//  files of boost/detail/sp_counted_base_w32.hpp
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
//
//  Lock-free algorithm by Alexander Terekhov
//
//  Thanks to Ben Hitchings for the #weak + (#shared != 0)
//  formulation
//

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/detail/win32_api.hpp>
#include <typeinfo>

namespace boost {

namespace interprocess {

namespace detail {

class sp_counted_base
{
private:

    sp_counted_base( sp_counted_base const & );
    sp_counted_base & operator= ( sp_counted_base const & );

    long use_count_;        // #shared
    long weak_count_;       // #weak + (#shared != 0)

public:

    sp_counted_base(): use_count_( 1 ), weak_count_( 1 )
    {}

    ~sp_counted_base() // nothrow
    {}
/*
    // dispose() is called when use_count_ drops to zero, to release
    // the resources managed by *this.

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
        winapi::interlocked_increment( &use_count_ );
    }

    bool add_ref_lock() // true on success
    {
        for( ;; )
        {
            long tmp = static_cast< long const volatile& >( use_count_ );
            if( tmp == 0 ) return false;
            if( winapi::interlocked_compare_exchange( &use_count_, tmp + 1, tmp ) == tmp ) return true;
        }
    }

    bool ref_release() // nothrow
       { return winapi::interlocked_decrement( &use_count_ ) == 0;  }

/*
    void release() // nothrow
    {
        if(ref_release()){
            //dispose();
            weak_release();
        }
    }
*/
   void weak_add_ref() // nothrow
   { winapi::interlocked_increment( &weak_count_ ); }

   bool weak_release() // nothrow
   { return winapi::interlocked_decrement( &weak_count_ ) == 0; }

   long use_count() const // nothrow
   { return static_cast<long const volatile &>( use_count_ ); }
};

} // namespace detail

} // namespace interprocess

} // namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif  // #ifndef BOOST_INTERPROCESS_DETAIL_SP_COUNTED_BASE_W32_HPP_INCLUDED
