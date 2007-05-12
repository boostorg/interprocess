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
//[doc_named_allocA
#include <boost/interprocess/managed_shared_memory.hpp>
#include <utility>

int main ()
{
   using namespace boost::interprocess;
   typedef std::pair<double, int> MyType;

   try{
      //An special shared memory where we can
      //construct objects associated with a name.
      //First remove any old shared memory of the same name, create 
      //the shared memory segment and initialize needed resources
      shared_memory_object::remove("MySharedMemory");
      managed_shared_memory segment
         //create       segment name    segment size
         (create_only, "MySharedMemory", 65536);

      //Create an object of MyType initialized to {0.0, 0}
      MyType *instance = segment.construct<MyType>
         ("MyType instance")  //name of the object
         (0.0, 0);            //ctor second argument

      //Create an array of 10 elements of MyType initialized to {0.0, 0}
      MyType *array = segment.construct<MyType>
         ("MyType array")     //name of the object
         [10]                 //number of elements
         (0.0, 0);            //ctor second argument
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
