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
//[doc_file_mapping
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstddef>
#include <cstdio>    //std::remove

int main ()
{
   using namespace boost::interprocess;
   try{
      //Create a file
      std::filebuf fbuf;
      fbuf.open("file.bin", std::ios_base::in | std::ios_base::out 
                           | std::ios_base::trunc | std::ios_base::binary); 

      //Set the size
      fbuf.pubseekoff(9999, std::ios_base::beg);
      fbuf.sputc(0);
      fbuf.close();

      //Create a file mapping.
      file_mapping m_file("file.bin", read_write);

      //Map the whole file in this process
      mapped_region region
         (m_file                    //What to map
         ,read_write //Map it as read-write
         );

      if(region.get_size() != 10000)
         return 1;

      //Get the address of the mapped region
      void * addr       = region.get_address();
      std::size_t size  = region.get_size();

      //Write all the memory to 1
      std::memset(addr, 1, size);

   }
   catch(interprocess_exception &ex){
      std::remove("file.bin");
      std::cout << ex.what() << std::endl;
      return 1;
   }
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
