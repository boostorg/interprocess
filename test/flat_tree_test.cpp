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
#include <set>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/flat_set.hpp>
#include <boost/interprocess/containers/flat_map.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/indexes/flat_map_index.hpp>
#include "print_container.hpp"
#include "dummy_test_allocator.hpp"
#include "movable_int.hpp"
#include "set_test.hpp"
#include "map_test.hpp"

/////////////////////////////////////////////////////////////////
//                                                               
//  This example repeats the same operations with std::set and   
//  shmem_set using the node allocator                           
//  and compares the values of both containers                   
//                                                               
/////////////////////////////////////////////////////////////////

using namespace boost::interprocess;

//Explicit instantiation to detect compilation errors
template class boost::interprocess::flat_set
   <test::movable_and_copyable_int
   ,std::less<test::movable_and_copyable_int>
   ,test::dummy_test_allocator<test::movable_and_copyable_int> >;

template class boost::interprocess::flat_map
   <test::movable_and_copyable_int
   ,test::movable_and_copyable_int
   ,std::less<test::movable_and_copyable_int>
   ,test::dummy_test_allocator<std::pair<test::movable_and_copyable_int
                                        ,test::movable_and_copyable_int> > >;

template class boost::interprocess::flat_multiset
   <test::movable_and_copyable_int
   ,std::less<test::movable_and_copyable_int>
   ,test::dummy_test_allocator<test::movable_and_copyable_int> >;

template class boost::interprocess::flat_multimap
   <test::movable_and_copyable_int
   ,test::movable_and_copyable_int
   ,std::less<test::movable_and_copyable_int>
   ,test::dummy_test_allocator<std::pair<test::movable_and_copyable_int
                                        ,test::movable_and_copyable_int> > >;

//Customize managed_shared_memory class
typedef basic_managed_shared_memory
   <char,
    //simple_seq_fit<mutex_family>,
    rbtree_best_fit<mutex_family>,
    flat_map_index
   > my_managed_shared_memory;

//Alias allocator type
typedef allocator<int, my_managed_shared_memory::segment_manager> 
   shmem_allocator_t;
typedef allocator<test::movable_int, my_managed_shared_memory::segment_manager> 
   shmem_movable_allocator_t;
typedef allocator<std::pair<int, int>, my_managed_shared_memory::segment_manager> 
   shmem_pair_allocator_t;
typedef allocator<std::pair<test::movable_int, test::movable_int>, my_managed_shared_memory::segment_manager> 
   shmem_movable_pair_allocator_t;

typedef allocator<test::movable_and_copyable_int, my_managed_shared_memory::segment_manager> 
   shmem_move_copy_allocator_t;
typedef allocator<std::pair<test::movable_and_copyable_int, test::movable_and_copyable_int>, my_managed_shared_memory::segment_manager> 
   shmem_move_copy_pair_allocator_t;

//Alias set types
typedef std::set<int>                                                   MyStdSet;
typedef std::multiset<int>                                              MyStdMultiSet;
typedef std::map<int, int>                                              MyStdMap;
typedef std::multimap<int, int>                                         MyStdMultiMap;

typedef flat_set<int, std::less<int>, shmem_allocator_t>                MyShmSet;
typedef flat_multiset<int, std::less<int>, shmem_allocator_t>           MyShmMultiSet;
typedef flat_map<int, int, std::less<int>, shmem_pair_allocator_t>      MyShmMap;
typedef flat_multimap<int, int, std::less<int>, shmem_pair_allocator_t> MyShmMultiMap;

typedef flat_set<test::movable_int, std::less<test::movable_int>
                ,shmem_movable_allocator_t>                             MyMovableShmSet;
typedef flat_multiset<test::movable_int,std::less<test::movable_int>
                     ,shmem_movable_allocator_t>                        MyMovableShmMultiSet;
typedef flat_map<test::movable_int, test::movable_int
                ,std::less<test::movable_int>
                ,shmem_movable_pair_allocator_t>                        MyMovableShmMap;
typedef flat_multimap<test::movable_int, test::movable_int
                ,std::less<test::movable_int>
                ,shmem_movable_pair_allocator_t>                        MyMovableShmMultiMap;

typedef flat_set<test::movable_and_copyable_int, std::less<test::movable_and_copyable_int>
                ,shmem_move_copy_allocator_t>                             MyMoveCopyShmSet;
typedef flat_multiset<test::movable_and_copyable_int,std::less<test::movable_and_copyable_int>
                     ,shmem_move_copy_allocator_t>                        MyMoveCopyShmMultiSet;
typedef flat_map<test::movable_and_copyable_int, test::movable_and_copyable_int
                ,std::less<test::movable_and_copyable_int>
                ,shmem_move_copy_pair_allocator_t>                        MyMoveCopyShmMap;
typedef flat_multimap<test::movable_and_copyable_int, test::movable_and_copyable_int
                ,std::less<test::movable_and_copyable_int>
                ,shmem_move_copy_pair_allocator_t>                        MyMoveCopyShmMultiMap;

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

   if (0 != map_test<my_managed_shared_memory
                  ,MyShmMap
                  ,MyStdMap
                  ,MyShmMultiMap
                  ,MyStdMultiMap>()){
      std::cout << "Error in set_test<MyShmMap>" << std::endl;
      return 1;
   }

   if (0 != map_test_copyable<my_managed_shared_memory
                  ,MyShmMap
                  ,MyStdMap
                  ,MyShmMultiMap
                  ,MyStdMultiMap>()){
      std::cout << "Error in set_test<MyShmMap>" << std::endl;
      return 1;
   }

//   if (0 != map_test<my_managed_shared_memory
//                  ,MyMovableShmMap
//                  ,MyStdMap
//                  ,MyMovableShmMultiMap
//                  ,MyStdMultiMap>()){
//      return 1;
//   }

   if (0 != map_test<my_managed_shared_memory
                  ,MyMoveCopyShmMap
                  ,MyStdMap
                  ,MyMoveCopyShmMultiMap
                  ,MyStdMultiMap>()){
      std::cout << "Error in set_test<MyMoveCopyShmMap>" << std::endl;
      return 1;
   }

   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
