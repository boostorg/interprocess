////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2004-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_TEST_SET_TEST_HEADER
#define BOOST_INTERPROCESS_TEST_SET_TEST_HEADER

#include <boost/interprocess/detail/config_begin.hpp>
#include "check_equal_containers.hpp"
#include <memory>
#include <set>
#include <functional>
#include "print_container.hpp"
#include <boost/interprocess/detail/move_iterator.hpp>

namespace boost{
namespace interprocess{
namespace test{

template<class ManagedSharedMemory
        ,class MyShmSet
        ,class MyStdSet
        ,class MyShmMultiSet
        ,class MyStdMultiSet>
int set_test ()
{
   typedef typename MyShmSet::value_type IntType;
   const int memsize = 65536;
   const char *const shMemName = "/MySharedMemory";
   const int max = 100;

   try{
   //Create shared memory
   shared_memory_object::remove(shMemName);
   ManagedSharedMemory segment(create_only, shMemName, memsize);

   segment.reserve_named_objects(100);

   //Shared memory allocator must be always be initialized
   //since it has no default constructor
   MyShmSet *shmset = 
      segment.template construct<MyShmSet>("MyShmSet")
         (std::less<IntType>(), segment.get_segment_manager());

   MyStdSet *stdset = new MyStdSet;

   MyShmMultiSet *shmmultiset = 
      segment.template construct<MyShmMultiSet>("MyShmMultiSet")
         (std::less<IntType>(), segment.get_segment_manager());

   MyStdMultiSet *stdmultiset = new MyStdMultiSet;

   //Test construction from a range   
   {
      IntType aux_vect[50];
      for(int i = 0; i < 50; ++i){
         IntType move_me(i/2);
         aux_vect[i] = move(move_me);
      }
      int aux_vect2[50];
      for(int i = 0; i < 50; ++i){
         aux_vect2[i] = i/2;
      }
      IntType aux_vect3[50];
      for(int i = 0; i < 50; ++i){
         IntType move_me(i/2);
         aux_vect3[i] = move(move_me);
      }

      MyShmSet *shmset2 = 
         segment.template construct<MyShmSet>("MyShmSet2")
            (detail::make_move_iterator(&aux_vect[0])
            , detail::make_move_iterator(aux_vect + 50)
            , std::less<IntType>(), segment.get_segment_manager());

      MyStdSet *stdset2 = new MyStdSet(aux_vect2, aux_vect2 + 50);

      MyShmMultiSet *shmmultiset2 = 
         segment.template construct<MyShmMultiSet>("MyShmMultiSet2")
            (detail::make_move_iterator(&aux_vect3[0])
            , detail::make_move_iterator(aux_vect3 + 50)
            , std::less<IntType>(), segment.get_segment_manager());

      MyStdMultiSet *stdmultiset2 = new MyStdMultiSet(aux_vect2, aux_vect2 + 50);
      if(!CheckEqualContainers(shmset2, stdset2)) return 1;
      if(!CheckEqualContainers(shmmultiset2, stdmultiset2)) return 1;
      segment.destroy_ptr(shmset2);
      segment.destroy_ptr(shmmultiset2);
      delete stdset2;
      delete stdmultiset2;
   }

   int i, j;
   for(i = 0; i < max; ++i){
      IntType move_me(i);
      shmset->insert(move(move_me));
      stdset->insert(i);
      IntType move_me2(i);
      if(i == 9)
         i = i;
      shmmultiset->insert(move(move_me2));
      stdmultiset->insert(i);
   }

   if(!CheckEqualContainers(shmmultiset, stdmultiset)) return 1;

   typename MyShmSet::iterator it;
   typename MyShmSet::const_iterator cit = it;

   shmset->erase(shmset->begin()++);
   stdset->erase(stdset->begin()++);
   shmmultiset->erase(shmmultiset->begin()++);
   stdmultiset->erase(stdmultiset->begin()++);
   if(!CheckEqualContainers(shmset, stdset)) return 1;
   if(!CheckEqualContainers(shmmultiset, stdmultiset)) return 1;

   shmset->erase(shmset->begin());
   stdset->erase(stdset->begin());
   shmmultiset->erase(shmmultiset->begin());
   stdmultiset->erase(stdmultiset->begin());
   if(!CheckEqualContainers(shmset, stdset)) return 1;
   if(!CheckEqualContainers(shmmultiset, stdmultiset)) return 1;

   //Swapping test
   std::less<IntType> lessfunc;
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
   if(!CheckEqualContainers(shmset, stdset)) return 1;
   if(!CheckEqualContainers(shmmultiset, stdmultiset)) return 1;

   //Insertion from other container
   //Initialize values
   {
      IntType aux_vect[50];
      for(int i = 0; i < 50; ++i){
         IntType move_me(-1);
         aux_vect[i] = move(move_me);
      }
      int aux_vect2[50];
      for(int i = 0; i < 50; ++i){
         aux_vect2[i] = -1;
      }
      IntType aux_vect3[50];
      for(int i = 0; i < 50; ++i){
         IntType move_me(-1);
         aux_vect3[i] = move(move_me);
      }

      shmset->insert(detail::make_move_iterator(&aux_vect[0]), detail::make_move_iterator(aux_vect + 50));
      stdset->insert(aux_vect2, aux_vect2 + 50);
      shmmultiset->insert(detail::make_move_iterator(&aux_vect3[0]), detail::make_move_iterator(aux_vect3 + 50));
      stdmultiset->insert(aux_vect2, aux_vect2 + 50);
      if(!CheckEqualContainers(shmset, stdset)) return 1;
      if(!CheckEqualContainers(shmmultiset, stdmultiset)) return 1;

      for(int i = 0, j = static_cast<int>(shmset->size()); i < j; ++i){
         IntType erase_me(i);
         shmset->erase(erase_me);
         stdset->erase(i);
         shmmultiset->erase(erase_me);
         stdmultiset->erase(i);
      }
      if(!CheckEqualContainers(shmset, stdset)) return 1;
      if(!CheckEqualContainers(shmmultiset, stdmultiset)) return 1;
   }
   {
      IntType aux_vect[50];
      for(int i = 0; i < 50; ++i){
         IntType move_me(-1);
         aux_vect[i] = move(move_me);
      }
      int aux_vect2[50];
      for(int i = 0; i < 50; ++i){
         aux_vect2[i] = -1;
      }
      IntType aux_vect3[50];
      for(int i = 0; i < 50; ++i){
         IntType move_me(-1);
         aux_vect3[i] = move(move_me);
      }

      IntType aux_vect4[50];
      for(int i = 0; i < 50; ++i){
         IntType move_me(-1);
         aux_vect4[i] = move(move_me);
      }

      IntType aux_vect5[50];
      for(int i = 0; i < 50; ++i){
         IntType move_me(-1);
         aux_vect5[i] = move(move_me);
      }

      shmset->insert(detail::make_move_iterator(&aux_vect[0]), detail::make_move_iterator(aux_vect + 50));
      shmset->insert(detail::make_move_iterator(&aux_vect3[0]), detail::make_move_iterator(aux_vect3 + 50));
      stdset->insert(aux_vect2, aux_vect2 + 50);
      stdset->insert(aux_vect2, aux_vect2 + 50);
      shmmultiset->insert(detail::make_move_iterator(&aux_vect4[0]), detail::make_move_iterator(aux_vect4 + 50));
      shmmultiset->insert(detail::make_move_iterator(&aux_vect5[0]), detail::make_move_iterator(aux_vect5 + 50));
      stdmultiset->insert(aux_vect2, aux_vect2 + 50);
      stdmultiset->insert(aux_vect2, aux_vect2 + 50);
      if(!CheckEqualContainers(shmset, stdset)) return 1;
      if(!CheckEqualContainers(shmmultiset, stdmultiset)) return 1;

      shmset->erase(*shmset->begin());
      stdset->erase(*stdset->begin());
      shmmultiset->erase(*shmmultiset->begin());
      stdmultiset->erase(*stdmultiset->begin());
      if(!CheckEqualContainers(shmset, stdset)) return 1;
      if(!CheckEqualContainers(shmmultiset, stdmultiset)) return 1;
   }

   for(i = 0; i < max; ++i){
      IntType move_me(i);
      shmset->insert(move(move_me));
      stdset->insert(i);
      IntType move_me2(i);
      shmmultiset->insert(move(move_me2));
      stdmultiset->insert(i);
   }

   if(!CheckEqualContainers(shmset, stdset)) return 1;
   if(!CheckEqualContainers(shmmultiset, stdmultiset)) return 1;

   for(i = 0; i < max; ++i){
      IntType move_me(i);
      shmset->insert(shmset->begin(), move(move_me));
      stdset->insert(stdset->begin(), i);
      //PrintContainers(shmset, stdset);
      IntType move_me2(i);
      shmmultiset->insert(shmmultiset->begin(), move(move_me2));
      stdmultiset->insert(stdmultiset->begin(), i);
      //PrintContainers(shmmultiset, stdmultiset);
      if(!CheckEqualContainers(shmset, stdset))
         return 1;
      if(!CheckEqualContainers(shmmultiset, stdmultiset))
         return 1;

      IntType move_me3(i);
      shmset->insert(shmset->end(), move(move_me3));
      stdset->insert(stdset->end(), i);
      IntType move_me4(i);
      shmmultiset->insert(shmmultiset->end(), move(move_me4));
      stdmultiset->insert(stdmultiset->end(), i);
      if(!CheckEqualContainers(shmset, stdset))
         return 1;
      if(!CheckEqualContainers(shmmultiset, stdmultiset))
         return 1;
      {
      IntType move_me(i);
      shmset->insert(shmset->upper_bound(move_me), move(move_me));
      stdset->insert(stdset->upper_bound(i), i);
      //PrintContainers(shmset, stdset);
      IntType move_me2(i);
      shmmultiset->insert(shmmultiset->upper_bound(move_me2), move(move_me2));
      stdmultiset->insert(stdmultiset->upper_bound(i), i);
      //PrintContainers(shmmultiset, stdmultiset);
      if(!CheckEqualContainers(shmset, stdset))
         return 1;
      if(!CheckEqualContainers(shmmultiset, stdmultiset))
         return 1;

      }
      {
      IntType move_me(i);
      shmset->insert(shmset->lower_bound(move_me), move(move_me2));
      stdset->insert(stdset->lower_bound(i), i);
      //PrintContainers(shmset, stdset);
      IntType move_me2(i);
      shmmultiset->insert(shmmultiset->lower_bound(move_me2), move(move_me2));
      stdmultiset->insert(stdmultiset->lower_bound(i), i);
      //PrintContainers(shmmultiset, stdmultiset);
      if(!CheckEqualContainers(shmset, stdset))
         return 1;
      if(!CheckEqualContainers(shmmultiset, stdmultiset))
         return 1;
      }
   }

   //Compare count with std containers
   for(i = 0; i < max; ++i){
      IntType count_me(i);
      if(shmset->count(count_me) != stdset->count(i)){
         return -1;
      }
      if(shmmultiset->count(count_me) != stdmultiset->count(i)){
         return -1;
      }
   }

   //Now do count exercise
   shmset->clear();
   shmmultiset->clear();

   for(j = 0; j < 3; ++j)
   for(i = 0; i < 100; ++i){
      IntType move_me(i);
      shmset->insert(move(move_me));
      IntType move_me2(i);
      shmmultiset->insert(move(move_me2));
      IntType count_me(i);
      if(shmset->count(count_me) != typename MyShmMultiSet::size_type(1))
         return 1;
      if(shmmultiset->count(count_me) != typename MyShmMultiSet::size_type(j+1))
         return 1;
   }

   segment.template destroy<MyShmSet>("MyShmSet");
   delete stdset;
   segment.destroy_ptr(shmmultiset);
   delete stdmultiset;
   }
   catch(...){
      shared_memory_object::remove(shMemName);
      throw;
   }
   shared_memory_object::remove(shMemName);
   return 0;
}

template<class ManagedSharedMemory
        ,class MyShmSet
        ,class MyStdSet
        ,class MyShmMultiSet
        ,class MyStdMultiSet>
int set_test_copyable ()
{
   typedef typename MyShmSet::value_type IntType;
   const int memsize = 65536;
   const char *const shMemName = "/MySharedMemory";
   const int max = 100;

   try{
   //Create shared memory
   shared_memory_object::remove(shMemName);
   ManagedSharedMemory segment(create_only, shMemName, memsize);

   segment.reserve_named_objects(100);

   //Shared memory allocator must be always be initialized
   //since it has no default constructor
   MyShmSet *shmset = 
      segment.template construct<MyShmSet>("MyShmSet")
         (std::less<IntType>(), segment.get_segment_manager());

   MyStdSet *stdset = new MyStdSet;

   MyShmMultiSet *shmmultiset = 
      segment.template construct<MyShmMultiSet>("MyShmMultiSet")
         (std::less<IntType>(), segment.get_segment_manager());

   MyStdMultiSet *stdmultiset = new MyStdMultiSet;

   int i;
   for(i = 0; i < max; ++i){
      IntType move_me(i);
      shmset->insert(move(move_me));
      stdset->insert(i);
      IntType move_me2(i);
      shmmultiset->insert(move(move_me2));
      stdmultiset->insert(i);
   }
   if(!CheckEqualContainers(shmset, stdset)) return 1;
   if(!CheckEqualContainers(shmmultiset, stdmultiset)) return 1;

   {
      //Now, test copy constructor
      MyShmSet shmsetcopy(*shmset);
      MyStdSet stdsetcopy(*stdset);
      MyShmMultiSet shmmsetcopy(*shmmultiset);
      MyStdMultiSet stdmsetcopy(*stdmultiset);

      if(!CheckEqualContainers(&shmsetcopy, &stdsetcopy))
         return 1;
      if(!CheckEqualContainers(&shmmsetcopy, &stdmsetcopy))
         return 1;

      //And now assignment
      shmsetcopy  = *shmset;
      stdsetcopy  = *stdset;
      shmmsetcopy = *shmmultiset;
      stdmsetcopy = *stdmultiset;
      
      if(!CheckEqualContainers(&shmsetcopy, &stdsetcopy))
         return 1;
      if(!CheckEqualContainers(&shmmsetcopy, &stdmsetcopy))
         return 1;
   }
   }
   catch(...){
      shared_memory_object::remove(shMemName);
      throw;
   }
   shared_memory_object::remove(shMemName);
   return 0;
}

}  //namespace test{
}  //namespace interprocess{
}  //namespace boost{

#include <boost/interprocess/detail/config_end.hpp>

#endif
