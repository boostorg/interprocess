//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
//[doc_managed_multiple_allocation
#include <boost/interprocess/managed_shared_memory.hpp>
#include <cassert>//assert
#include <cstring>//std::memset
#include <new>    //std::nothrow

int main()
{
   using namespace boost::interprocess;
   typedef managed_shared_memory::multiallocation_iterator multiallocation_iterator;

   //Try to erase any previous managed segment with the same name
   shared_memory_object::remove("MyManagedShm");

   try{
      managed_shared_memory managed_shm(create_only, "MyManagedShm", 65536);
      std::size_t received_size;
      //Allocate 16 elements of 100 bytes in a single call. Non-throwing version.
      multiallocation_iterator beg_it = managed_shm.allocate_many(100, 16, 16, received_size, std::nothrow);

      //To check for an error, we can use a boolean expresssion
      //or compare it with a default constructed iterator
      assert(!beg_it == (beg_it == multiallocation_iterator()));
      
      //Check if the memory allocation was successful
      if(!beg_it)  return 1;

      //Initialize our data
      for( multiallocation_iterator it = beg_it, end_it; it != end_it; )
         std::memset(*it++, 0, 100);

      //Now deallocate
      for(multiallocation_iterator it = beg_it, end_it; it != end_it;)
         managed_shm.deallocate(*it++);

      //Allocate 10 buffers of different sizes in a single call. Throwing version
      std::size_t sizes[10];
      for(std::size_t i = 0; i < 10; ++i)
         sizes[i] = i*3;

      beg_it  = managed_shm.allocate_many(sizes, 10);

      //Iterate each allocated buffer and deallocate
      //The "end" condition can be also checked with operator!
      for(multiallocation_iterator it = beg_it; it;)
         managed_shm.deallocate(*it++);
   }
   catch(...){
      shared_memory_object::remove("MyManagedShm");
      throw;
   }
   shared_memory_object::remove("MyManagedShm");
   return 0;
}
//]
