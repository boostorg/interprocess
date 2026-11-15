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
#include <boost/interprocess/exceptions.hpp>
#include <boost/container/allocator_traits.hpp>
#include <boost/container/scoped_allocator.hpp>
#include <boost/container/list.hpp>
#include <boost/container/detail/type_traits.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/move/detail/iterator_to_raw_pointer.hpp>
#include <cstddef>
#include <cstdio>

//Tests for the Interprocess allocators

namespace boost{
namespace interprocess{
namespace test{

//The size of each managed_heap_memory segment that the tests use
static const std::size_t allocator_test_segment_size = 1024u*1024u;

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
   managed_heap_memory heap1(allocator_test_segment_size);
   managed_heap_memory heap2(allocator_test_segment_size);
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

   //scoped_allocator_adaptor compares as the allocators it adapts
   typedef boost::container::scoped_allocator_adaptor<IntAlloc> scoped_int_t;
   typedef boost::container::scoped_allocator_adaptor<BigAlloc> scoped_big_t;
   const scoped_int_t sa(a);
   const scoped_big_t sb(b);
   const scoped_big_t sc(c);
   if(!check_allocator_compare(sa, sb, rebound_allocators_equal))
      return 1;
   if(!check_allocator_compare(sa, sc, false))
      return 1;

