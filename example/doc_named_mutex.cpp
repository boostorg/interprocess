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
//[doc_named_mutex
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <fstream>
#include <iostream>

int main ()
{
   using namespace boost::interprocess;
   try{
      
      //Open or create the named mutex
      named_mutex mutex(open_or_create, "fstream_named_mutex");

      std::ofstream file("file_name");

      for(int i = 0; i < 10; ++i){
         
         //Do some operations...

         //Write to file atomically
         scoped_lock<named_mutex> lock(mutex);
         file << "Process name, ";
         file << "This is iteration #" << i;
         file << std::endl;
      }
   }
   catch(interprocess_exception &ex){
      named_mutex::remove("fstream_named_mutex");
      std::cout << ex.what() << std::endl;
      return 1;
   }
   named_mutex::remove("fstream_named_mutex");
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
