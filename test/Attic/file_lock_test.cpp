//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gazta�ga 2004-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "mutex_test_template.hpp"
#include "sharable_mutex_test_template.hpp"
#include <fstream>
#include <cstdio>


//This wrapper is necessary to have a default constructor
//in generic mutex_test_template functions
class file_lock_lock_test_wrapper
   : public boost::interprocess::file_lock
{
   public:
   file_lock_lock_test_wrapper()
      :  boost::interprocess::file_lock("file_lock")
   {}
};

int main ()
{
   using namespace boost::interprocess;
   //Destroy and create file
   {
      std::remove("file_lock");
      std::ofstream file("file_lock");
      if(!file){
         return 1;
      }
   }

   test::test_all_lock<file_lock_lock_test_wrapper>();
   test::test_all_mutex<false, file_lock_lock_test_wrapper>();
   test::test_all_sharable_mutex<false, file_lock_lock_test_wrapper>();
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
