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
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include "mutex_test_template.hpp"
#include "named_creation_template.hpp"

struct deleter
{
   ~deleter()
   {  boost::interprocess::named_mutex::remove("named_mutex"); }
};

//This wrapper is necessary to have a default constructor
//in generic mutex_test_template functions
class named_mutex_lock_test_wrapper
   : public boost::interprocess::named_mutex
{
   public:
   named_mutex_lock_test_wrapper()
      :  boost::interprocess::named_mutex
            (boost::interprocess::open_or_create, "named_mutex")
   {}
};

//This wrapper is necessary to have a common constructor
//in generic named_creation_template functions
class named_mutex_creation_test_wrapper
   : public deleter, public boost::interprocess::named_mutex
{
   public:
   named_mutex_creation_test_wrapper(boost::interprocess::detail::create_only_t)
      :  boost::interprocess::named_mutex
            (boost::interprocess::create_only, "named_mutex")
   {}

   named_mutex_creation_test_wrapper(boost::interprocess::detail::open_only_t)
      :  boost::interprocess::named_mutex
            (boost::interprocess::open_only, "named_mutex")
   {}

   named_mutex_creation_test_wrapper(boost::interprocess::detail::open_or_create_t)
      :  boost::interprocess::named_mutex
            (boost::interprocess::open_or_create, "named_mutex")
   {}
};

int main ()
{
   using namespace boost::interprocess;
   try{
      named_mutex::remove("named_mutex");
      test::test_named_creation<named_mutex_creation_test_wrapper>();
      test::test_all_lock<named_mutex_lock_test_wrapper>();
      test::test_all_mutex<false, named_mutex_lock_test_wrapper>();
   }
   catch(std::exception &ex){
      named_mutex::remove("named_mutex");
      std::cout << ex.what() << std::endl;
      return 1;
   }
   named_mutex::remove("named_mutex");
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
