//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <cstddef>

int main ()
{
   using namespace boost::interprocess;

   shared_memory_object::remove("MySharedMemory");

   //Create managed shared memory
   managed_shared_memory segment(create_only, 
                               "MySharedMemory",//segment name
                               65536);           //segment size in bytes;

   //Allocate a portion of the segment
   void * shptr   = segment.allocate(1024);
   managed_shared_memory::handle_t handle = segment.get_handle_from_address(shptr);
   (void)handle;

   // Copy message to buffer
   // . . .
   // Send handle to other process
   // . . .
   // Wait response from other process 
   // . . .
   
   {
      using namespace boost::interprocess;

      //Named allocate capable shared memory allocator
      managed_shared_memory segment(open_only, "MySharedMemory");

      managed_shared_memory::handle_t handle = 0;
      (void)handle;

      //Wait handle msg from other process and put it in
      //"handle" local variable

      //Get buffer local address from handle
      void *msg = segment.get_address_from_handle(handle);
      (void)msg;
    
      //Do anything with msg
      //. . .
      //Send ack to sender process
   }

   segment.deallocate(shptr);
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
