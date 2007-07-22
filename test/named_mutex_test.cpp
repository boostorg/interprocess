//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2004-2007. Distributed under the Boost
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
#include <string>
#include "get_compiler_name.hpp"

using namespace boost::interprocess;

struct mutex_deleter
{
   ~mutex_deleter()
   {  named_mutex::remove(test::get_compiler_name()); }
};

//This wrapper is necessary to have a default constructor
//in generic mutex_test_template functions
class named_mutex_lock_test_wrapper
   : public named_mutex
{
   public:
   named_mutex_lock_test_wrapper()
      :  named_mutex(open_or_create, test::get_compiler_name())
   {}
};

//This wrapper is necessary to have a common constructor
//in generic named_creation_template functions
class named_mutex_creation_test_wrapper
   : public mutex_deleter, public named_mutex
{
   public:
   named_mutex_creation_test_wrapper(create_only_t)
      :  named_mutex(create_only, test::get_compiler_name())
   {}

   named_mutex_creation_test_wrapper(open_only_t)
      :  named_mutex(open_only, test::get_compiler_name())
   {}

   named_mutex_creation_test_wrapper(open_or_create_t)
      :  named_mutex(open_or_create, test::get_compiler_name())
   {}
};

int main ()
{
   try{
      named_mutex::remove(test::get_compiler_name());
      test::test_named_creation<named_mutex_creation_test_wrapper>();
      test::test_all_lock<named_mutex_lock_test_wrapper>();
      test::test_all_mutex<false, named_mutex_lock_test_wrapper>();
   }
   catch(std::exception &ex){
      named_mutex::remove(test::get_compiler_name());
      std::cout << ex.what() << std::endl;
      return 1;
   }
   named_mutex::remove(test::get_compiler_name());
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
