// Copyright (C) 2001-2003
// William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.
//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/thread/detail/config.hpp>
#include <boost/thread/thread.hpp>
#include <boost/interprocess/sync/interprocess_barrier.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

namespace {

// Shared variables for generation barrier test
const int N_THREADS=10;
boost::interprocess::barrier gen_barrier(N_THREADS);
boost::interprocess::interprocess_mutex interprocess_mutex;
long global_parameter;

void barrier_thread()
{
    for (int i = 0; i < 5; ++i)
    {
        if (gen_barrier.wait())
        {
            boost::interprocess::scoped_lock
               <boost::interprocess::interprocess_mutex> lock(interprocess_mutex);
            global_parameter++;
        }
    }
}

} // namespace

int main ()
{
   boost::thread_group g;
   global_parameter = 0;

   for (int i = 0; i < N_THREADS; ++i)
      g.create_thread(&barrier_thread);

   g.join_all();
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
