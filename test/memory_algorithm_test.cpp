//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/mem_algo/simple_seq_fit.hpp>
#include <boost/interprocess/mem_algo/rbtree_best_fit.hpp>
#include <boost/interprocess/indexes/null_index.hpp>
#include <boost/interprocess/sync/mutex_family.hpp>
#include "memory_algorithm_test_template.hpp"
#include <iostream>


int main ()
{
   using namespace boost::interprocess;
   const int memsize = 16384;
   const char *const shMemName = "MySharedMemory";

   {
      //A shared memory with simple sequential fit algorithm
      typedef basic_managed_shared_memory
         <char
         ,simple_seq_fit<mutex_family>
         ,null_index
         > my_managed_shared_memory;

      //Create shared memory
      shared_memory_object::remove(shMemName);
      my_managed_shared_memory segment(create_only, shMemName, memsize);

      //Now take the segment manager and launch memory test
      if(!test::test_all_allocation(*segment.get_segment_manager())){
         return 1;
      }
   }

   {
      //A shared memory with red-black tree best fit algorithm
      typedef basic_managed_shared_memory
         <char
         ,rbtree_best_fit<mutex_family>
         ,null_index
         > my_managed_shared_memory;

      //Create shared memory
      shared_memory_object::remove(shMemName);
      my_managed_shared_memory segment(create_only, shMemName, memsize);

      //Now take the segment manager and launch memory test
      if(!test::test_all_allocation(*segment.get_segment_manager())){
         return 1;
      }
   }
   shared_memory_object::remove(shMemName);
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
