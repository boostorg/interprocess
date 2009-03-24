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
   //Remove shared memory on construction and destruction
   struct shm_destroy
   {
      shm_destroy() { shared_memory_object::remove("MySharedMemory"); }
      ~shm_destroy(){ shared_memory_object::remove("MySharedMemory"); }
   } remover;

   {
      //Create a managed shared memory
      managed_shared_memory shm(create_only, "MySharedMemory", 1000);
      //Check size
      assert(shm.get_size() == 1000);
      //Construct a named object
      MyClass *myclass = shm.construct<MyClass>("MyClass")();
      //The managed segment is unmapped here
      //<-
      (void)myclass;
      //->
   }
   {
      //Now that the segment is not mapped grow it adding extra 500 bytes
      managed_shared_memory::grow("MySharedMemory", 500);
      //Map it again
      managed_shared_memory shm(open_only, "MySharedMemory");
      //Check size
      assert(shm.get_size() == 1500);
      //Check "MyClass" is still there
      MyClass *myclass = shm.find<MyClass>("MyClass").first;
      assert(myclass != 0);
      //The managed segment is unmapped here
   }
   {
      //Now minimize the size of the segment
      managed_shared_memory::shrink_to_fit("MySharedMemory");
      //Map it again
      managed_shared_memory shm(open_only, "MySharedMemory");
      //Check size
      assert(shm.get_size() < 1000);
      //Check "MyClass" is still there
      MyClass *myclass = shm.find<MyClass>("MyClass").first;
      assert(myclass != 0);
      //The managed segment is unmapped here
   }
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
