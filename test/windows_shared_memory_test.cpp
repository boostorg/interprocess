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
#include <boost/interprocess/detail/workaround.hpp>

#ifdef BOOST_WINDOWS

#include <boost/interprocess/windows_shared_memory.hpp>
#include <boost/interprocess/detail/managed_open_or_create_impl.hpp>
#include <boost/interprocess/exceptions.hpp>
#include "named_creation_template.hpp"
#include <cstring>   //for strcmp, memset
#include <iostream>  //for cout

static const std::size_t ShmSize = 1000;
static const char *      ShmName = "windows_shared_memory";

//This wrapper is necessary to have a common constructor
//in generic named_creation_template functions
class shared_memory_creation_test_wrapper
   : public boost::interprocess::detail::managed_open_or_create_impl
      <boost::interprocess::windows_shared_memory, false>
{
   typedef boost::interprocess::detail::managed_open_or_create_impl
      <boost::interprocess::windows_shared_memory, false> windows_shared_memory;

   public:
   shared_memory_creation_test_wrapper(boost::interprocess::detail::create_only_t)
      :  windows_shared_memory(boost::interprocess::create_only, ShmName, ShmSize)
   {}

   shared_memory_creation_test_wrapper(boost::interprocess::detail::open_only_t)
      :  windows_shared_memory(boost::interprocess::open_only, ShmName)
   {}

   shared_memory_creation_test_wrapper(boost::interprocess::detail::open_or_create_t)
      :  windows_shared_memory(boost::interprocess::open_or_create, ShmName, ShmSize)
   {}
};


int main ()
{
   using namespace boost::interprocess;
   typedef detail::managed_open_or_create_impl<windows_shared_memory, false> windows_shared_memory;

   try{
      test::test_named_creation<shared_memory_creation_test_wrapper>();
/*
      //Create and get name, size and address
      {  
         windows_shared_memory shm1(create_only, ShmName, ShmSize);

         //Compare name
         if(std::strcmp(shm1.get_name(), ShmName) != 0){
            return 1;
         }

         //Overwrite all memory
         std::memset(shm1.get_address(), 0, shm1.get_size());
      }*/
   }
   catch(std::exception &ex){
      std::cout << ex.what() << std::endl;
   }
   return 0;
}

#else

int main()
{
   return 0;
}

#endif   //#ifdef BOOST_WINDOWS

#include <boost/interprocess/detail/config_end.hpp>
