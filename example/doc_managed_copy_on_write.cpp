//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2008. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/interprocess/detail/config_begin.hpp>
//[doc_managed_copy_on_write
#include <boost/interprocess/managed_mapped_file.hpp>
#include <fstream> //std::fstream
#include <cstdio>  //std::remove

int main()
{
   using namespace boost::interprocess;

   //Try to erase any previous managed segment with the same name
   std::remove("MyManagedFile");
   std::remove("MyManagedFile2");

   try{
      //Create an named integer in a managed mapped file
      managed_mapped_file managed_file(create_only, "MyManagedFile", 65536);
      managed_file.construct<int>("MyInt")(0u);

      //Now create a copy on write version
      managed_mapped_file managed_file_cow(open_copy_on_write, "MyManagedFile");

      //Erase the int and create a new one
      if(!managed_file_cow.destroy<int>("MyInt"))
         throw int(0);
      managed_file_cow.construct<int>("MyInt2");

      //Check changes
      if(managed_file_cow.find<int>("MyInt").first && !managed_file_cow.find<int>("MyInt2").first)
         throw int(0);

      //Check the original is intact
      if(!managed_file.find<int>("MyInt").first && managed_file.find<int>("MyInt2").first)
         throw int(0);

      {  //Dump the modified copy on write segment to a file
         std::fstream file("MyManagedFile2", std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
         if(!file)
            throw int(0);
         file.write((const char *)managed_file_cow.get_address(), managed_file_cow.get_size());
      }

      //Now open the modified file and test changes
      managed_mapped_file managed_file_cow2(open_only, "MyManagedFile2");
      if(managed_file_cow2.find<int>("MyInt").first && !managed_file_cow2.find<int>("MyInt2").first)
         throw int(0);
   }
   catch(...){
      std::remove("MyManagedFile");
      std::remove("MyManagedFile2");
      throw;
   }
   std::remove("MyManagedFile");
   std::remove("MyManagedFile2");
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
