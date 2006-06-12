////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2004-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_TEST_SET_TEST_HEADER
#define BOOST_INTERPROCESS_TEST_SET_TEST_HEADER

#include <algorithm>
#include <memory>
#include <vector>
#include <set>
#include <iostream>
#include <functional>
#include "printcontainer.hpp"

namespace boost{
namespace interprocess{
namespace test{

//Function to dump data
template<class MyShmSet
        ,class MyStdSet>
void PrintContainers(MyShmSet *shmset, MyStdSet *stdset)
{
   typename MyShmSet::iterator itshm = shmset->begin(), itshmend = shmset->end();
   typename MyStdSet::iterator itstd = stdset->begin(), itstdend = stdset->end();

   std::cout << "MyShmSet" << std::endl;
   for(; itshm != itshmend; ++itshm){
      std::cout << *itshm << std::endl;
   }
   std::cout << "MyStdSet" << *itstd << std::endl; 
  
   for(; itstd != itstdend; ++itstd){
      std::cout << *itstd << std::endl;
   }
}


//Function to check if both sets are equal
template<class MyShmSet
        ,class MyStdSet>
bool CheckEqual(MyShmSet *shmset, MyStdSet *stdset)
{
   if(shmset->size() != stdset->size())
      return false;
   return std::equal(shmset->begin(), shmset->end(), stdset->begin());
}

template<class ManagedSharedMemory
        ,class MyShmSet
        ,class MyStdSet
        ,class MyShmMultiSet
        ,class MyStdMultiSet>
int set_test ()
{
   const int memsize = 65536;
   const char *const shMemName = "/MySharedMemory";
   const int max = 100;

   //Create shared memory
   ManagedSharedMemory::remove(shMemName);
   ManagedSharedMemory segment(create_only, shMemName, memsize);

   segment.reserve_named_objects(100);

   //Shared memory allocator must be always be initialized
   //since it has no default constructor
   MyShmSet *shmset = segment.template construct<MyShmSet>("MyShmSet")
                           (std::less<int>(), segment.get_segment_manager());

   MyStdSet *stdset = new MyStdSet;

   MyShmMultiSet *shmmultiset = segment.template construct<MyShmMultiSet>("MyShmMultiSet")
                           (std::less<int>(), segment.get_segment_manager());

   MyStdMultiSet *stdmultiset = new MyStdMultiSet;

   int i, j;
   for(i = 0; i < max; ++i){
      shmset->insert(i);
      stdset->insert(i);
      shmmultiset->insert(i);
      stdmultiset->insert(i);
   }
   if(!CheckEqual(shmset, stdset)) return 1;
   if(!CheckEqual(shmmultiset, stdmultiset)) return 1;

   typename MyShmSet::iterator it;
   typename MyShmSet::const_iterator cit = it;

   shmset->erase(shmset->begin()++);
   stdset->erase(stdset->begin()++);
   shmmultiset->erase(shmmultiset->begin()++);
   stdmultiset->erase(stdmultiset->begin()++);
   if(!CheckEqual(shmset, stdset)) return 1;
   if(!CheckEqual(shmmultiset, stdmultiset)) return 1;

   shmset->erase(shmset->begin());
   stdset->erase(stdset->begin());
   shmmultiset->erase(shmmultiset->begin());
   stdmultiset->erase(stdmultiset->begin());
   if(!CheckEqual(shmset, stdset)) return 1;
   if(!CheckEqual(shmmultiset, stdmultiset)) return 1;

   //Swapping test
   std::less<int> lessfunc;
   MyShmSet tmpshmeset2 (lessfunc, segment.get_segment_manager());
   MyStdSet tmpstdset2;
   MyShmMultiSet tmpshmemultiset2(lessfunc, segment.get_segment_manager());
   MyStdMultiSet tmpstdmultiset2;
   shmset->swap(tmpshmeset2);
   stdset->swap(tmpstdset2);
   shmmultiset->swap(tmpshmemultiset2);
   stdmultiset->swap(tmpstdmultiset2);
   shmset->swap(tmpshmeset2);
   stdset->swap(tmpstdset2);
   shmmultiset->swap(tmpshmemultiset2);
   stdmultiset->swap(tmpstdmultiset2);
   if(!CheckEqual(shmset, stdset)) return 1;
   if(!CheckEqual(shmmultiset, stdmultiset)) return 1;

   //Insertion from other container
   std::vector<int> aux_vect;
   #if !BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
   aux_vect.assign(50, -1);
   shmset->insert(aux_vect.begin(), aux_vect.end());
   stdset->insert(aux_vect.begin(), aux_vect.end());
   shmmultiset->insert(aux_vect.begin(), aux_vect.end());
   stdmultiset->insert(aux_vect.begin(), aux_vect.end());
   if(!CheckEqual(shmset, stdset)) return 1;
   if(!CheckEqual(shmmultiset, stdmultiset)) return 1;
   #endif

   #if !BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
   for(i = 0, j = static_cast<int>(shmset->size()); i < j; ++i){
      shmset->erase(i);
      stdset->erase(i);
      shmmultiset->erase(i);
      stdmultiset->erase(i);
   }
   if(!CheckEqual(shmset, stdset)) return 1;
   if(!CheckEqual(shmmultiset, stdmultiset)) return 1;
   #endif

   #if !BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
   shmset->insert(aux_vect.begin(), aux_vect.end());
   shmset->insert(aux_vect.begin(), aux_vect.end());
   stdset->insert(aux_vect.begin(), aux_vect.end());
   stdset->insert(aux_vect.begin(), aux_vect.end());
   shmmultiset->insert(aux_vect.begin(), aux_vect.end());
   shmmultiset->insert(aux_vect.begin(), aux_vect.end());
   stdmultiset->insert(aux_vect.begin(), aux_vect.end());
   stdmultiset->insert(aux_vect.begin(), aux_vect.end());
   if(!CheckEqual(shmset, stdset)) return 1;
   if(!CheckEqual(shmmultiset, stdmultiset)) return 1;
   #endif

   shmset->erase(*shmset->begin());
   stdset->erase(*stdset->begin());
   shmmultiset->erase(*shmmultiset->begin());
   stdmultiset->erase(*stdmultiset->begin());
   if(!CheckEqual(shmset, stdset)) return 1;
   if(!CheckEqual(shmmultiset, stdmultiset)) return 1;

   for(i = 0; i < max; ++i){
      shmset->insert(i);
      stdset->insert(i);
      shmmultiset->insert(i);
      stdmultiset->insert(i);
   }

   if(!CheckEqual(shmset, stdset)) return 1;
   if(!CheckEqual(shmmultiset, stdmultiset)) return 1;

   for(i = 0; i < max; ++i){
      shmset->insert(shmset->begin(), i);
      stdset->insert(stdset->begin(), i);
      //PrintContainers(shmset, stdset);
      shmmultiset->insert(shmmultiset->begin(), i);
      stdmultiset->insert(stdmultiset->begin(), i);
      //PrintContainers(shmmultiset, stdmultiset);
      if(!CheckEqual(shmset, stdset))
         return 1;
      if(!CheckEqual(shmmultiset, stdmultiset))
         return 1;

      shmset->insert(shmset->end(), i);
      stdset->insert(stdset->end(), i);
      shmmultiset->insert(shmmultiset->end(), i);
      stdmultiset->insert(stdmultiset->end(), i);
      if(!CheckEqual(shmset, stdset))
         return 1;
      if(!CheckEqual(shmmultiset, stdmultiset))
         return 1;

      shmset->insert(shmset->lower_bound(i), i);
      stdset->insert(stdset->lower_bound(i), i);
      //PrintContainers(shmset, stdset);
      shmmultiset->insert(shmmultiset->lower_bound(i), i);
      stdmultiset->insert(stdmultiset->lower_bound(i), i);
      //PrintContainers(shmmultiset, stdmultiset);
      if(!CheckEqual(shmset, stdset))
         return 1;
      if(!CheckEqual(shmmultiset, stdmultiset))
         return 1;
      shmset->insert(shmset->upper_bound(i), i);
      stdset->insert(stdset->upper_bound(i), i);
      //PrintContainers(shmset, stdset);
      shmmultiset->insert(shmmultiset->upper_bound(i), i);
      stdmultiset->insert(stdmultiset->upper_bound(i), i);
      //PrintContainers(shmmultiset, stdmultiset);
      if(!CheckEqual(shmset, stdset))
         return 1;
      if(!CheckEqual(shmmultiset, stdmultiset))
         return 1;

   }

   //Compare count with std containers
   for(i = 0; i < max; ++i){
      if(shmset->count(i) != stdset->count(i)){
         return -1;
      }

      if(shmmultiset->count(i) != stdmultiset->count(i)){
         return -1;
      }
   }

   //Now do count exercise
   shmset->clear();
   shmmultiset->clear();

   for(j = 0; j < 3; ++j)
   for(i = 0; i < 100; ++i){
      shmset->insert(i);
      shmmultiset->insert(i);
      if(shmset->count(i) != typename MyShmMultiSet::size_type(1))
         return 1;
      if(shmmultiset->count(i) != typename MyShmMultiSet::size_type(j+1))
         return 1;
   }

   segment.template destroy<MyShmSet>("MyShmSet");
   delete stdset;
   segment.destroy_ptr(shmmultiset);
   delete stdmultiset;

   return 0;
}

}  //namespace test{
}  //namespace interprocess{
}  //namespace boost{

#endif
