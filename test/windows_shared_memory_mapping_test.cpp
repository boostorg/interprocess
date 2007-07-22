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

#ifdef BOOST_WINDOWS

#include <fstream>
#include <iostream>
#include <boost/interprocess/windows_shared_memory.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <string>
#include "get_compiler_name.hpp"

using namespace boost::interprocess;

int main ()
{
   std::string compiler_name;
   test::get_compiler_name(compiler_name);

   try{
      const std::size_t FileSize = 99999*2;
      //Create shared memory and file mapping
      windows_shared_memory mapping(create_only, compiler_name.c_str(), read_write, FileSize);

      {
         //Create a file mapping
         windows_shared_memory mapping(open_only, compiler_name.c_str(), read_write);

         //Create two mapped regions, one half of the file each
         mapped_region region (mapping
                              ,read_write
                              ,0
                              ,FileSize/2
                              ,0);

         mapped_region region2(mapping
                              ,read_write
                              ,FileSize/2
                              ,FileSize - FileSize/2
                              ,0);

         //Fill two regions with a pattern   
         unsigned char *filler = static_cast<unsigned char*>(region.get_address());
         for(std::size_t i = 0
            ;i < FileSize/2
            ;++i){
            *filler++ = static_cast<unsigned char>(i);
         }

         filler = static_cast<unsigned char*>(region2.get_address());
         for(std::size_t i = FileSize/2
            ;i < FileSize
            ;++i){
            *filler++ = static_cast<unsigned char>(i);
         }
      }

      //See if the pattern is correct in the file using two mapped regions
      {
         //Create a file mapping
         windows_shared_memory mapping(open_only, compiler_name.c_str(), read_write);
         mapped_region region(mapping, read_write, 0, FileSize/2, 0);
         mapped_region region2(mapping, read_write, FileSize/2, 0/*FileSize - FileSize/2*/, 0);

         unsigned char *checker = (unsigned char*)region.get_address();
         //Check pattern
         for(std::size_t i = 0
            ;i < FileSize/2
            ;++i){
            if(*checker++ != static_cast<unsigned char>(i)){
               return 1;
            }
         }

         //Check second half
         checker = (unsigned char *)region2.get_address();

         //Check pattern
         for(std::size_t i = FileSize/2
            ;i < FileSize
            ;++i){
            if(*checker++ != static_cast<unsigned char>(i)){
               return 1;
            }
         }
      }

      //Now check the pattern mapping a single read only mapped_region
      {
         //Create a file mapping
         windows_shared_memory mapping(open_only, compiler_name.c_str(), read_only);

         //Create a single regions, mapping all the file
         mapped_region region (mapping
                              ,read_only);

         //Check pattern
         unsigned char *pattern = static_cast<unsigned char*>(region.get_address());
         for(std::size_t i = 0
            ;i < FileSize
            ;++i, ++pattern){
            if(*pattern != static_cast<unsigned char>(i)){
               return 1;
            }
         }
      }
   }
   catch(std::exception &exc){
      std::cout << "Unhandled exception: " << exc.what() << std::endl;
   }

   return 0;
}

#else

int main()
{
   return 0;
}

#endif

#include <boost/interprocess/detail/config_end.hpp>
