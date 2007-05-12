#ifndef BOOST_INTERPROCESS_DETAIL_SP_COUNTED_BASE_HPP_INCLUDED
#define BOOST_INTERPROCESS_DETAIL_SP_COUNTED_BASE_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//  This file is the adaptation for shared memory memory mapped
//  files of boost/detail/sp_counted_base.hpp
//
//  Copyright 2005 Peter Dimov
//  Copyright 2006 Ion Gaztanaga
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#if defined( WIN32 ) || defined( _WIN32 ) || defined( __WIN32__ )

# include <boost/interprocess/smart_ptr/detail/sp_counted_base_w32.hpp>

#elif defined( BOOST_HAS_PTHREADS ) && defined BOOST_INTERPROCESS_POSIX_PROCESS_SHARED

//Ordinary pthreads counted base is not enough
//we need process shared attributte.
# include <boost/interprocess/smart_ptr/detail/sp_counted_base_pt.hpp>

#else

# include <boost/interprocess/smart_ptr/detail/sp_counted_base_atomic.hpp>

#endif

#include <boost/interprocess/detail/config_end.hpp>

#endif  // #ifndef BOOST_INTERPROCESS_DETAIL_SP_COUNTED_BASE_HPP_INCLUDED

