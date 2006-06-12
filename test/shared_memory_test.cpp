//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2004-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/shared_memory.hpp>
#include <boost/interprocess/exceptions.hpp>
#include "named_creation_template.hpp"
#include <memory.h>  //for memset
#include <string.h>  //for strcmp
#include <iostream>  //for strcmp

static const std::size_t ShmSize = 1000;
static const char *      ShmName = "shared_memory";

struct shared_memory_eraser
{
   ~shared_memory_eraser()
   {
      boost::interprocess::shared_memory_mapping::remove(ShmName);
   }
};

//This wrapper is necessary to have a common constructor
//in generic named_creation_template functions
class shared_memory_creation_test_wrapper
   : public boost::interprocess::shared_memory
{
   public:
   shared_memory_creation_test_wrapper(boost::interprocess::detail::create_only_t)
      :  boost::interprocess::shared_memory
            (boost::interprocess::create_only, ShmName, ShmSize)
   {}

   shared_memory_creation_test_wrapper(boost::interprocess::detail::open_only_t)
      :  boost::interprocess::shared_memory
            (boost::interprocess::open_only, ShmName)
   {}

   shared_memory_creation_test_wrapper(boost::interprocess::detail::open_or_create_t)
      :  boost::interprocess::shared_memory
            (boost::interprocess::open_or_create, ShmName, ShmSize)
   {}
};


int main ()
{
   using namespace boost::interprocess;
   try{
      shared_memory_mapping::remove(ShmName);
      test::test_named_creation<shared_memory_creation_test_wrapper>();

      //Create and get name, size and address
      {  
         shared_memory_mapping::remove(ShmName);
         shared_memory shm1(create_only, ShmName, ShmSize);

         //Compare size
         if(shm1.get_size() != ShmSize)
            return 1;

         //Compare name
         if(strcmp(shm1.get_name(), ShmName) != 0){
            return 1;
         }

         //Overwrite all memory
         memset(shm1.get_address(), 0, ShmSize);
      }
   }
   catch(std::exception &ex){
      std::cout << ex.what() << std::endl;
   }
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>

