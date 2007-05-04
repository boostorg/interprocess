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
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/lexical_cast.hpp>
#include "condition_test_template.hpp"
#include "named_creation_template.hpp"

struct condition_deleter
{
   std::string name;

   ~condition_deleter()
   {  
      if(name.empty())
         boost::interprocess::named_condition::remove("named_condition");
      else
         boost::interprocess::named_condition::remove(name.c_str()); 
   }
};

//This wrapper is necessary to have a default constructor
//in generic mutex_test_template functions
class named_condition_test_wrapper
   : public condition_deleter, public boost::interprocess::named_condition
{
   public:

   named_condition_test_wrapper()
      :  boost::interprocess::named_condition
            (boost::interprocess::open_or_create, 
             ("test_cond" + boost::lexical_cast<std::string>(count)).c_str())
   {
      condition_deleter::name += "test_cond";
      condition_deleter::name += boost::lexical_cast<std::string>(count);
      ++count;
   }

   ~named_condition_test_wrapper()
   {  --count; }

   static int count;
};

int named_condition_test_wrapper::count = 0;

//This wrapper is necessary to have a common constructor
//in generic named_creation_template functions
class named_condition_creation_test_wrapper
   : public condition_deleter, public boost::interprocess::named_condition
{
   public:
   named_condition_creation_test_wrapper(boost::interprocess::detail::create_only_t)
      :  boost::interprocess::named_condition
            (boost::interprocess::create_only, "named_condition")
   {}

   named_condition_creation_test_wrapper(boost::interprocess::detail::open_only_t)
      :  boost::interprocess::named_condition
            (boost::interprocess::open_only, "named_condition")
   {}

   named_condition_creation_test_wrapper(boost::interprocess::detail::open_or_create_t)
      :  boost::interprocess::named_condition
            (boost::interprocess::open_or_create, "named_condition")
   {}
};

struct mutex_deleter
{
   std::string name;

   ~mutex_deleter()
   {  
      if(name.empty())
         boost::interprocess::named_mutex::remove("named_mutex");
      else
         boost::interprocess::named_mutex::remove(name.c_str()); 
   }
};

//This wrapper is necessary to have a default constructor
//in generic mutex_test_template functions
class named_mutex_test_wrapper
   : public mutex_deleter, public boost::interprocess::named_mutex
{
   public:
   named_mutex_test_wrapper()
      :  boost::interprocess::named_mutex
            (boost::interprocess::open_or_create, 
             ("test_mutex" + boost::lexical_cast<std::string>(count)).c_str())
   {
      mutex_deleter::name += "test_mutex";
      mutex_deleter::name += boost::lexical_cast<std::string>(count);
      ++count;
   }

   ~named_mutex_test_wrapper()
   {  --count; }

   static int count;
};

int named_mutex_test_wrapper::count = 0;

int main ()
{
   using namespace boost::interprocess;
   try{
      //Remove previous mutexes and conditions
      named_mutex::remove("test_mutex0");
      named_condition::remove("test_cond0");
      named_condition::remove("test_cond1");
      named_condition::remove("named_condition");
      named_mutex::remove("named_mutex");

      test::test_named_creation<named_condition_creation_test_wrapper>();
      test::do_test_condition<named_condition_test_wrapper
                             ,named_mutex_test_wrapper>();
   }
   catch(std::exception &ex){
      std::cout << ex.what() << std::endl;
      return 1;
   }
   named_mutex::remove("test_mutex0");
   named_condition::remove("test_cond0");
   named_condition::remove("test_cond1");
   named_condition::remove("named_condition");
   named_mutex::remove("named_mutex");
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
