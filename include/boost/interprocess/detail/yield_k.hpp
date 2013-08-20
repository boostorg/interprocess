//This file was copied from boost/smart_ptr/detail and
//modified here to avoid dependencies with that library
#ifndef BOOST_INTERPROCESS_DETAIL_YIELD_K_HPP_INCLUDED
#define BOOST_INTERPROCESS_DETAIL_YIELD_K_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//
//  yield_k.hpp
//
//  Copyright (c) 2008 Peter Dimov
//
//  void yield( unsigned k );
//
//  Typical use:
//
//  for( unsigned k = 0; !try_lock(); ++k ) yield( k );
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

// BOOST_INTERPROCESS_SMT_PAUSE

#if defined(_MSC_VER) && _MSC_VER >= 1310 && ( defined(_M_IX86) || defined(_M_X64) )

extern "C" void _mm_pause();
#pragma intrinsic( _mm_pause )

#define BOOST_INTERPROCESS_SMT_PAUSE _mm_pause();

#elif defined(__GNUC__) && ( defined(__i386__) || defined(__x86_64__) )

#define BOOST_INTERPROCESS_SMT_PAUSE __asm__ __volatile__( "rep; nop" : : : "memory" );

#endif

//

#if defined (BOOST_INTERPROCESS_WINDOWS)

#include <boost/interprocess/detail/win32_api.hpp>

namespace boost
{
namespace interprocess
{
namespace ipcdetail
{

inline void yield( unsigned k )
{
   if( k < 4 )
   {
   }
#if defined( BOOST_INTERPROCESS_SMT_PAUSE )
   else if( k < 16 ){
      BOOST_INTERPROCESS_SMT_PAUSE
   }
#endif
   else if( k < 32 ){
      //Try to yield to another thread running on the current processor
      if(!winapi::SwitchToThread()){
         //If not yield to any thread of same or higher priority on any processor
         boost::interprocess::winapi::Sleep(0);
      }
   }
   else{
      //Yields to any thread on any processor
      boost::interprocess::winapi::Sleep(1);
   }
}

} // namespace ipcdetail
} // namespace interprocess
} // namespace boost

#else

#include <sched.h>
#include <time.h>

namespace boost
{
namespace interprocess
{
namespace ipcdetail
{

inline void yield( unsigned k )
{
    if( k < 4 )
    {
    }
#if defined( BOOST_INTERPROCESS_SMT_PAUSE )
    else if( k < 16 )
    {
        BOOST_INTERPROCESS_SMT_PAUSE
    }
#endif
    else if( k < 32 || k & 1 )
    {
        sched_yield();
    }
    else
    {
        // g++ -Wextra warns on {} or {0}
        struct timespec rqtp = { 0, 0 };

        // POSIX says that timespec has tv_sec and tv_nsec
        // But it doesn't guarantee order or placement

        rqtp.tv_sec = 0;
        rqtp.tv_nsec = 1000;

        nanosleep( &rqtp, 0 );
    }
}

} // namespace ipcdetail
} // namespace interprocess
} // namespace boost

#endif

#include <boost/interprocess/detail/config_end.hpp>

#endif // #ifndef BOOST_INTERPROCESS_DETAIL_YIELD_K_HPP_INCLUDED
