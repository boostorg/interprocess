//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_TEST_ALLOCATION_TEST_TEMPLATE_HEADER
#define BOOST_INTERPROCESS_TEST_ALLOCATION_TEST_TEMPLATE_HEADER

#include <vector>
#include <iostream>
#include <new>
#include <utility>

namespace boost { namespace interprocess { namespace test {

//This test allocates until there is no more memory
//and after that deallocates all in the inverse order
template<class Allocator>
bool test_allocation_inverse_deallocation(Allocator &a)
{
   std::vector<void*> buffers;

   for(int i = 0; true; ++i){
      void *ptr = a.allocate(i, std::nothrow);
      if(!ptr)
         break;
      buffers.push_back(ptr);
   }

   for(int j = (int)buffers.size()
      ;j--
      ;){
      a.deallocate(buffers[j]);
   }

   return a.all_memory_deallocated() && a.check_sanity();
}

//This test allocates until there is no more memory
//and after that deallocates all in the same order
template<class Allocator>
bool test_allocation_direct_deallocation(Allocator &a)
{
   std::vector<void*> buffers;

   for(int i = 0; true; ++i){
      void *ptr = a.allocate(i, std::nothrow);
      if(!ptr)
         break;
      buffers.push_back(ptr);
   }

   for(int j = 0, max = (int)buffers.size()
      ;j < max
      ;++j){
      a.deallocate(buffers[j]);
   }

   return a.all_memory_deallocated() && a.check_sanity();
}

//This test allocates until there is no more memory
//and after that deallocates all following a pattern
template<class Allocator>
bool test_allocation_mixed_deallocation(Allocator &a)
{
   std::vector<void*> buffers;

   for(int i = 0; true; ++i){
      void *ptr = a.allocate(i, std::nothrow);
      if(!ptr)
         break;
      buffers.push_back(ptr);
   }

   for(int j = 0, max = (int)buffers.size()
      ;j < max
      ;++j){
      int pos = (j%4)*((int)buffers.size())/4;
      a.deallocate(buffers[pos]);
      buffers.erase(buffers.begin()+pos);
   }

   return a.all_memory_deallocated() && a.check_sanity();
}

//This test allocates until there is no more memory
//and after that tries to shrink all the buffers to the
//half of the original size
template<class Allocator>
bool test_allocation_shrink(Allocator &a)
{
   std::vector<void*> buffers;

   //Allocate buffers with extra memory
   for(int i = 0; true; ++i){
      void *ptr = a.allocate(i*2, std::nothrow);
      if(!ptr)
         break;
      buffers.push_back(ptr);
   }

   //Now shrink to half
   for(int i = 0, max = (int)buffers.size()
      ;i < max
      ; ++i){
      std::size_t received_size;
      if(a.allocation_command( shrink_in_place | nothrow_allocation, i*2
                             , i, received_size, buffers[i]).first){
         if(received_size > std::size_t(i*2)){
            return false;
         }
         if(received_size < std::size_t(i)){
            return false;
         }
      }
   }
   
   //Deallocate it in non sequential order
   for(int j = 0, max = (int)buffers.size()
      ;j < max
      ;++j){
      int pos = (j%4)*((int)buffers.size())/4;
      a.deallocate(buffers[pos]);
      buffers.erase(buffers.begin()+pos);
   }

   return a.all_memory_deallocated() && a.check_sanity();
}

//This test allocates until there is no more memory
//and after that tries to expand all the buffers to
//avoid the wasted internal fragmentation
template<class Allocator>
bool test_allocation_expand(Allocator &a)
{
   std::vector<void*> buffers;

   //Allocate buffers with extra memory
   for(int i = 0; true; ++i){
      void *ptr = a.allocate(i, std::nothrow);
      if(!ptr)
         break;
      buffers.push_back(ptr);
   }

   //Now try to expand to the double of the size
   for(int i = 0, max = (int)buffers.size()
      ;i < max
      ;++i){
      std::size_t received_size;
      std::size_t min_size = i+1;
      std::size_t preferred_size = i*2;
      preferred_size = min_size > preferred_size ? min_size : preferred_size;

      while(a.allocation_command( expand_fwd | nothrow_allocation, min_size
                                , preferred_size, received_size, buffers[i]).first){
         //Check received size is bigger than minimum
         if(received_size < min_size){
            return false;
         }
         //Now, try to expand further
         min_size       = received_size+1;
         preferred_size = min_size*2;
      }
   }
   
   //Deallocate it in non sequential order
   for(int j = 0, max = (int)buffers.size()
      ;j < max
      ;++j){
      int pos = (j%4)*((int)buffers.size())/4;
      a.deallocate(buffers[pos]);
      buffers.erase(buffers.begin()+pos);
   }

   return a.all_memory_deallocated() && a.check_sanity();
}

//This test allocates until there is no more memory
//and after that deallocates the odd buffers to
//make room for expansions. The expansion will probably
//success since the deallocation left room for that.
template<class Allocator>
bool test_allocation_deallocation_expand(Allocator &a)
{
   std::vector<void*> buffers;

   //Allocate buffers with extra memory
   for(int i = 0; true; ++i){
      void *ptr = a.allocate(i, std::nothrow);
      if(!ptr)
         break;
      buffers.push_back(ptr);
   }

   //Now deallocate the half of the blocks
   //so expand maybe can merge new free blocks
   for(int i = 0, max = (int)buffers.size()
      ;i < max
      ;++i){
      if(i%2){
         a.deallocate(buffers[i]);
         buffers[i] = 0;
      }
   }

   //Now try to expand to the double of the size
   for(int i = 0, max = (int)buffers.size()
      ;i < max
      ;++i){
      //
      if(buffers[i]){
         std::size_t received_size;
         std::size_t min_size = i+1;
         std::size_t preferred_size = i*2;
         preferred_size = min_size > preferred_size ? min_size : preferred_size;

         while(a.allocation_command( expand_fwd | nothrow_allocation, min_size
                                   , preferred_size, received_size, buffers[i]).first){
            //Check received size is bigger than minimum
            if(received_size < min_size){
               return false;
            }
            //Now, try to expand further
            min_size       = received_size+1;
            preferred_size = min_size*2;
         }
      }
   }
   
   //Now erase null values from the vector
   buffers.erase(std::remove(buffers.begin(), buffers.end(), (void*)0)
                ,buffers.end());

   //Deallocate it in non sequential order
   for(int j = 0, max = (int)buffers.size()
      ;j < max
      ;++j){
      int pos = (j%4)*((int)buffers.size())/4;
      a.deallocate(buffers[pos]);
      buffers.erase(buffers.begin()+pos);
   }

   return a.all_memory_deallocated() && a.check_sanity();
}

//This test allocates until there is no more memory
//and after that deallocates all except the last.
//If the allocation algorithm is a bottom-up algorithm
//the last buffer will be in the end of the segment.
//Then the test will start expanding backwards, until
//the buffer fills all the memory
template<class Allocator>
bool test_allocation_with_reuse(Allocator &a)
{
   std::vector<void*> buffers;

   //Allocate buffers with extra memory
   for(int i = 0; true; ++i){
      void *ptr = a.allocate(i, std::nothrow);
      if(!ptr)
         break;
      buffers.push_back(ptr);
   }
   
   //Now deallocate all except the latest
   //Now try to expand to the double of the size
   for(int i = 0, max = (int)buffers.size() - 1
      ;i < max
      ;++i){
      a.deallocate(buffers[i]);
   }

   //Save the unique buffer and clear vector
   void *ptr = buffers.back();
   buffers.clear();

   //Now allocate with reuse
   std::size_t received_size = 0;
   for(int i = 0; true; ++i){
      std::pair<void*, bool> ret = 
         a.allocation_command( expand_bwd | nothrow_allocation, received_size+1
                             , received_size+(i+1)*2, received_size, ptr);
      if(!ret.first)
         break;
      //If we have memory, this must be a buffer reuse
      if(!ret.second)
         return 1;
      ptr = ret.first;
   }
   //There is only a single block so deallocate it
   a.deallocate(ptr);
   
   return a.all_memory_deallocated() && a.check_sanity();
}

//This function calls all tests
template<class Allocator>
bool test_all_allocation(Allocator &a)
{

   std::cout << "Starting test_allocation_direct_deallocation. Class: "
             << typeid(a).name() << std::endl;

   if(!test_allocation_direct_deallocation(a)){
      std::cout << "test_allocation_direct_deallocation failed. Class: "
                << typeid(a).name() << std::endl;
      return false;
   }

   std::cout << "Starting test_allocation_inverse_deallocation. Class: "
             << typeid(a).name() << std::endl;

   if(!test_allocation_inverse_deallocation(a)){
      std::cout << "test_allocation_inverse_deallocation failed. Class: "
                << typeid(a).name() << std::endl;
      return false;
   }

   std::cout << "Starting test_allocation_mixed_deallocation. Class: "
             << typeid(a).name() << std::endl;

   if(!test_allocation_mixed_deallocation(a)){
      std::cout << "test_allocation_mixed_deallocation failed. Class: "
                << typeid(a).name() << std::endl;
      return false;
   }

   std::cout << "Starting test_allocation_shrink. Class: "
             << typeid(a).name() << std::endl;

   if(!test_allocation_shrink(a)){
      std::cout << "test_allocation_shrink failed. Class: "
                << typeid(a).name() << std::endl;
      return false;
   }

   std::cout << "Starting test_allocation_expand. Class: "
             << typeid(a).name() << std::endl;

   if(!test_allocation_expand(a)){
      std::cout << "test_allocation_expand failed. Class: "
                << typeid(a).name() << std::endl;
      return false;
   }

   if(!test_allocation_deallocation_expand(a)){
      std::cout << "test_allocation_deallocation_expand failed. Class: "
                << typeid(a).name() << std::endl;
      return false;
   }

   if(!test_allocation_with_reuse(a)){
      std::cout << "test_allocation_with_reuse failed. Class: "
                << typeid(a).name() << std::endl;
      return false;
   }

   return true;
}

}}}   //namespace boost { namespace interprocess { namespace test {

#endif   //BOOST_INTERPROCESS_TEST_ALLOCATION_TEST_TEMPLATE_HEADER

