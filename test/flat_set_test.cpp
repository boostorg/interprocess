//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2004-2026. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <set>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include "print_container.hpp"
#include "movable_int.hpp"
#include "set_test.hpp"
#include "emplace_test.hpp"

/////////////////////////////////////////////////////////////////
//
//  This example repeats the same operations with std::set and
//  shmem_set using the node allocator
//  and compares the values of both containers
//
/////////////////////////////////////////////////////////////////

using namespace boost::interprocess;

//Customize managed_shared_memory class
typedef basic_managed_shared_memory
   <char,
    //simple_seq_fit<mutex_family>,
    rbtree_best_fit<mutex_family>,
    iset_index
   > my_managed_shared_memory;

//Alias allocator type
typedef allocator<int, my_managed_shared_memory::segment_manager>
   shmem_allocator_t;
typedef allocator<test::movable_int, my_managed_shared_memory::segment_manager>
   shmem_movable_allocator_t;

typedef allocator<test::movable_and_copyable_int, my_managed_shared_memory::segment_manager>
   shmem_move_copy_allocator_t;

typedef allocator<test::copyable_int, my_managed_shared_memory::segment_manager>
   shmem_copy_allocator_t;

//Alias set types
typedef std::set<int>                                                   MyStdSet;
typedef std::multiset<int>                                              MyStdMultiSet;

typedef boost::container::flat_set<int, std::less<int>, shmem_allocator_t>                MyShmSet;
typedef boost::container::flat_multiset<int, std::less<int>, shmem_allocator_t>           MyShmMultiSet;

typedef boost::container::flat_set<test::movable_int, std::less<test::movable_int>
                ,shmem_movable_allocator_t>                             MyMovableShmSet;
typedef boost::container::flat_multiset<test::movable_int,std::less<test::movable_int>
                     ,shmem_movable_allocator_t>                        MyMovableShmMultiSet;

typedef boost::container::flat_set<test::movable_and_copyable_int, std::less<test::movable_and_copyable_int>
                ,shmem_move_copy_allocator_t>                             MyMoveCopyShmSet;
typedef boost::container::flat_multiset<test::movable_and_copyable_int,std::less<test::movable_and_copyable_int>
                     ,shmem_move_copy_allocator_t>                        MyMoveCopyShmMultiSet;

typedef boost::container::flat_set<test::copyable_int, std::less<test::copyable_int>
                ,shmem_copy_allocator_t>                                MyCopyShmSet;
typedef boost::container::flat_multiset<test::copyable_int,std::less<test::copyable_int>
                     ,shmem_copy_allocator_t>                           MyCopyShmMultiSet;

int main()
{
   using namespace boost::interprocess::test;

   if (0 != set_test<my_managed_shared_memory
                  ,MyShmSet
                  ,MyStdSet
                  ,MyShmMultiSet
                  ,MyStdMultiSet>()){
      std::cout << "Error in set_test<MyShmSet>" << std::endl;
      return 1;
   }

   if (0 != set_test_copyable<my_managed_shared_memory
                  ,MyShmSet
                  ,MyStdSet
                  ,MyShmMultiSet
                  ,MyStdMultiSet>()){
      std::cout << "Error in set_test<MyShmSet>" << std::endl;
      return 1;
   }

   if (0 != set_test<my_managed_shared_memory
                  ,MyMovableShmSet
                  ,MyStdSet
                  ,MyMovableShmMultiSet
                  ,MyStdMultiSet>()){
      std::cout << "Error in set_test<MyMovableShmSet>" << std::endl;
      return 1;
   }

   if (0 != set_test<my_managed_shared_memory
                  ,MyMoveCopyShmSet
                  ,MyStdSet
                  ,MyMoveCopyShmMultiSet
                  ,MyStdMultiSet>()){
      std::cout << "Error in set_test<MyMoveCopyShmSet>" << std::endl;
      return 1;
   }

   if (0 != set_test<my_managed_shared_memory
                  ,MyCopyShmSet
                  ,MyStdSet
                  ,MyCopyShmMultiSet
                  ,MyStdMultiSet>()){
      std::cout << "Error in set_test<MyCopyShmSet>" << std::endl;
      return 1;
   }

   //#if !defined(__GNUC__) || (__GNUC__ < 4) || (__GNUC_MINOR__ < 3)
   const test::EmplaceOptions SetOptions = (test::EmplaceOptions)(test::EMPLACE_HINT | test::EMPLACE_ASSOC);

   if(!boost::interprocess::test::test_emplace<boost::container::flat_set<test::EmplaceInt>, SetOptions>())
      return 1;
   if(!boost::interprocess::test::test_emplace<boost::container::flat_multiset<test::EmplaceInt>, SetOptions>())
      return 1;
   //#endif   //!defined(__GNUC__)
   return 0;

}
