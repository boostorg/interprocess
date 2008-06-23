////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_TEST_MAP_TEST_HEADER
#define BOOST_INTERPROCESS_TEST_MAP_TEST_HEADER

#include <boost/interprocess/detail/config_begin.hpp>
#include "check_equal_containers.hpp"
#include <map>
#include <functional>
#include <utility>
#include "print_container.hpp"
#include <boost/interprocess/detail/move_iterator.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/iterators.hpp>
#include <string>
#include "get_process_id_name.hpp"

template<class T1, class T2, class T3, class T4>
bool operator ==(std::pair<T1, T2> &p1, std::pair<T1, T2> &p2)
{
   return p1.first == p2.first && p1.second == p2.second;
}

namespace boost{
namespace interprocess{
namespace test{

template<class ManagedSharedMemory
        ,class MyShmMap
        ,class MyStdMap
        ,class MyShmMultiMap
        ,class MyStdMultiMap>
int map_test ()
{
   typedef typename MyShmMap::key_type    IntType;
   typedef std::pair<IntType, IntType>    IntPairType;
   typedef typename MyStdMap::value_type  StdPairType;
   const int memsize = 65536;
   const char *const shMemName = test::get_process_id_name();
   const int max = 100;

   try{
      //Create shared memory
      shared_memory_object::remove(shMemName);
      ManagedSharedMemory segment(create_only, shMemName, memsize);

      segment.reserve_named_objects(100);

      //Shared memory allocator must be always be initialized
      //since it has no default constructor
      MyShmMap *shmmap = 
         segment.template construct<MyShmMap>("MyShmMap")
            (std::less<IntType>(), segment.get_segment_manager());

      MyStdMap *stdmap = new MyStdMap;

      MyShmMultiMap *shmmultimap = 
         segment.template construct<MyShmMultiMap>("MyShmMultiMap")
            (std::less<IntType>(), segment.get_segment_manager());

      MyStdMultiMap *stdmultimap = new MyStdMultiMap;

      //Test construction from a range   
      {
         //This is really nasty, but we have no other simple choice
         IntPairType aux_vect[50];
         for(int i = 0; i < 50; ++i){
            new(&aux_vect[i])IntPairType(IntType(i/2), IntType(i/2));
         }

         typedef typename MyStdMap::value_type StdValueType;
         typedef typename MyStdMap::key_type StdKeyType;
         typedef typename MyStdMap::mapped_type StdMappedType;
         StdValueType aux_vect2[50];
         for(int i = 0; i < 50; ++i){
            new(&aux_vect2[i])StdValueType(StdKeyType(i/2), StdMappedType(i/2));
         }

         IntPairType aux_vect3[50];
         for(int i = 0; i < 50; ++i){
            new(&aux_vect3[i])IntPairType(IntType(i/2), IntType(i/2));
         }

         MyShmMap *shmmap2 = 
            segment.template construct<MyShmMap>("MyShmMap2")
               (detail::make_move_iterator(&aux_vect[0])
               , detail::make_move_iterator(aux_vect + 50)
               , std::less<IntType>(), segment.get_segment_manager());

         MyStdMap *stdmap2 = new MyStdMap(aux_vect2, aux_vect2 + 50);

         MyShmMultiMap *shmmultimap2 = 
            segment.template construct<MyShmMultiMap>("MyShmMultiMap2")
               (detail::make_move_iterator(&aux_vect3[0])
               , detail::make_move_iterator(aux_vect3 + 50)
               , std::less<IntType>(), segment.get_segment_manager());

         MyStdMultiMap *stdmultimap2 = new MyStdMultiMap(aux_vect2, aux_vect2 + 50);
         if(!CheckEqualContainers(shmmap2, stdmap2)) return 1;
         if(!CheckEqualContainers(shmmultimap2, stdmultimap2)) return 1;

         segment.destroy_ptr(shmmap2);
         segment.destroy_ptr(shmmultimap2);
         delete stdmap2;
         delete stdmultimap2;
      }

      int i, j;
      for(i = 0; i < max; ++i){
         shmmap->insert(detail::move_impl(IntPairType (detail::move_impl(IntType(i)), detail::move_impl(IntType(i)))));
         stdmap->insert(StdPairType(i, i));
         shmmultimap->insert(detail::move_impl(IntPairType(detail::move_impl(IntType(i)), detail::move_impl(IntType(i)))));
         stdmultimap->insert(StdPairType(i, i));
      }

      if(!CheckEqualPairContainers(shmmap, stdmap)) return 1;
      if(!CheckEqualPairContainers(shmmultimap, stdmultimap)) return 1;

      typename MyShmMap::iterator it;
      typename MyShmMap::const_iterator cit = it;

      shmmap->erase(shmmap->begin()++);
      stdmap->erase(stdmap->begin()++);
      shmmultimap->erase(shmmultimap->begin()++);
      stdmultimap->erase(stdmultimap->begin()++);
      if(!CheckEqualPairContainers(shmmap, stdmap)) return 1;
      if(!CheckEqualPairContainers(shmmultimap, stdmultimap)) return 1;

      shmmap->erase(shmmap->begin());
      stdmap->erase(stdmap->begin());
      shmmultimap->erase(shmmultimap->begin());
      stdmultimap->erase(stdmultimap->begin());
      if(!CheckEqualPairContainers(shmmap, stdmap)) return 1;
      if(!CheckEqualPairContainers(shmmultimap, stdmultimap)) return 1;

      //Swapping test
      std::less<IntType> lessfunc;
      MyShmMap tmpshmemap2 (lessfunc, segment.get_segment_manager());
      MyStdMap tmpstdmap2;
      MyShmMultiMap tmpshmemultimap2(lessfunc, segment.get_segment_manager());
      MyStdMultiMap tmpstdmultimap2;
      shmmap->swap(tmpshmemap2);
      stdmap->swap(tmpstdmap2);
      shmmultimap->swap(tmpshmemultimap2);
      stdmultimap->swap(tmpstdmultimap2);
      shmmap->swap(tmpshmemap2);
      stdmap->swap(tmpstdmap2);
      shmmultimap->swap(tmpshmemultimap2);
      stdmultimap->swap(tmpstdmultimap2);
      if(!CheckEqualPairContainers(shmmap, stdmap)) return 1;
      if(!CheckEqualPairContainers(shmmultimap, stdmultimap)) return 1;

      //Insertion from other container
      //Initialize values
      {
         //This is really nasty, but we have no other simple choice
         IntPairType aux_vect[50];
         for(int i = 0; i < 50; ++i){
            new(&aux_vect[i])IntPairType(IntType(-1), IntType(-1));
         }
         IntPairType aux_vect3[50];
         for(int i = 0; i < 50; ++i){
            new(&aux_vect3[i])IntPairType(IntType(-1), IntType(-1));
         }

         shmmap->insert(detail::make_move_iterator(&aux_vect[0]), detail::make_move_iterator(aux_vect + 50));
         StdPairType stdpairtype(-1, -1);
         constant_iterator<StdPairType> constant_beg(stdpairtype, 50), constant_end;
         stdmap->insert(constant_beg, constant_end);
         shmmultimap->insert(detail::make_move_iterator(&aux_vect3[0]), detail::make_move_iterator(aux_vect3 + 50));
         stdmultimap->insert(constant_beg, constant_end);
         if(!CheckEqualPairContainers(shmmap, stdmap)) return 1;
         if(!CheckEqualPairContainers(shmmultimap, stdmultimap)) return 1;

         for(int i = 0, j = static_cast<int>(shmmap->size()); i < j; ++i){
            shmmap->erase(IntType(i));
            stdmap->erase(i);
            shmmultimap->erase(IntType(i));
            stdmultimap->erase(i);
         }
         if(!CheckEqualPairContainers(shmmap, stdmap)) return 1;
         if(!CheckEqualPairContainers(shmmultimap, stdmultimap)) return 1;
      }
      {
         IntPairType aux_vect[50];
         for(int i = 0; i < 50; ++i){
            new(&aux_vect[i])IntPairType(IntType(-1), IntType(-1));
         }

         IntPairType aux_vect3[50];
         for(int i = 0; i < 50; ++i){
            new(&aux_vect3[i])IntPairType(IntType(-1), IntType(-1));
         }

         IntPairType aux_vect4[50];
         for(int i = 0; i < 50; ++i){
            new(&aux_vect4[i])IntPairType(IntType(-1), IntType(-1));
         }

         IntPairType aux_vect5[50];
         for(int i = 0; i < 50; ++i){
            new(&aux_vect5[i])IntPairType(IntType(-1), IntType(-1));
         }

         shmmap->insert(detail::make_move_iterator(&aux_vect[0]), detail::make_move_iterator(aux_vect + 50));
         shmmap->insert(detail::make_move_iterator(&aux_vect3[0]), detail::make_move_iterator(aux_vect3 + 50));
         StdPairType stdpairtype(-1, -1);
         constant_iterator<StdPairType> constant_beg(stdpairtype, 50), constant_end;
         stdmap->insert(constant_beg, constant_end);
         stdmap->insert(constant_beg, constant_end);
         shmmultimap->insert(detail::make_move_iterator(&aux_vect4[0]), detail::make_move_iterator(aux_vect4 + 50));
         shmmultimap->insert(detail::make_move_iterator(&aux_vect5[0]), detail::make_move_iterator(aux_vect5 + 50));
         stdmultimap->insert(constant_beg, constant_end);
         stdmultimap->insert(constant_beg, constant_end);
         if(!CheckEqualPairContainers(shmmap, stdmap)) return 1;
         if(!CheckEqualPairContainers(shmmultimap, stdmultimap)) return 1;

         shmmap->erase(shmmap->begin()->first);
         stdmap->erase(stdmap->begin()->first);
         shmmultimap->erase(shmmultimap->begin()->first);
         stdmultimap->erase(stdmultimap->begin()->first);
         if(!CheckEqualPairContainers(shmmap, stdmap)) return 1;
         if(!CheckEqualPairContainers(shmmultimap, stdmultimap)) return 1;
      }

      for(i = 0; i < max; ++i){
         shmmap->insert(detail::move_impl(IntPairType(detail::move_impl(IntType(i)), detail::move_impl(IntType(i)))));
         stdmap->insert(StdPairType(i, i));
         shmmultimap->insert(detail::move_impl(IntPairType(detail::move_impl(IntType(i)), detail::move_impl(IntType(i)))));
         stdmultimap->insert(StdPairType(i, i));
      }

      if(!CheckEqualPairContainers(shmmap, stdmap)) return 1;
      if(!CheckEqualPairContainers(shmmultimap, stdmultimap)) return 1;

      for(i = 0; i < max; ++i){
         shmmap->insert(shmmap->begin(), detail::move_impl(IntPairType(detail::move_impl(IntType(i)), detail::move_impl(IntType(i)))));
         stdmap->insert(stdmap->begin(), StdPairType(i, i));
         //PrintContainers(shmmap, stdmap);
         shmmultimap->insert(shmmultimap->begin(), detail::move_impl(IntPairType(detail::move_impl(IntType(i)), detail::move_impl(IntType(i)))));
         stdmultimap->insert(stdmultimap->begin(), StdPairType(i, i));
         //PrintContainers(shmmultimap, stdmultimap);
         if(!CheckEqualPairContainers(shmmap, stdmap))
            return 1;
         if(!CheckEqualPairContainers(shmmultimap, stdmultimap))
            return 1;

         shmmap->insert(shmmap->end(), detail::move_impl(IntPairType(detail::move_impl(IntType(i)), detail::move_impl(IntType(i)))));
         stdmap->insert(stdmap->end(), StdPairType(i, i));
         shmmultimap->insert(shmmultimap->end(), detail::move_impl(IntPairType(detail::move_impl(IntType(i)), detail::move_impl(IntType(i)))));
         stdmultimap->insert(stdmultimap->end(), StdPairType(i, i));
         if(!CheckEqualPairContainers(shmmap, stdmap))
            return 1;
         if(!CheckEqualPairContainers(shmmultimap, stdmultimap))
            return 1;

         shmmap->insert(shmmap->lower_bound(IntType(i)), detail::move_impl(IntPairType(detail::move_impl(IntType(i)), detail::move_impl(IntType(i)))));
         stdmap->insert(stdmap->lower_bound(i), StdPairType(i, i));
         //PrintContainers(shmmap, stdmap);
         shmmultimap->insert(shmmultimap->lower_bound(IntType(i)), detail::move_impl(IntPairType(detail::move_impl(IntType(i)), detail::move_impl(IntType(i)))));
         stdmultimap->insert(stdmultimap->lower_bound(i), StdPairType(i, i));
         //PrintContainers(shmmultimap, stdmultimap);
         if(!CheckEqualPairContainers(shmmap, stdmap))
            return 1;
         if(!CheckEqualPairContainers(shmmultimap, stdmultimap))
            return 1;
         shmmap->insert(shmmap->upper_bound(IntType(i)), detail::move_impl(IntPairType(detail::move_impl(IntType(i)), detail::move_impl(IntType(i)))));
         stdmap->insert(stdmap->upper_bound(i), StdPairType(i, i));
         //PrintContainers(shmmap, stdmap);
         shmmultimap->insert(shmmultimap->upper_bound(IntType(i)), detail::move_impl(IntPairType(detail::move_impl(IntType(i)), detail::move_impl(IntType(i)))));
         stdmultimap->insert(stdmultimap->upper_bound(i), StdPairType(i, i));
         //PrintContainers(shmmultimap, stdmultimap);
         if(!CheckEqualPairContainers(shmmap, stdmap))
            return 1;
         if(!CheckEqualPairContainers(shmmultimap, stdmultimap))
            return 1;
      }

      //Compare count with std containers
      for(i = 0; i < max; ++i){
         if(shmmap->count(IntType(i)) != stdmap->count(i)){
            return -1;
         }

         if(shmmultimap->count(IntType(i)) != stdmultimap->count(i)){
            return -1;
         }
      }

      //Now do count exercise
      shmmap->erase(shmmap->begin(), shmmap->end());
      shmmultimap->erase(shmmultimap->begin(), shmmultimap->end());
      shmmap->clear();
      shmmultimap->clear();

      for(j = 0; j < 3; ++j)
      for(i = 0; i < 100; ++i){
         shmmap->insert(detail::move_impl(IntPairType(detail::move_impl(IntType(i)), detail::move_impl(IntType(i)))));
         shmmultimap->insert(detail::move_impl(IntPairType(detail::move_impl(IntType(i)), detail::move_impl(IntType(i)))));
         if(shmmap->count(IntType(i)) != typename MyShmMultiMap::size_type(1))
            return 1;
         if(shmmultimap->count(IntType(i)) != typename MyShmMultiMap::size_type(j+1))
            return 1;
      }

      segment.template destroy<MyShmMap>("MyShmMap");
      delete stdmap;
      segment.destroy_ptr(shmmultimap);
      delete stdmultimap;

      segment.shrink_to_fit_indexes();

      if(!segment.all_memory_deallocated())
         return 1;
   }
   catch(...){
      shared_memory_object::remove(shMemName);
      throw;
   }
   shared_memory_object::remove(shMemName);
   return 0;
}

template<class ManagedSharedMemory
        ,class MyShmMap
        ,class MyStdMap
        ,class MyShmMultiMap
        ,class MyStdMultiMap>
int map_test_copyable ()
{
   typedef typename MyShmMap::key_type    IntType;
   typedef std::pair<IntType, IntType>    IntPairType;
   typedef typename MyStdMap::value_type  StdPairType;

   const int memsize = 65536;
   const char *const shMemName = test::get_process_id_name();
   const int max = 100;

   try{
   //Create shared memory
   shared_memory_object::remove(shMemName);
   ManagedSharedMemory segment(create_only, shMemName, memsize);

   segment.reserve_named_objects(100);

   //Shared memory allocator must be always be initialized
   //since it has no default constructor
   MyShmMap *shmmap = 
      segment.template construct<MyShmMap>("MyShmMap")
         (std::less<IntType>(), segment.get_segment_manager());

   MyStdMap *stdmap = new MyStdMap;

   MyShmMultiMap *shmmultimap = 
      segment.template construct<MyShmMultiMap>("MyShmMultiMap")
         (std::less<IntType>(), segment.get_segment_manager());

   MyStdMultiMap *stdmultimap = new MyStdMultiMap;

   int i;
   for(i = 0; i < max; ++i){
      shmmap->insert(detail::move_impl(IntPairType(detail::move_impl(IntType(i)), detail::move_impl(IntType(i)))));
      stdmap->insert(StdPairType(i, i));
      shmmultimap->insert(detail::move_impl(IntPairType(detail::move_impl(IntType(i)), detail::move_impl(IntType(i)))));
      stdmultimap->insert(StdPairType(i, i));
   }
   if(!CheckEqualContainers(shmmap, stdmap)) return 1;
   if(!CheckEqualContainers(shmmultimap, stdmultimap)) return 1;

      {
         //Now, test copy constructor
         MyShmMap shmmapcopy(*shmmap);
         MyStdMap stdmapcopy(*stdmap);
         MyShmMultiMap shmmmapcopy(*shmmultimap);
         MyStdMultiMap stdmmapcopy(*stdmultimap);

         if(!CheckEqualContainers(&shmmapcopy, &stdmapcopy))
            return 1;
         if(!CheckEqualContainers(&shmmmapcopy, &stdmmapcopy))
            return 1;

         //And now assignment
         shmmapcopy  = *shmmap;
         stdmapcopy  = *stdmap;
         shmmmapcopy = *shmmultimap;
         stdmmapcopy = *stdmultimap;
         
         if(!CheckEqualContainers(&shmmapcopy, &stdmapcopy))
            return 1;
         if(!CheckEqualContainers(&shmmmapcopy, &stdmmapcopy))
            return 1;
         segment.destroy_ptr(shmmap);
         segment.destroy_ptr(shmmultimap);
      }
      segment.shrink_to_fit_indexes();

      if(!segment.all_memory_deallocated())
         return 1;
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

#endif   //#ifndef BOOST_INTERPROCESS_TEST_MAP_TEST_HEADER
