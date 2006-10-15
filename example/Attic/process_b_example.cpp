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

#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>

#include "print_container.hpp"
#include <algorithm>

using namespace boost::interprocess;

int main ()
{
   //Shared memory attributes
   const char *const shMemName = "MySharedMemory";

   //Create sems for synchronization
   named_semaphore semA(open_or_create, "processAsem", 0);
   named_semaphore semB(open_or_create, "processBsem", 0);

   //Wait until the shared memory is ready
   semB.wait();

   //Create shared memory
   managed_shared_memory segment(open_or_create, shMemName, 65536);

   //STL compatible allocator object, uses allocate(), deallocate() functions
   typedef allocator<int, managed_shared_memory::segment_manager>
      shmem_allocator_int_t;

   //This is the shared memory vector type
   typedef vector<int, shmem_allocator_int_t >  MyVect;

   //Finding vector in shared memory and printing contents
   std::cout << "Connecting to object ShmVect\n\n";
   MyVect *shmem_vect = segment.find<MyVect>("ShmVect").first;
   PrintContents(*shmem_vect, "ShmVect");

   //Reverse sorting the vector with std::sort
   std::cout << "Reverse sorting ShmVect\n\n";
   MyVect::reverse_iterator rbeg = 
      shmem_vect->rbegin(), rend = shmem_vect->rend();
   std::sort(shmem_vect->rbegin(), shmem_vect->rend());
   std::sort(rbeg, rend);

   //Printing values after sorting
   std::cout << "Sorting complete\n\n";
   PrintContents(*shmem_vect, "ShmVect");

   //Waking up process A
   std::cout << "Waking up processA\n\n";
   semA.post();

   //We're done, closing shared memory
   std::cout << "Closing shmem segment\n\n";

   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
