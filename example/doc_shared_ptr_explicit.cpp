//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2007.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

//[doc_shared_ptr_explicit
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/smart_ptr/shared_ptr.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/smart_ptr/deleter.hpp>
#include <cassert>

using namespace boost::interprocess;

//This is type of the object we want to share
class MyType
{};

typedef managed_shared_memory::segment_manager segment_manager_type;
typedef allocator<void, segment_manager_type>  void_allocator_type;
typedef deleter<MyType, segment_manager_type>  deleter_type;
typedef shared_ptr<MyType, void_allocator_type, deleter_type> my_shared_ptr;

int main ()
{
   //Destroy any previous segment with the name to be used.
   shared_memory_object::remove("MySharedMemory");
   managed_shared_memory segment(create_only, "MySharedMemory", 4096);
   
   //Create a shared pointer in shared memory
   //pointing to a newly created object in the segment
   my_shared_ptr &shared_ptr_instance =
      *segment.construct<my_shared_ptr>("shared ptr")
         //Arguments to construct the shared pointer
         ( segment.construct<MyType>("object to share")()      //object to own
         , void_allocator_type(segment.get_segment_manager())  //allocator
         , deleter_type(segment.get_segment_manager())         //deleter
         );
   assert(shared_ptr_instance.use_count() == 1);

   //Destroy "shared ptr". "object to share" will be automatically destroyed
   segment.destroy_ptr(&shared_ptr_instance);

   shared_memory_object::remove("MySharedMemory");
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
