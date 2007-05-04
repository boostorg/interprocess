//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2004-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/sync/named_recursive_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "mutex_test_template.hpp"
#include "named_creation_template.hpp"

struct deleter
{
   ~deleter()
   {  boost::interprocess::named_recursive_mutex::remove("named_recursive_mutex"); }
};

//This wrapper is necessary to have a default constructor
//in generic mutex_test_template functions
class named_recursive_mutex_lock_test_wrapper
   : public boost::interprocess::named_recursive_mutex
{
   public:
   named_recursive_mutex_lock_test_wrapper()
      :  boost::interprocess::named_recursive_mutex
            (boost::interprocess::open_or_create, "named_recursive_mutex")
   {}
};

//This wrapper is necessary to have a common constructor
//in generic named_creation_template functions
class named_mutex_creation_test_wrapper
   : public deleter, public boost::interprocess::named_recursive_mutex
{
   public:
   named_mutex_creation_test_wrapper(boost::interprocess::detail::create_only_t)
      :  boost::interprocess::named_recursive_mutex
            (boost::interprocess::create_only, "named_recursive_mutex")
   {}

   named_mutex_creation_test_wrapper(boost::interprocess::detail::open_only_t)
      :  boost::interprocess::named_recursive_mutex
            (boost::interprocess::open_only, "named_recursive_mutex")
   {}

   named_mutex_creation_test_wrapper(boost::interprocess::detail::open_or_create_t)
      :  boost::interprocess::named_recursive_mutex
            (boost::interprocess::open_or_create, "named_recursive_mutex")
   {}
};

int main ()
{
   using namespace boost::interprocess;
   try{
      named_recursive_mutex::remove("named_recursive_mutex");
      test::test_named_creation<named_mutex_creation_test_wrapper>();
      test::test_all_lock<named_recursive_mutex_lock_test_wrapper>();
      test::test_all_mutex<false, named_recursive_mutex_lock_test_wrapper>();
      test::test_all_recursive_lock<named_recursive_mutex_lock_test_wrapper>();
   }
   catch(std::exception &ex){
      named_recursive_mutex::remove("named_recursive_mutex");
      std::cout << ex.what() << std::endl;
      return 1;
   }
   named_recursive_mutex::remove("named_recursive_mutex");
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>

