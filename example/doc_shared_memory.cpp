//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/interprocess/detail/config_begin.hpp>
//[doc_shared_memory
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <iostream>
#include <cstring>

int main ()
{
   using namespace boost::interprocess;
   try{
      //Erase previous shared memory
      shared_memory_object::remove("shared_memory");

      //Create a shared memory object.
      shared_memory_object shm (create_only, "shared_memory", read_write);

      //Set size
      shm.truncate(1000);

      //Map the whole shared memory in this process
      mapped_region region(shm, read_write);

      //Write all the memory to 1
      std::memset(region.get_address(), 1, region.get_size());
   }
   catch(interprocess_exception &ex){
      shared_memory_object::remove("shared_memory");
      std::cout << ex.what() << std::endl;
      return 1;
   }
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
