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
//[doc_file_mapping2
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <iostream>
#include <fstream>
#include <cstddef>
#include <cstdio>    //std::remove
#include <vector>

int main ()
{
   using namespace boost::interprocess;
   try{
      //Open the file mapping
      file_mapping m_file ("file.bin", read_only);

      //Map the whole file in this process
      mapped_region region
         (m_file                    //What to map
         ,read_only  //Map it as read-only
         );

      //Get the address of the mapped region
      void * addr       = region.get_address();
      std::size_t size  = region.get_size();

      //Check that memory was initialized to 1
      const char *mem = static_cast<char*>(addr);
      for(std::size_t i = 0; i < size; ++i){
         if(*mem++ != 1){
            std::cout << "Error checking memory!" << std::endl;
            return 1;
         }
      }

      //Now test it reading the file
      std::filebuf fbuf;
      fbuf.open("file.bin", std::ios_base::in | std::ios_base::binary); 

      //Read it to memory
      std::vector<char> vect(region.get_size(), 0);
      fbuf.sgetn(&vect[0], std::streamsize(vect.size()));

      //Check that memory was initialized to 1
      mem = static_cast<char*>(&vect[0]);
      for(std::size_t i = 0; i < size; ++i){
         if(*mem++ != 1){
            std::cout << "Error checking memory!" << std::endl;
            return 1;
         }
      }

      std::cout << "Test successful!" << std::endl;
   }
   catch(interprocess_exception &ex){
      std::remove("file.bin");
      std::cout << "Unexpected exception: " << ex.what() << std::endl;
      return 1;
   }
   std::remove("file.bin");
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
