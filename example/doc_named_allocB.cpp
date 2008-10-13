//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
//[doc_named_allocB
#include <boost/interprocess/managed_shared_memory.hpp>
#include <cstddef>
#include <utility>
#include <cassert>

int main ()
{
   using namespace boost::interprocess;
   typedef std::pair<double, int> MyType;

   try{
      //A special shared memory where we can
      //construct objects associated with a name.
      //Connect to the already created shared memory segment
      //and initialize needed resources
      managed_shared_memory segment(open_only, "MySharedMemory");

      std::pair<MyType*, std::size_t> res;

      //Find the array
      res = segment.find<MyType> ("MyType array");   
      //Length should be 10
      assert(res.second == 10);

      //Find the object
      res = segment.find<MyType> ("MyType instance");   
      //Length should be 1
      assert(res.second == 1);

      //Find the array constructed from iterators
      res = segment.find<MyType> ("MyType array from it");
      //Length should be 3
      assert(res.second == 3);

      //Use data
      // . . . 

      //We're done, delete all the objects
      segment.destroy<MyType>("MyType array");
      segment.destroy<MyType>("MyType instance");
      segment.destroy<MyType>("MyType array from it");
   }
   catch(...){
      shared_memory_object::remove("MySharedMemory");
      throw;
   }
   shared_memory_object::remove("MySharedMemory");
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
