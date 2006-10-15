//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2004-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/offset_ptr.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include "print_container.hpp"
#include <algorithm>
#include <boost/interprocess/sync/named_semaphore.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>

using namespace boost::interprocess;

int main ()
{
   //Shared memory attributes
   const int memsize = 65536;
   const char *const shMemName = "MySharedMemory";

   //Create sems for synchronization
   named_semaphore semA(open_or_create, "processAsem", 0);
   named_semaphore semB(open_or_create, "processBsem", 0);

   //Create shared memory
   managed_shared_memory segment(open_or_create, shMemName, memsize);

   //STL compatible allocator object, uses allocate(), deallocate() functions
   typedef allocator<int, managed_shared_memory::segment_manager>
      shmem_allocator_int_t;

   const int num_elements = 100;

   //Type of shared memory vector
   typedef vector<int, shmem_allocator_int_t > MyVect;

   const shmem_allocator_int_t &alloc_ref (segment.get_segment_manager());

   //Creating the vector in shared memory
   std::cout << "Named New of ShmVect\n\n";
   MyVect *shmem_vect = segment.construct<MyVect> ("ShmVect")(alloc_ref);

   offset_ptr<MyVect> shmptr_vect = 0;
   offset_ptr<MyVect> other_shmptr_vect = 0;

   //Fill the vector
   std::cout << "Filling ShmVect\n\n";
   int i;
   for(i = 0; i < num_elements; ++i){
      shmem_vect->push_back(i);
   }

   //Printing contents before waiting to second process
   PrintContents(*shmem_vect, "ShmVect");

   //Wake up other process and sleeping until notified
   semB.post();
   std::cout << "Waking up processB and waiting sorting\n\n";
   semA.wait();

   //Notification received, let's see the changes
   std::cout << "processB sorting complete\n\n";
   PrintContents(*shmem_vect, "ShmVect");

   //Let's delete the vector from memory
   std::cout << "Deleting the vector with destroy\n\n";
   segment.destroy<MyVect> ("ShmVect");
   std::cout << "vector deleted\n\n";
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
