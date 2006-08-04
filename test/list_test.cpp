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

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/allocators/node_allocator.hpp>
#include <boost/interprocess/offset_ptr.hpp>
#include <cstddef>
#include <memory>
#include <new>

using namespace boost::interprocess;

typedef node_allocator
   <int, 64, managed_shared_memory::segment_manager> ShmemAllocator;

template class list<int, ShmemAllocator>;

struct int_holder
{
   int_holder(int value) : m_int(value){}
   bool operator==(int_holder other) const
      {  return m_int == other.m_int;   }
   bool operator<(int_holder other) const
      {  return m_int < other.m_int;   }

 private:
   int m_int;
};

typedef node_allocator
<int_holder, 64, managed_shared_memory::segment_manager> ShmemIntHolderAllocator;

//Instantations to detect compilation errors
template class list<int_holder, ShmemIntHolderAllocator>;
template class list<int_holder, std::allocator<int_holder> >;

int main ()
{
   //Create shared memory
   shared_memory_object::remove("MySharedMemory");
   managed_shared_memory segment
      (create_only, 
      "MySharedMemory",//segment name
      65536);           //segment size in bytes
   
   typedef list<int, ShmemAllocator> MyList;
   typedef list<int, ShmemAllocator> MyIntHolderList;

   int initVal[]  = {0, 1, 2, 3, 4, 5, 6 };
   const int *begVal    = initVal;
   const int *endVal    = initVal + sizeof(initVal)/sizeof(initVal[0]);

   const ShmemAllocator alloc_inst (segment.get_segment_manager());

   //Initialize vector with a range or iterators and allocator
   MyList *mylist = 
      segment.find_or_construct<MyList>
                                ("MyList", std::nothrow)  //object name 
                                (begVal,    //first ctor parameter
                                 endVal,    //second ctor parameter 
                                 alloc_inst);//third ctor parameter 

   //Use vector as you want
   mylist->insert(mylist->end(), begVal, endVal);
   // . . .
   //When done, delete vector
   segment.destroy<MyList>("MyList");

   const ShmemIntHolderAllocator & 
      alloc_int_holder(segment.get_segment_manager());

   MyIntHolderList *myintholderlist = 
      segment.find_or_construct<MyIntHolderList>
      ("MyList", std::nothrow)  //object name 
      (alloc_int_holder);

   mylist->insert(mylist->end(), 0);
   segment.destroy_ptr(myintholderlist);

   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
