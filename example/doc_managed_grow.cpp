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
//[doc_managed_grow
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <cassert>

class MyClass
{
   //...
};

int main()
{
   using namespace boost::interprocess;
   try{
      {  //Remove old shared memory if present
         shared_memory_object::remove("MyManagedShm");
         //Create a managed shared memory
         managed_shared_memory shm(create_only, "MyManagedShm", 1000);
         //Check size
         assert(shm.get_size() == 1000);
         //Construct a named object
         MyClass *myclass = shm.construct<MyClass>("MyClass")();
         //The managed segment is unmapped here
      }
      {
         //Now that the segment is not mapped grow it adding extra 500 bytes
         managed_shared_memory::grow("MyManagedShm", 500);
         //Map it again
         managed_shared_memory shm(open_only, "MyManagedShm");
         //Check size
         assert(shm.get_size() == 1500);
         //Check "MyClass" is still there
         MyClass *myclass = shm.find<MyClass>("MyClass").first;
         assert(myclass != 0);
         //The managed segment is unmapped here
      }
      {
         //Now minimize the size of the segment
         managed_shared_memory::shrink_to_fit("MyManagedShm");
         //Map it again
         managed_shared_memory shm(open_only, "MyManagedShm");
         //Check size
         assert(shm.get_size() < 1000);
         //Check "MyClass" is still there
         MyClass *myclass = shm.find<MyClass>("MyClass").first;
         assert(myclass != 0);
         //The managed segment is unmapped here
      }
   }
   catch(...){
      shared_memory_object::remove("MyManagedShm");
      throw;
   }
   //Remove the managed segment
   shared_memory_object::remove("MyManagedShm");
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
