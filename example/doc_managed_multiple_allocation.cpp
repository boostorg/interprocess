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
//[doc_managed_multiple_allocation
#include <boost/interprocess/managed_shared_memory.hpp>
#include <cassert>//assert
#include <cstring>//std::memset
#include <new>    //std::nothrow
#include <vector> //std::vector

int main()
{
   using namespace boost::interprocess;
   typedef managed_shared_memory::multiallocation_iterator multiallocation_iterator;

   //Try to erase any previous managed segment with the same name
   shared_memory_object::remove("MyManagedShm");

   try{
      managed_shared_memory managed_shm(create_only, "MyManagedShm", 65536);

      //Allocate 16 elements of 100 bytes in a single call. Non-throwing version.
      multiallocation_iterator beg_it = managed_shm.allocate_many(100, 16, std::nothrow);

      //To check for an error, we can use a boolean expression
      //or compare it with a default constructed iterator
      assert(!beg_it == (beg_it == multiallocation_iterator()));
      
      //Check if the memory allocation was successful
      if(!beg_it)  return 1;

      //Allocated buffers
      std::vector<char*> allocated_buffers;

      //Initialize our data
      for( multiallocation_iterator it = beg_it, end_it; it != end_it; ){
         allocated_buffers.push_back(&*it);
         //The iterator must be incremented before overwriting memory
         //because otherwise, the iterator is invalidated.
         std::memset(&*it++, 0, 100);
      }

      //Now deallocate
      while(!allocated_buffers.empty()){
         managed_shm.deallocate(allocated_buffers.back());
         allocated_buffers.pop_back();
      }

      //Allocate 10 buffers of different sizes in a single call. Throwing version
      std::size_t sizes[10];
      for(std::size_t i = 0; i < 10; ++i)
         sizes[i] = i*3;

      beg_it  = managed_shm.allocate_many(sizes, 10);

      //Iterate each allocated buffer and deallocate
      //The "end" condition can be also checked with operator!
      for(multiallocation_iterator it = beg_it; it;){
         //The iterator must be incremented before overwriting memory
         //because otherwise, the iterator is invalidated.
         managed_shm.deallocate(&*it++);
      }
   }
   catch(...){
      shared_memory_object::remove("MyManagedShm");
      throw;
   }
   shared_memory_object::remove("MyManagedShm");
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
