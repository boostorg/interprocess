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
//[doc_shared_memory2
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <iostream>
#include <cstring>

int main ()
{
   using namespace boost::interprocess;
   try{
      //Open already created shared memory object.
      shared_memory_object shm (open_only, "shared_memory", read_only);

      //Map the whole shared memory in this process
      mapped_region region(shm, read_only);

      //Check that memory was initialized to 1
      const char *mem = static_cast<char*>(region.get_address());
      for(std::size_t i = 0; i < region.get_size(); ++i){
         if(*mem++ != 1){
            std::cout << "Error checking memory!" << std::endl;               
            return 1;
         }
      }
      std::cout << "Test successful!" << std::endl;
   }
   catch(interprocess_exception &ex){
      std::cout << "Unexpected exception: " << ex.what() << std::endl;
      shared_memory_object::remove("shared_memory");
      return 1;
   }
   shared_memory_object::remove("shared_memory");
   return 0;
}
//]

#include <boost/interprocess/detail/config_end.hpp>
