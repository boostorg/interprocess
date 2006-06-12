//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/mem_algo/simple_seq_fit.hpp>
#include <boost/interprocess/indexes/null_index.hpp>
#include <boost/interprocess/sync/mutex_family.hpp>
#include "allocation_test_template.hpp"
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
         ,simple_seq_fit<mutex_family, offset_ptr<void> >
         ,null_index
         > my_managed_shared_memory;

      //Create shared memory
      my_managed_shared_memory::remove(shMemName);
      my_managed_shared_memory segment(create_only, shMemName, memsize);

      //Now take the segment manager and launch memory test
      if(!test::test_all_allocation(*segment.get_segment_manager())){
         return 1;
      }
   }

   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>

