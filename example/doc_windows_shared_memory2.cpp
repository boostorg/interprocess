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
#include <boost/interprocess/detail/workaround.hpp>

#ifdef BOOST_WINDOWS
//[doc_windows_shared_memory2
#include <boost/interprocess/windows_shared_memory.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <iostream>
#include <cstring>

int main ()
{
   using namespace boost::interprocess;
   try{
      //Open already created shared memory object.
      windows_shared_memory shm(open_only, "shared_memory", read_only);

      //Map the whole shared memory in this process
      mapped_region region (shm, read_only);

      //Check that memory was initialized to 1
      const char *mem = static_cast<char*>(region.get_address());
      for(std::size_t i = 0; i < 1000; ++i){
         if(*mem++ != 1){
            std::cout << "Error checking memory!" << std::endl;               
            return 1;
         }
      }

      std::cout << "Test successful!" << std::endl;
   }
   catch(interprocess_exception &ex){
      std::cout << "Unexpected exception: " << ex.what() << std::endl;
      return 1;
   }

   return 0;
}
//]
#else //#ifdef BOOST_WINDOWS
int main()
{  return 0;   }
#endif//#ifdef BOOST_WINDOWS

#include <boost/interprocess/detail/config_end.hpp>

