//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2026-2026. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/container/list.hpp>
#include <boost/container/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include "allocator_v1.hpp"
#include "movable_int.hpp"
#include "list_test.hpp"
#include "vector_test.hpp"
#include "allocator_test_utils.hpp"

using namespace boost::interprocess;

typedef test::overaligned_copyable_int oint_t;

//Alias an allocator that allocates ints
typedef allocator
   <int, managed_shared_memory::segment_manager> shmem_allocator_t;
typedef test::allocator_v1
   <int, managed_shared_memory::segment_manager> shmem_allocator_v1_t;
typedef allocator
   < oint_t, managed_shared_memory::segment_manager> shmem_oallocator_t;
typedef test::allocator_v1
   < oint_t, managed_shared_memory::segment_manager> shmem_oallocator_v1_t;

namespace boost {
namespace interprocess {

//Explicit instantiations to catch compilation errors
template class allocator<int, managed_shared_memory::segment_manager>;
template class allocator<oint_t, managed_shared_memory::segment_manager>;
template class allocator<void, managed_shared_memory::segment_manager>;

namespace test {

template class test::allocator_v1<int, managed_shared_memory::segment_manager>;
template class test::allocator_v1<oint_t, managed_shared_memory::segment_manager>;

}}}

//Alias list types
typedef boost::container::list<int, shmem_allocator_t>    MyShmList;
typedef boost::container::list<int, shmem_allocator_v1_t> MyShmListV1;
typedef boost::container::list<oint_t, shmem_oallocator_t>    MyOShmList;
typedef boost::container::list<oint_t, shmem_oallocator_v1_t> MyOShmListV1;

//Alias vector types
typedef boost::container::vector<int, shmem_allocator_t>    MyShmVector;
typedef boost::container::vector<int, shmem_allocator_v1_t> MyShmVectorV1;
typedef boost::container::vector<oint_t, shmem_oallocator_t>    MyOShmVector;
typedef boost::container::vector<oint_t, shmem_oallocator_v1_t> MyOShmVectorV1;

int main ()
{
   if(test::list_test<managed_shared_memory, MyShmList, true>())
      return 1;
   if(test::list_test<managed_shared_memory, MyShmListV1, true>())
      return 1;

   if(test::list_test<managed_shared_memory, MyOShmList, true>())
      return 1;
   if(test::list_test<managed_shared_memory, MyOShmListV1, true>())
      return 1;

   if(test::vector_test<managed_shared_memory, MyShmVector>())
      return 1;
   if(test::vector_test<managed_shared_memory, MyShmVectorV1>())
      return 1;

   if(test::vector_test<managed_shared_memory, MyOShmVector>())
      return 1;
   if(test::vector_test<managed_shared_memory, MyOShmVectorV1>())
      return 1;

   if(test::allocator_compare_test
      < allocator<int, managed_heap_memory::segment_manager> >())
      return 1;

   //allocator<void> is used as a type erased allocator: it must convert
   //to and from allocators of other value types and compare with them
   {
      typedef allocator<void, managed_heap_memory::segment_manager> void_alloc_t;
      typedef allocator<int, managed_heap_memory::segment_manager>  int_alloc_t;
      managed_heap_memory heap(1024*1024);
      const void_alloc_t va(heap.get_segment_manager());
      const int_alloc_t ia(va);
      const void_alloc_t va2(ia);
      if(!(ia == va) || !(va == ia) || ia != va || va != ia)
         return 1;
      if(!(va2 == va) || va2 != va)
         return 1;
      boost::container::vector<int, int_alloc_t> v(va);
      v.push_back(1);
      if(v.get_allocator() != va)
         return 1;
   }

   return 0;
}
