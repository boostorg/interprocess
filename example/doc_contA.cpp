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
//[doc_contA
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

int main ()
{
   using namespace boost::interprocess;
   try{
      //Shared memory front-end that is able to construct objects
      //associated with a c-string. Erase previous shared memory with the name
      //to be used and create the memory segment at the specified address and initialize resources
      shared_memory_object::remove("MySharedMemory");
      managed_shared_memory segment
         (create_only 
         ,"MySharedMemory" //segment name
         ,65536);          //segment size in bytes

      //Alias an STL compatible allocator of ints that allocates ints from the managed
      //shared memory segment.  This allocator will allow to place containers
      //in managed shared memory segments
      typedef allocator<int, managed_shared_memory::segment_manager> 
         ShmemAllocator;

      //Alias a vector that uses the previous STL-like allocator
      typedef vector<int, ShmemAllocator> MyVector;

      //Initialize shared memory STL-compatible allocator
      const ShmemAllocator alloc_inst (segment.get_segment_manager());

      //Construct a shared memory
      MyVector *myvector = 
         segment.construct<MyVector>("MyVector") //object name
                                    (alloc_inst);//first ctor parameter

      //Insert data in the vector
      for(int i = 0; i < 100; ++i){
         myvector->push_back(i);
      }
   }
   catch(...){
      shared_memory_object::remove("MySharedMemory");
      throw;
   }
   shared_memory_object::remove("MySharedMemory");
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
