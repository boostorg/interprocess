//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2004-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/indexes/unordered_map_index.hpp>
#include <boost/interprocess/sync/mutex_family.hpp>
#include <boost/interprocess/allocators/node_allocator.hpp>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/interprocess/offset_ptr.hpp>
#include <cstddef>
#include <new>
#include <memory>
#include <functional>

using namespace boost::interprocess;

//Customize managed_shared_memory class
typedef basic_managed_shared_memory
   <char,
    simple_seq_fit<mutex_family>,
    unordered_map_index
   > my_managed_shared_memory;
typedef node_allocator<int, 64, my_managed_shared_memory::segment_manager> 
   ShmemAllocator;

//explicit instantiations

template class boost::unordered_set < int,
                                      boost::hash<int>,
                                      std::equal_to<int>,
                                      ShmemAllocator >;

template class boost::unordered_multiset < int,
                                           boost::hash<int>,
                                           std::equal_to<int>,
                                           ShmemAllocator >;

template class boost::unordered_map < int, int,
                                      boost::hash<int>,
                                      std::equal_to<int>,
                                      ShmemAllocator >;

template class boost::unordered_multimap < int, int,
                                           boost::hash<int>,
                                           std::equal_to<int>,
                                           ShmemAllocator >;

int main ()
{
   //Create shared memory
   my_managed_shared_memory::remove("MySharedMemory");
   my_managed_shared_memory segment
      (create_only, 
      "MySharedMemory",//segment name (leading / for portability) 
      65536);           //segment size in bytes

   my_managed_shared_memory::remove("MySharedMemory2");
   my_managed_shared_memory segment2
      (create_only, 
      "MySharedMemory2",//segment name (leading / for portability)  
      65536);            //segment size in bytes
   
   typedef boost::unordered_set <int,
                                 boost::hash<int>,
                                 std::equal_to<int>,
                                 ShmemAllocator > MyUnorderedSet;

   boost::unordered_set <std::string> temp;
   std::string value("std::string");
   temp.insert(value);

   boost::unordered_set<int,
                        boost::hash<int>,
                        std::equal_to<int>,
                        std::allocator<int> > stdset;

   int initVal[]  = {0, 1, 2, 3, 4, 5, 6 };
   const int *begVal    = initVal;
   const int *endVal    = initVal + sizeof(initVal)/sizeof(initVal[0]);

   const ShmemAllocator alloc_inst (segment.get_segment_manager());
   const ShmemAllocator alloc_inst2(segment2.get_segment_manager());

   //Initialize with a range or iterators and allocator
   const int Count = 100;
   MyUnorderedSet *my_unordered_set = 
      segment.find_or_construct<MyUnorderedSet>
                                ("MyUnorderedSet", std::nothrow)  //object name 
                                (begVal,    //first ctor parameter
                                 endVal,    //second ctor parameter 
                                 Count,//third ctor parameter 
                                 MyUnorderedSet::hasher(),  //fourth ctor parameter 
                                 MyUnorderedSet::key_equal(),//fifth ctor parameter 
                                 alloc_inst);      // sixth ctor parameter 


   MyUnorderedSet *my_unordered_set2 = 
      segment2.find_or_construct<MyUnorderedSet>
                                ("MyUnorderedSet2", std::nothrow)  //object name 
                                (begVal,    //first ctor parameter
                                 endVal,    //second ctor parameter 
                                 Count,//third ctor parameter 
                                 MyUnorderedSet::hasher(),  //fourth ctor parameter 
                                 MyUnorderedSet::key_equal(),//fifth ctor parameter 
                                 alloc_inst2);      // sixth ctor parameter 

   MyUnorderedSet *my_unordered_set3 = 
      segment.find_or_construct<MyUnorderedSet>
                                ("MyUnorderedSet3", std::nothrow)  //object name 
                                (*my_unordered_set);//first ctor parameter

   //Use as you want
   my_unordered_set->insert(begVal, endVal);

   //Use as you want
   my_unordered_set->insert(*begVal);
   my_unordered_set->clear();
   my_unordered_set->rehash(1000);

   //Double swapping with equal allocators. Just internal swapping
   my_unordered_set->swap(*my_unordered_set3);
   my_unordered_set->swap(*my_unordered_set3);

   my_unordered_set->insert(1000);
   my_unordered_set->size();
   my_unordered_set2->size();
   my_unordered_set2->insert(begVal, endVal);
   *my_unordered_set = *my_unordered_set2;
   my_unordered_set->size();
   // . . .

   //When done, delete 
   segment.destroy<MyUnorderedSet>("MyUnorderedSet");
   //When done, delete 
   segment2.destroy<MyUnorderedSet>("MyUnorderedSet2");
   segment.destroy<MyUnorderedSet>("MyUnorderedSet3");
   return 0;
}

