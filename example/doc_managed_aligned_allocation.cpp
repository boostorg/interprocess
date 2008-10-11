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
//[doc_managed_aligned_allocation
#include <boost/interprocess/managed_shared_memory.hpp>
#include <cassert>

int main()
{
   using namespace boost::interprocess;

   //Managed memory segment that allocates portions of a shared memory
   //segment with the default management algorithm
   shared_memory_object::remove("MyManagedShm");

   try{
      managed_shared_memory managed_shm(create_only, "MyManagedShm", 65536);

      const std::size_t Alignment = 128;

      //Allocate 100 bytes aligned to Alignment from segment, throwing version
      void *ptr = managed_shm.allocate_aligned(100, Alignment);

      //Check alignment
      assert((static_cast<char*>(ptr)-static_cast<char*>(0)) % Alignment == 0);

      //Deallocate it
      managed_shm.deallocate(ptr);

      //Non throwing version
      ptr = managed_shm.allocate_aligned(100, Alignment, std::nothrow);

      //Check alignment
      assert((static_cast<char*>(ptr)-static_cast<char*>(0)) % Alignment == 0);

      //Deallocate it
      managed_shm.deallocate(ptr);

      //If we want to efficiently allocate aligned blocks of memory
      //use managed_shared_memory::PayloadPerAllocation value
      assert(Alignment > managed_shared_memory::PayloadPerAllocation);

      //This allocation will maximize the size of the aligned memory
      //and will increase the possibility of finding more aligned memory
      ptr = managed_shm.allocate_aligned
         (3*Alignment - managed_shared_memory::PayloadPerAllocation, Alignment);

      //Check alignment
      assert((static_cast<char*>(ptr)-static_cast<char*>(0)) % Alignment == 0);

      //Deallocate it
      managed_shm.deallocate(ptr);
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
