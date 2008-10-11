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
//[doc_offset_ptr
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/offset_ptr.hpp>

using namespace boost::interprocess;

//Shared memory linked list node
struct list_node
{
   offset_ptr<list_node> next;
   int                   value;
};

int main ()
{
   //Destroy any previous shared memory with the name to be used.
   //Create a special shared memory from which we can
   //allocate buffers of raw memory.
   shared_memory_object::remove("MySharedMemory");
   try{
      managed_shared_memory segment(
         create_only, 
         "MySharedMemory", //segment name
         65536);           //segment size in bytes

      //Create linked list with 10 nodes in shared memory
      offset_ptr<list_node> prev = 0, current, first;

      int i;
      for(i = 0; i < 10; ++i, prev = current){
         current = static_cast<list_node*>(segment.allocate(sizeof(list_node)));
         current->value = i;
         current->next  = 0;

         if(!prev)
            first = current;
         else
            prev->next = current;
      }

      //Communicate list to other processes
      //. . .
      //When done, destroy list
      for(current = first; current; /**/){
         prev = current;
         current = current->next;
         segment.deallocate(prev.get());
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
