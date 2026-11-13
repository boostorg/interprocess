////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2026-2026. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_TEST_ALLOCATOR_TEST_UTILS_HEADER
#define BOOST_INTERPROCESS_TEST_ALLOCATOR_TEST_UTILS_HEADER

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/managed_heap_memory.hpp>

namespace boost{
namespace interprocess{
namespace test{

//A value type with a size different from int, so that the pools
//of the rebound allocators are different pools on all platforms.
struct allocator_compare_big_t
{
   char data[sizeof(int)*16u];
};

//Checks == and != are consistent
template<class Alloc1, class Alloc2>
bool check_allocator_compare(const Alloc1 &a1, const Alloc2 &a2, bool equal)
{
   return (a1 == a2) == equal && (a2 == a1) == equal &&
          (a1 != a2) != equal && (a2 != a1) != equal;
}

//Compares allocators with different value types
// 
//If rebound_allocators_equal is true, allocators are equal if they use
//the same segment manager (shared pool)
// 
//If rebound_allocators_equal is false, allocators with different value
//types are never equal (private pools).
template<class IntAlloc>
int allocator_compare_test(bool rebound_allocators_equal = true)
{
   typedef typename IntAlloc::template rebind<allocator_compare_big_t>::other BigAlloc;
   managed_heap_memory heap1(1024*1024);
   managed_heap_memory heap2(1024*1024);
   const IntAlloc a(heap1.get_segment_manager());
   const BigAlloc b(heap1.get_segment_manager());
   const BigAlloc c(heap2.get_segment_manager());

   //Same segment manager
   if(!check_allocator_compare(a, a, true))
      return 1;
   if(!check_allocator_compare(a, b, rebound_allocators_equal))
      return 1;

   //The result must be the same as with an explicit rebound copy
   const IntAlloc rb(b);
   if(!check_allocator_compare(a, rb, rebound_allocators_equal))
      return 1;

   //Different segment managers
   if(!check_allocator_compare(a, c, false))
      return 1;
   const IntAlloc rc(c);
   if(!check_allocator_compare(a, rc, false))
      return 1;

   return 0;
}

}  //namespace test{
}  //namespace interprocess{
}  //namespace boost{

#include <boost/interprocess/detail/config_end.hpp>

#endif