   return 0;
}

//The propagation traits that containers see
template<class Alloc, bool POCCA, bool POCMA, bool POCS, bool AlwaysEqual>
void propagation_traits_test()
{
   typedef boost::container::allocator_traits<Alloc> traits_t;
   BOOST_INTERPROCESS_STATIC_ASSERT((traits_t::propagate_on_container_copy_assignment::value == POCCA));
   BOOST_INTERPROCESS_STATIC_ASSERT((traits_t::propagate_on_container_move_assignment::value == POCMA));
   BOOST_INTERPROCESS_STATIC_ASSERT((traits_t::propagate_on_container_swap::value == POCS));
   BOOST_INTERPROCESS_STATIC_ASSERT((traits_t::is_always_equal::value == AlwaysEqual));
}

//Tests an allocator of void, used as a type erased allocator
template<class VoidAlloc>
int void_allocator_test(bool copies_equal = true)
{
   typedef boost::container::allocator_traits<VoidAlloc>                      void_traits;
   typedef typename void_traits::template portable_rebind_alloc<int>::type    int_alloc_t;
   typedef typename void_traits::template portable_rebind_alloc<double>::type double_alloc_t;
   typedef boost::container::allocator_traits<int_alloc_t>                    int_traits;
   using boost::container::dtl::is_same;
   using boost::container::dtl::is_convertible;

   BOOST_INTERPROCESS_STATIC_ASSERT((is_same<typename void_traits::value_type, void>::value));
   BOOST_INTERPROCESS_STATIC_ASSERT((is_same<typename int_traits::value_type, int>::value));
   //The pointers of the void allocator are the void pointers of the other ones
   BOOST_INTERPROCESS_STATIC_ASSERT((is_same<typename void_traits::pointer, typename int_traits::void_pointer>::value));
   BOOST_INTERPROCESS_STATIC_ASSERT((is_same<typename void_traits::const_pointer, typename int_traits::const_void_pointer>::value));
   BOOST_INTERPROCESS_STATIC_ASSERT((is_same<typename void_traits::void_pointer, typename void_traits::pointer>::value));
   //Rebinding back to void gives the same allocator
   BOOST_INTERPROCESS_STATIC_ASSERT((is_same<typename int_traits::template portable_rebind_alloc<void>::type, VoidAlloc>::value));
   BOOST_INTERPROCESS_STATIC_ASSERT((is_same<typename int_traits::template portable_rebind_alloc<double>::type, double_alloc_t>::value));

   //The void allocator propagates like the other types
   BOOST_INTERPROCESS_STATIC_ASSERT((void_traits::propagate_on_container_copy_assignment::value == int_traits::propagate_on_container_copy_assignment::value));
   BOOST_INTERPROCESS_STATIC_ASSERT((void_traits::propagate_on_container_move_assignment::value == int_traits::propagate_on_container_move_assignment::value));
   BOOST_INTERPROCESS_STATIC_ASSERT((void_traits::propagate_on_container_swap::value == int_traits::propagate_on_container_swap::value));
   BOOST_INTERPROCESS_STATIC_ASSERT((void_traits::is_always_equal::value == int_traits::is_always_equal::value));

   //Conversions in both directions and between non-void types
   BOOST_INTERPROCESS_STATIC_ASSERT((is_convertible<int_alloc_t, VoidAlloc>::value));
   BOOST_INTERPROCESS_STATIC_ASSERT((is_convertible<VoidAlloc, int_alloc_t>::value));
   BOOST_INTERPROCESS_STATIC_ASSERT((is_convertible<VoidAlloc, double_alloc_t>::value));
   BOOST_INTERPROCESS_STATIC_ASSERT((is_convertible<int_alloc_t, double_alloc_t>::value));

   managed_heap_memory heap(allocator_test_segment_size);
   const int_alloc_t    i(heap.get_segment_manager());
   const VoidAlloc      v(i);
   const int_alloc_t    i2(v);
   const double_alloc_t d(v);
   const VoidAlloc      v2(d);
   const VoidAlloc      v3(v);

   //An allocator is equal to itself, and to its copies if they share the pool
   if(!check_allocator_compare(v, v, true))
      return 1;
   if(!check_allocator_compare(v, v3, copies_equal))
      return 1;
   if(!check_allocator_compare(v, v2, copies_equal))
      return 1;
   if(!check_allocator_compare(i2, v, copies_equal))
      return 1;
   if(v.get_segment_manager() != heap.get_segment_manager() ||
      i2.get_segment_manager() != heap.get_segment_manager())
      return 1;

   //The void allocator converts to an allocator that allocates
   int_alloc_t i3(v);
   typename int_traits::pointer p = i3.allocate(2u);
   if(!p)
      return 1;
   i3.deallocate(p, 2u);
   return 0;
}

//Appends the values 0 to n-1 to a list
template<class List>
void fill_transfer_list(List &l, int n)
{
   for(int i = 0; i != n; ++i)
      l.push_back(i);
}

//Checks a list that took the nodes of a list filled by fill_transfer_list,
//after the source list is destroyed: the nodes must be usable, and a new
//node must be allocated from the allocator of the list
template<class List>
int check_transfer_list(List &l, int n, managed_heap_memory &heap, const char *what)
{
   l.push_back(n);
   if(int(l.size()) != n + 1 || l.front() != 0 || l.back() != n){
      std::printf("container_transfer_test: wrong elements after %s\n", what);
      return 1;
   }
   if(!heap.check_sanity()){
      std::printf("container_transfer_test: heap is corrupted after %s\n", what);
      return 1;
   }
   return 0;
}

//Moves, move assigns and swaps containers. The nodes must stay with the
//allocator that allocated them, also after the source container (and its
//allocator) is destroyed. For allocators with private pools this needs
//propagate_on_container_move_assignment and propagate_on_container_swap.
template<class IntAlloc>
int container_transfer_test()
{
   typedef boost::container::list<int, IntAlloc> list_t;
   managed_heap_memory heap(allocator_test_segment_size);
   const int n = 300;

   //Move construction, then swap with the target
   {
      list_t dst(IntAlloc(heap.get_segment_manager()));
      dst.push_back(-1);
      {
         list_t src(IntAlloc(heap.get_segment_manager()));
         fill_transfer_list(src, n);
         list_t tmp(boost::move(src));
         dst.swap(tmp);
      }
      if(check_transfer_list(dst, n, heap, "move construction"))
         return 1;
   }

   //Move construction with another allocator, then swap with the target
   {
      list_t dst(IntAlloc(heap.get_segment_manager()));
      dst.push_back(-1);
      {
         list_t src(IntAlloc(heap.get_segment_manager()));
         fill_transfer_list(src, n);
         const IntAlloc a(heap.get_segment_manager());
         list_t tmp(boost::move(src), a);
         dst.swap(tmp);
      }
      if(check_transfer_list(dst, n, heap, "move construction with an allocator"))
         return 1;
   }

   //Move assignment
   {
      list_t dst(IntAlloc(heap.get_segment_manager()));
      dst.push_back(-1);
      {
         list_t src(IntAlloc(heap.get_segment_manager()));
         fill_transfer_list(src, n);
         dst = boost::move(src);
      }
      if(check_transfer_list(dst, n, heap, "move assignment"))
         return 1;
   }

   //Swap
   {
      list_t dst(IntAlloc(heap.get_segment_manager()));
      dst.push_back(-1);
      {
         list_t src(IntAlloc(heap.get_segment_manager()));
         fill_transfer_list(src, n);
         dst.swap(src);
      }
      if(check_transfer_list(dst, n, heap, "swap"))
         return 1;
   }
   return 0;
}

//Allocators with a private pool: move assignment deallocates the blocks of the
//target and the target takes the blocks of the source.
template<class IntAlloc>
int private_allocator_move_test()
{
   typedef typename IntAlloc::multiallocation_chain chain_t;
   managed_heap_memory heap(allocator_test_segment_size);
   const std::size_t initial_free = heap.get_free_memory();
   {
      IntAlloc a(heap.get_segment_manager());
      IntAlloc b(heap.get_segment_manager());
      //The nodes are deallocated, but the pools keep their blocks
      chain_t chain;
      a.allocate_individual(32u, chain);
      a.deallocate_individual(chain);
      b.allocate_individual(32u, chain);
      b.deallocate_individual(chain);
      const std::size_t free_before_move = heap.get_free_memory();
      a = boost::move(b);
      if(heap.get_free_memory() <= free_before_move){
         std::printf("private_allocator_move_test: move assignment did not deallocate the old blocks\n");
         return 1;
      }
      //Both allocators can allocate after the move
      typename IntAlloc::pointer p = a.allocate_one();
      typename IntAlloc::pointer q = b.allocate_one();
      a.deallocate_one(p);
      b.deallocate_one(q);
      //A move constructed allocator takes the pool too
      IntAlloc c(boost::move(a));
      p = c.allocate_one();
      c.deallocate_one(p);
   }
   if(heap.get_free_memory() != initial_free || !heap.check_sanity()){
      std::printf("private_allocator_move_test: memory leaked or heap corrupted\n");
      return 1;
   }
   return 0;
}

//allocate throws bad_alloc when the memory can not be allocated.
template<class Alloc>
int allocate_throws_bad_alloc_test()
{
   #ifndef BOOST_NO_EXCEPTIONS
   typedef typename Alloc::pointer pointer;
   managed_heap_memory heap(allocator_test_segment_size);
   Alloc a(heap.get_segment_manager());
   pointer p0 = pointer();
   pointer p1 = pointer();
   bool thrown = false;
   BOOST_INTERPROCESS_TRY{
      p0 = a.allocate(a.max_size());
      p1 = a.allocate(a.max_size());
   }
   BOOST_INTERPROCESS_CATCH(const boost::interprocess::bad_alloc &){
      thrown = true;
   }
   BOOST_INTERPROCESS_CATCH_END
   if(p1)
      a.deallocate(p1, a.max_size());
   if(p0)
      a.deallocate(p0, a.max_size());
   if(!thrown){
      std::printf("allocate_throws_bad_alloc_test: allocate(max_size()) did not throw\n");
      return 1;
   }
   #endif   //BOOST_NO_EXCEPTIONS
   return 0;
}

//Largest node count any case below asks for
static const std::size_t extended_alloc_max_nodes = 40u;

//Structural invariants that every bulk allocation must satisfy
template<class Allocator, class Chain>
int check_chain(Chain &chain, std::size_t expected_size, const char *alloc, const char *what)
{
   typedef typename Allocator::value_type value_type;
   typedef typename Chain::iterator       chain_iterator;

   //The chain must report as many nodes as expected
   if(chain.size() != expected_size){
      std::printf("  %s / %s: chain.size()=%u, expected %u\n", alloc, what,
                  static_cast<unsigned>(chain.size()), static_cast<unsigned>(expected_size));
      return 1;
   }

   //Collect the nodes in the array
   void *ptrs[extended_alloc_max_nodes];
   std::size_t walked = 0;
   for(chain_iterator it = chain.begin(), e = chain.end();
       it != e && walked < extended_alloc_max_nodes; ++it){
      ptrs[walked++] = static_cast<void *>(boost::movelib::iterator_to_raw_pointer(it));
   }

   //The chain must link as many nodes as it reports
   if(walked != expected_size){
      std::printf("  %s / %s: chain reports %u nodes but links %u\n", alloc, what,
                  static_cast<unsigned>(expected_size), static_cast<unsigned>(walked));
      return 1;
   }

   //The nodes must be non-null, aligned and distinct
   const std::size_t align = boost::container::dtl::alignment_of<value_type>::value;
   for(std::size_t i = 0; i != walked; ++i){
      if(!ptrs[i]){
         std::printf("  %s / %s: node %u is null\n", alloc, what, static_cast<unsigned>(i));
         return 1;
      }
      if(reinterpret_cast<std::size_t>(ptrs[i]) % align){
         std::printf("  %s / %s: node %u misaligned (needs %u)\n", alloc, what,
                     static_cast<unsigned>(i), static_cast<unsigned>(align));
         return 1;
      }
      for(std::size_t j = 0; j != i; ++j){
         if(ptrs[i] == ptrs[j]){
            std::printf("  %s / %s: nodes %u and %u are the same address\n", alloc, what,
                        static_cast<unsigned>(j), static_cast<unsigned>(i));
            return 1;
         }
      }
   }
   return 0;
}

//Writes a distinct byte pattern over every block and reads it all back
template<class Allocator>
int check_blocks_are_usable(Allocator &a, std::size_t n, const char *alloc)
{
   typedef typename Allocator::value_type            value_type;
   typedef typename Allocator::pointer               pointer;
   typedef typename Allocator::multiallocation_chain chain_t;

   chain_t chain;
   a.allocate_individual(n, chain);
   if(check_chain<Allocator>(chain, n, alloc, "allocate_individual (usable)"))
      return 1;
   pointer nodes[extended_alloc_max_nodes];
   for(std::size_t i = 0; i != n; ++i){
      nodes[i] = chain.pop_front();
   }
   if(!chain.empty()){
      std::printf("  %s: chain not empty after popping every node\n", alloc);
      return 1;
   }
   for(std::size_t i = 0; i != n; ++i){
      unsigned char *const raw =
         reinterpret_cast<unsigned char *>(boost::movelib::iterator_to_raw_pointer(nodes[i]));
      for(std::size_t b = 0; b != sizeof(value_type); ++b){
         raw[b] = static_cast<unsigned char>(i + 1u);
      }
   }
   int err = 0;
   for(std::size_t i = 0; i != n && !err; ++i){
      const unsigned char *const raw =
         reinterpret_cast<const unsigned char *>(boost::movelib::iterator_to_raw_pointer(nodes[i]));
      for(std::size_t b = 0; b != sizeof(value_type); ++b){
         if(raw[b] != static_cast<unsigned char>(i + 1u)){
            std::printf("  %s: block %u byte %u overwritten -> blocks overlap\n", alloc,
                        static_cast<unsigned>(i), static_cast<unsigned>(b));
            err = 1;
            break;
         }
      }
   }

   //allocate_individual's blocks are individually deallocatable with deallocate_one
   for(std::size_t i = 0; i != n; ++i){
      a.deallocate_one(nodes[i]);
   }
   return err;
}

//Tests the version 2 allocator interface that containers use for nodes
template<class Allocator>
int extended_allocator_test(const char *alloc_name)
{
   typedef typename Allocator::value_type            value_type;
   typedef typename Allocator::pointer               pointer;
   typedef typename Allocator::multiallocation_chain chain_t;

   managed_heap_memory heap(allocator_test_segment_size);
   {
      Allocator a(heap.get_segment_manager());

      //Counts include 1: with a single node the first and last of a chain
      //are the same node, so a splice that mixes them still looks correct
      const std::size_t counts[] = { 1u, 2u, 3u, 8u, 33u };
      const std::size_t ncounts  = sizeof(counts)/sizeof(counts[0]);

      //allocate_one / deallocate_one, repeated so freed nodes are used again
      for(std::size_t r = 0; r != 4u; ++r){
         pointer p = a.allocate_one();
         if(!p){
            std::printf("  %s: allocate_one returned null\n", alloc_name);
            return 1;
         }
         *p = value_type();
         a.deallocate_one(p);
      }

      //allocate_individual -> deallocate_individual
      for(std::size_t i = 0; i != ncounts; ++i){
         chain_t chain;
         a.allocate_individual(counts[i], chain);
         if(check_chain<Allocator>(chain, counts[i], alloc_name, "allocate_individual"))
            return 1;
         a.deallocate_individual(chain);
      }

      //allocate_individual -> deallocate_one, with the blocks written and verified
      if(check_blocks_are_usable(a, 16u, alloc_name))
         return 1;

      //deallocate_individual leaves the chain empty, so it can be used again:
      //a stale chain would free its old nodes a second time
      {
         chain_t chain;
         for(std::size_t r = 0; r != 2u; ++r){
            a.allocate_individual(4u, chain);
            a.deallocate_individual(chain);
            if(!chain.empty()){
               std::printf("  %s: deallocate_individual did not leave the chain empty\n", alloc_name);
               return 1;
            }
         }
      }

      //A request for 0 elements does not throw and leaves the chain empty
      {
         chain_t chain;
         a.allocate_individual(0u, chain);
         if(!chain.empty()){
            std::printf("  %s: a request for 0 elements did not leave the chain empty\n", alloc_name);
            return 1;
         }
      }

      //The chain can be move constructed and move assigned
      {
         chain_t c1;
         a.allocate_individual(3u, c1);
         chain_t c2(boost::move(c1));
         chain_t c3;
         a.allocate_individual(2u, c3);
         a.deallocate_individual(c3);
         c3 = boost::move(c2);
         if(!c1.empty() || !c2.empty() || c3.size() != 3u){
            std::printf("  %s: chain move construction or assignment failed\n", alloc_name);
            return 1;
         }
         a.deallocate_individual(c3);
      }
   }
   if(!heap.check_sanity()){
      std::printf("  %s: heap is corrupted\n", alloc_name);
      return 1;
   }
   return 0;
}

//The extended interface and deallocate_free_blocks() for pool allocators
template<class Allocator>
int extended_pool_allocator_test(const char *alloc_name)
{
   typedef typename Allocator::pointer               pointer;
   typedef typename Allocator::multiallocation_chain chain_t;

   if(extended_allocator_test<Allocator>(alloc_name))
      return 1;

   //Return the empty blocks to the segment manager and allocate again
   managed_heap_memory heap(allocator_test_segment_size);
   Allocator a(heap.get_segment_manager());
   {
      chain_t chain;
      a.allocate_individual(32u, chain);
      a.deallocate_individual(chain);
   }
   a.deallocate_free_blocks();

   pointer p = a.allocate_one();
   if(!p){
      std::printf("  %s: allocate_one failed after deallocate_free_blocks\n", alloc_name);
      return 1;
   }
   a.deallocate_one(p);
   a.deallocate_free_blocks();
   return 0;
}

//The node cache of cached_node_allocator and cached_adaptive_pool. Limits
//are kept by copies and rebound copies, and is usable after trimming the cache
template<class Allocator>
int cached_allocator_test(const char *alloc_name)
{
   typedef typename Allocator::pointer                pointer;
   typedef typename Allocator::size_type              size_type;
   typedef typename Allocator::multiallocation_chain  chain_t;
   typedef typename Allocator::template rebind<allocator_compare_big_t>::other big_alloc_t;

   managed_heap_memory heap(allocator_test_segment_size);
   {
      Allocator a(heap.get_segment_manager());
      const size_type new_max = 5u;
      a.set_max_cached_nodes(new_max);
      if(a.get_max_cached_nodes() != new_max){
         std::printf("  %s: set_max_cached_nodes did not change the limit\n", alloc_name);
         return 1;
      }
      const Allocator copy(a);
      const big_alloc_t rebound(a);
      if(copy.get_max_cached_nodes() != new_max || rebound.get_max_cached_nodes() != new_max){
         std::printf("  %s: a copy does not keep the limit of cached nodes\n", alloc_name);
         return 1;
      }

      //Fill the cache, trim it with a smaller limit, then empty it
      {
         chain_t chain;
         a.allocate_individual(20u, chain);
         a.deallocate_individual(chain);
      }
      a.set_max_cached_nodes(2u);
      pointer p = a.allocate_one();
      if(!p){
         std::printf("  %s: allocate_one failed after the cache was trimmed\n", alloc_name);
         return 1;
      }
      a.deallocate_one(p);
      a.deallocate_cache();
      p = a.allocate_one();
      if(!p){
         std::printf("  %s: allocate_one failed after deallocate_cache\n", alloc_name);
         return 1;
      }
      a.deallocate_one(p);
   }
   if(!heap.check_sanity()){
      std::printf("  %s: heap is corrupted\n", alloc_name);
      return 1;
   }
   return 0;
}

}  //namespace test{
}  //namespace interprocess{
}  //namespace boost{

#include <boost/interprocess/detail/config_end.hpp>

#endif
