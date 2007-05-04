//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_NAMED_ALLOCATION_TEST_TEMPLATE_HEADER
#define BOOST_INTERPROCESS_NAMED_ALLOCATION_TEST_TEMPLATE_HEADER

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/mem_algo/rbtree_best_fit.hpp>
#include <boost/interprocess/sync/mutex_family.hpp>
#include <vector>
#include <iostream>
#include <cstdio>
#include <new>
#include <utility>

namespace boost { namespace interprocess { namespace test {

//This test allocates until there is no more memory
//and after that deallocates all in the same order
template<class ManagedMemory>
bool test_names_and_types(ManagedMemory &m)
{
   std::vector<char*> buffers;
   const int BufferLen = 100;
   char name[BufferLen];

   for(int i = 0; true; ++i){
      std::sprintf(name, "%s%010d", "prefix_name_", i);
      char *ptr = m.template construct<char>(name, std::nothrow)(i);

      if(!ptr)
         break;
      if(std::strcmp(m.get_name(ptr), name) != 0)
         return false;

      if(m.template find<char>(name).first == 0)
         return false;

      if(m.get_type(ptr) != named_type)
         return false;

      buffers.push_back(ptr);
   }

   if(m.get_num_named_objects() != buffers.size() || !m.check_sanity())
      return false;

   for(int j = 0, max = (int)buffers.size()
      ;j < max
      ;++j){
      m.destroy_ptr(buffers[j]);
   }

   if(m.get_num_named_objects() != 0 || !m.check_sanity())
      return false;
   return true;
}

//This test allocates until there is no more memory
//and after that deallocates all in the same order
template<class ManagedMemory>
bool test_direct_named_allocation_destruction(ManagedMemory &m)
{
   std::vector<char*> buffers;
   const int BufferLen = 100;
   char name[BufferLen];

   for(int i = 0; true; ++i){
      std::sprintf(name, "%s%010d", "prefix_name_", i);
      char *ptr = m.template construct<char>(name, std::nothrow)(i);
      if(!ptr)
         break;
      if(m.template find<char>(name).first == 0)
         return false;
      buffers.push_back(ptr);
   }

   if(m.get_num_named_objects() != buffers.size() || !m.check_sanity())
      return false;

   for(int j = 0, max = (int)buffers.size()
      ;j < max
      ;++j){
      m.destroy_ptr(buffers[j]);
   }

   if(m.get_num_named_objects() != 0 || !m.check_sanity())
      return false;
   return true;
}

//This test allocates until there is no more memory
//and after that deallocates all in the inverse order
template<class ManagedMemory>
bool test_named_allocation_inverse_destruction(ManagedMemory &m)
{
   std::vector<char*> buffers;
   const int BufferLen = 100;
   char name[BufferLen];

   for(int i = 0; true; ++i){
      std::sprintf(name, "%s%010d", "prefix_name_", i);
      char *ptr = m.template construct<char>(name, std::nothrow)(i);
      if(!ptr)
         break;
      buffers.push_back(ptr);
   }

   if(m.get_num_named_objects() != buffers.size() || !m.check_sanity())
      return false;

   for(int j = (int)buffers.size()
      ;j--
      ;){
      m.destroy_ptr(buffers[j]);
   }

   if(m.get_num_named_objects() != 0 || !m.check_sanity())
      return false;
   return true;
}

//This test allocates until there is no more memory
//and after that deallocates all following a pattern
template<class ManagedMemory>
bool test_named_allocation_mixed_destruction(ManagedMemory &m)
{
   std::vector<char*> buffers;
   const int BufferLen = 100;
   char name[BufferLen];

   for(int i = 0; true; ++i){
      std::sprintf(name, "%s%010d", "prefix_name_", i);
      char *ptr = m.template construct<char>(name, std::nothrow)(i);
      if(!ptr)
         break;
      buffers.push_back(ptr);
   }

   if(m.get_num_named_objects() != buffers.size() || !m.check_sanity())
      return false;

   for(int j = 0, max = (int)buffers.size()
      ;j < max
      ;++j){
      int pos = (j%4)*((int)buffers.size())/4;
      m.destroy_ptr(buffers[pos]);
      buffers.erase(buffers.begin()+pos);
   }

   if(m.get_num_named_objects() != 0 || !m.check_sanity())
      return false;
   return true;
}

//This test allocates until there is no more memory
//and after that deallocates all in the same order
template<class ManagedMemory>
bool test_inverse_named_allocation_destruction(ManagedMemory &m)
{
   std::vector<char*> buffers;
   const int BufferLen = 100;
   char name[BufferLen];
   const unsigned int FirstNumber = (unsigned int)-1;

   for(unsigned int i = 0; true; ++i){
      std::sprintf(name, "%s%010u", "prefix_name_", FirstNumber - i);
      char *ptr = m.template construct<char>(name, std::nothrow)(i);
      if(!ptr)
         break;
      buffers.push_back(ptr);
   }

   if(m.get_num_named_objects() != buffers.size() || !m.check_sanity())
      return false;

   for(unsigned int j = 0, max = (unsigned int)buffers.size()
      ;j < max
      ;++j){
      m.destroy_ptr(buffers[j]);
   }

   if(m.get_num_named_objects() != 0 || !m.check_sanity())
      return false;
   return true;
}

///This function calls all tests
template<class ManagedMemory>
bool test_all_named_allocation(ManagedMemory &m)
{
   std::cout << "Starting test_names_and_types. Class: "
             << typeid(m).name() << std::endl;

   if(!test_names_and_types(m)){
      std::cout << "test_names_and_types failed. Class: "
                << typeid(m).name() << std::endl;
      return false;
   }

   std::cout << "Starting test_direct_named_allocation_destruction. Class: "
             << typeid(m).name() << std::endl;

   if(!test_direct_named_allocation_destruction(m)){
      std::cout << "test_direct_named_allocation_destruction failed. Class: "
                << typeid(m).name() << std::endl;
      return false;
   }

   std::cout << "Starting test_named_allocation_inverse_destruction. Class: "
             << typeid(m).name() << std::endl;

   if(!test_named_allocation_inverse_destruction(m)){
      std::cout << "test_named_allocation_inverse_destruction failed. Class: "
                << typeid(m).name() << std::endl;
      return false;
   }

   std::cout << "Starting test_named_allocation_mixed_destruction. Class: "
             << typeid(m).name() << std::endl;

   if(!test_named_allocation_mixed_destruction(m)){
      std::cout << "test_named_allocation_mixed_destruction failed. Class: "
                << typeid(m).name() << std::endl;
      return false;
   }

   std::cout << "Starting test_inverse_named_allocation_destruction. Class: "
             << typeid(m).name() << std::endl;

   if(!test_inverse_named_allocation_destruction(m)){
      std::cout << "test_inverse_named_allocation_destruction failed. Class: "
                << typeid(m).name() << std::endl;
      return false;
   }

   return true;
}

//This function calls all tests
template<template <class IndexConfig> class Index>
bool test_named_allocation()
{
   using namespace boost::interprocess;
   const int memsize = 163840;
   const char *const shMemName = "MySharedMemory";
   try
   {
      //A shared memory with rbtree best fit algorithm
      typedef basic_managed_shared_memory
         <char
         ,rbtree_best_fit<mutex_family>
         ,Index
         > my_managed_shared_memory;

      //Create shared memory
      shared_memory_object::remove(shMemName);
      my_managed_shared_memory segment(create_only, shMemName, memsize);

      //Now take the segment manager and launch memory test
      if(!test::test_all_named_allocation(*segment.get_segment_manager())){
         return false;
      }
   }
   catch(...){
      shared_memory_object::remove(shMemName);
      throw;
   }
   shared_memory_object::remove(shMemName);
/*
   //Now test it with wchar_t
   try
   {
      //A shared memory with simple sequential fit algorithm
      typedef basic_managed_shared_memory
         <wchar_t
         ,rbtree_best_fit<mutex_family>
         ,Index
         > my_managed_shared_memory;

      //Create shared memory
      shared_memory_object::remove(shMemName);
      my_managed_shared_memory segment(create_only, shMemName, memsize);

      //Now take the segment manager and launch memory test
      if(!test::test_all_named_allocation(*segment.get_segment_manager())){
         return false;
      }
   }
   catch(...){
      shared_memory_object::remove(shMemName);
      throw;
   }
   shared_memory_object::remove(shMemName);
*/
   return true;
}

}}}   //namespace boost { namespace interprocess { namespace test {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_NAMED_ALLOCATION_TEST_TEMPLATE_HEADER

