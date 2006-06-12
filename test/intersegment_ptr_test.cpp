//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/intersegment_ptr.hpp>
#include <boost/interprocess/managed_multi_shared_memory.hpp>
#include <boost/interprocess/mem_algo/multi_simple_seq_fit.hpp>

//Instantiation headers
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/flat_set.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/indexes/map_index.hpp>
#include <boost/interprocess/streams/bufferstream.hpp>
#include <stdio.h>
#include <deque>

using namespace boost::interprocess;

typedef flat_map_intersegment<interprocess_mutex> basic_is_ptr;
template class intersegment_ptr<int/*, basic_is_ptr*/>;
typedef class intersegment_ptr<int/*, basic_is_ptr*/> is_ptr;
template class basic_managed_multi_shared_memory
   <char
   ,multi_simple_seq_fit<mutex_family, intersegment_ptr<void/*, basic_is_ptr*/> >
   ,map_index>;
typedef basic_managed_multi_shared_memory
   <char
   ,multi_simple_seq_fit<mutex_family, intersegment_ptr<void/*, basic_is_ptr*/> >
   ,map_index> my_managed_shared_memory;
typedef allocator<int, my_managed_shared_memory::segment_manager> allocator_int_t;

//Force instantiation of several containers to catch compile-time errors
template class vector<int, allocator_int_t >;
template class flat_set<int, std::less<int>, allocator_int_t >;
template class set<int, std::less<int>, allocator_int_t >;
template class list<int, allocator_int_t>;

//Multi-segment, shared memory linked list node
struct list_node
{
   list_node(){}
   list_node(list_node* ptr, int val)
      : next(ptr), value(val){}
   intersegment_ptr<list_node>   next;
   int                           value;
};

static int slist_test()
{
   //Create shared memory
   const std::size_t segment_size = 65536/100;
   my_managed_shared_memory segment0
                           (create_only, 
                            "MySharedMemory0",//segment0 name
                            segment_size);     //segment0 size in bytes
   my_managed_shared_memory  segment1
                           (create_only, 
                            "MySharedMemory1",//segment1 name
                            segment_size);     //segment1 size in bytes

   //Create linked list shared memory
   intersegment_ptr<list_node> prev = 0, current, first;

   //Let's make sure we need more than 1 segment
   int i, max = 4*(segment_size/sizeof(list_node));

   for(i = 0; i < max; ++i, prev = current){
      //Allocate even nodes from segment0 and odds from segment1
      if((i%2)==0)
         current = static_cast<list_node*>(segment0.allocate(sizeof(list_node)));
      else
         current = static_cast<list_node*>(segment1.allocate(sizeof(list_node)));
      current->value = i;
      current->next  = 0;

      if(!prev)
         first = current;
      else
         prev->next = current;
   }

   //Communicate list to other processes
   //. . .
   //When done, destroy list
   i = 0;
   for(current = first; current; ++i){
      prev = current;
      current = current->next;
      assert(prev->value == i);
      //Deallocate even nodes from segment0 and odds from segment1
      if((i%2)==0)
         segment0.deallocate(prev.get());
      else
         segment1.deallocate(prev.get());
   }

   return 0;
}

static int named_alloc_test()
{
   my_managed_shared_memory multi_named_segment
                          (create_only, 
                           "named_object_segments",
                           10000);
   const std::size_t Name_Buf_Size = 100;
   char name[Name_Buf_Size];
   obufferstream formatter(name, Name_Buf_Size);

   try{  
      typedef std::deque<list_node*> object_stack_t;
      std::deque<list_node*> object_stack;
      for(std::size_t i = 0; i < 10000; ++i){
         formatter.seekp(0, std::ios::beg);
         formatter << "my_object" << static_cast<unsigned int>(i) << std::ends;
         object_stack.push_back
            (multi_named_segment.construct<list_node>(name)());
      }

      object_stack_t::reverse_iterator 
         rit    = object_stack.rbegin(), 
         ritend = object_stack.rend();

      for(; rit != ritend; ++rit){
         multi_named_segment.destroy_ptr(*rit);
      }
   }
   catch(const bad_alloc &){
      return 1;
   }
   return 0;   
}


int main ()
{
   try{
      if(slist_test()==-1){
         return 1;
      }

      if(named_alloc_test()==-1){
         return 1;
      }
   }
   catch(std::exception &e){
      std::cout<< e.what();
   }

   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
