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
#include <cstddef>

int main ()
{
   using namespace boost::interprocess;
   typedef std::pair<double, int> MyType;

   //Named allocate capable shared mem allocator
   //Create shared memory
   shared_memory_object::remove("MySharedMemory");
   managed_shared_memory segment
      (create_only, 
      "MySharedMemory",//segment name
      65536);           //segment size in bytes

   //Create an object of MyType initialized to {0, 0}
   segment.construct<MyType>
      ("MyType instance")  /*name of the object*/
      (0                   /*ctor first argument*/,
       0                   /*ctor second argument*/);    

   //Create an array of 10 elements of MyType initialized to {0, 0}
   segment.construct<MyType>
      ("MyType array")    /*name of the object*/
       [10]                /*number of elements*/
      (0                   /*ctor first argument*/,
       0                   /*ctor second argument*/);    

   
   //Let's simulate other process
  {
   using namespace boost::interprocess;
   typedef std::pair<double, int> MyType;

   //Named allocate capable shared mem allocator
   //Create shared memory
   managed_shared_memory segment
      (open_only, "MySharedMemory"); //segment name

   //Find the array and object
   std::pair<MyType*, std::size_t> res;
   res = segment.find<MyType> ("MyType array");   

   std::size_t array_len   = res.second;
   //Length should be 1
   assert(array_len == 10);

   //Find the array and the object
   res = segment.find<MyType> ("MyType instance");   

   std::size_t len   = res.second;

   //Length should be 1
   assert(len == 1);

   //Change data
   // . . . 

   //We're done, delete array from memory
   segment.destroy<MyType>("MyType array");
   //We're done, delete object from memory
   segment.destroy<MyType>("MyType instance");
 }

   MyType *anonymous = segment.construct<MyType>(anonymous_instance)
      [10]                 //number of elements
      (1,                  //ctor first argument
       1);                 //ctor second argument

   segment.destroy_ptr(anonymous);

   segment.construct<MyType>(unique_instance)
      [10]                 //number of elements
      (1,                   //ctor first argument
       1);                   //ctor second argument  

   std::pair<MyType *,std::size_t> ret = segment.find<MyType>(unique_instance);

   segment.destroy<MyType>(unique_instance);
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
