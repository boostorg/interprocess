//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2004-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/mapped_file.hpp>
#include "named_creation_template.hpp"
#include <cstdio>
#include <cstring>

static const std::size_t FileSize = 1000;
static const char *      FileName = "mapped_file";

struct file_destroyer
{
   ~file_destroyer()
   {
      //The last destructor will destroy the file
      std::remove(FileName);  
   }
};

//This wrapper is necessary to have a common constructor
//in generic named_creation_template functions
class mapped_file_creation_test_wrapper
   : public file_destroyer, public boost::interprocess::mapped_file
{
   public:
   mapped_file_creation_test_wrapper(boost::interprocess::detail::create_only_t)
      :  boost::interprocess::mapped_file
            (boost::interprocess::create_only, FileName, FileSize)
   {}

   mapped_file_creation_test_wrapper(boost::interprocess::detail::open_only_t)
      :  boost::interprocess::mapped_file
            (boost::interprocess::open_only, FileName)
   {}

   mapped_file_creation_test_wrapper(boost::interprocess::detail::open_or_create_t)
      :  boost::interprocess::mapped_file
            (boost::interprocess::open_or_create, FileName, FileSize)
   {}
};

int main ()
{
   using namespace boost::interprocess;
   std::remove(FileName);
   test::test_named_creation<mapped_file_creation_test_wrapper>();

   //Create and get name, size and address
   {  
      mapped_file file1(create_only, FileName, FileSize);

      //Compare size
      if(file1.get_size() != FileSize)
         return 1;

      //Compare name
      if(std::strcmp(file1.get_name(), FileName) != 0){
         return 1;
      }

      //Overwrite all memory
      std::memset(file1.get_address(), 0, FileSize);
   }
   std::remove(FileName);
   return 0;
}
