//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2004-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/offset_ptr.hpp>
#include "dummy_test_allocator.hpp"
#include "list_test.hpp"
#include "movable_int.hpp"

using namespace boost::interprocess;

//Explicit instantiation to detect compilation errors
template class boost::interprocess::list<test::movable_and_copyable_int, 
   test::dummy_test_allocator<test::movable_and_copyable_int> >;

typedef allocator<int, managed_shared_memory::segment_manager> ShmemAllocator;
typedef list<int, ShmemAllocator> MyList;

typedef allocator<volatile int, managed_shared_memory::segment_manager> ShmemVolatileAllocator;
typedef list<volatile int, ShmemVolatileAllocator> MyVolatileList;

typedef allocator<test::movable_int, managed_shared_memory::segment_manager> ShmemMoveAllocator;
typedef list<test::movable_int, ShmemMoveAllocator> MyMoveList;

typedef allocator<test::movable_and_copyable_int, managed_shared_memory::segment_manager> ShmemCopyMoveAllocator;
typedef list<test::movable_and_copyable_int, ShmemCopyMoveAllocator> MyCopyMoveList;

int main ()
{
   if(test::list_test<managed_shared_memory, MyList, true>())
      return 1;

   if(test::list_test<managed_shared_memory, MyVolatileList, true>())
      return 1;

   if(test::list_test<managed_shared_memory, MyMoveList, true>())
      return 1;

   if(test::list_test<managed_shared_memory, MyCopyMoveList, true>())
      return 1;

   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
