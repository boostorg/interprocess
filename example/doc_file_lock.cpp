//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2026. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

//[doc_file_lock
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <fstream>
#include <iostream>
#include <cstdio>
//<-
#include <string>
#include "../test/get_process_id_name.hpp"
//->

int main ()
{
   using namespace boost::interprocess;
   BOOST_INTERPROCESS_TRY{
      //<-
      #if 1
      std::string file_name(get_filename());
      const char *FileName = file_name.c_str();
      #else
      //->
      const char *FileName = "my_file";
      //<-
      #endif
      //->

      struct file_remove
      {
         file_remove(const char *name)
            : name_(name) { std::remove(name_); }
         ~file_remove(){ std::remove(name_); }
         const char *name_;
      } remover(FileName);
      //<-
      (void)remover;
      //->

      //file_lock needs an existing file
      {
         std::ofstream file(FileName);
         if(!file){
            return 1;
         }
      }

      file_lock f_lock(FileName);

      {
         //Takes exclusive ownership. Released in the destructor
         scoped_lock<file_lock> e_lock(f_lock);
         if(!e_lock){
            return 1;
         }
      }

      {
         scoped_lock<file_lock> e_lock(f_lock, try_to_lock);
         if(!e_lock){
            return 1;
         }
      }
   }
   BOOST_INTERPROCESS_CATCH(interprocess_exception &ex){
      std::cout << ex.what() << std::endl;
      return 1;
   } BOOST_INTERPROCESS_CATCH_END
   return 0;
}
//]
