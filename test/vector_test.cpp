//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2004-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/detail/config_begin.hpp>
#include <algorithm>
#include <memory>
#include <vector>
#include <vector>
#include <iostream>
#include <functional>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/indexes/flat_map_index.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include "allocator_v1.hpp"
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/move_iterator.hpp>
#include <boost/interprocess/detail/move.hpp>
#include "print_container.hpp"
#include "check_equal_containers.hpp"
#include "movable_int.hpp"
#include <memory.h>
#include "expand_bwd_test_allocator.hpp"
#include "expand_bwd_test_template.hpp"
#include "dummy_test_allocator.hpp"
#include <boost/type_traits/integral_constant.hpp>

using namespace boost::interprocess;

typedef basic_managed_shared_memory
   <char,
   simple_seq_fit<mutex_family>,
   flat_map_index
   > managed_shared_memory_t;

//Explicit instantiation to detect compilation errors
template class boost::interprocess::vector<test::movable_and_copyable_int, 
   test::dummy_test_allocator<test::movable_and_copyable_int> >;

template<class V1, class V2>
bool copyable_only(V1 *shmvector, V2 *stdvector, boost::false_type)
{
   return true;
}

//Function to check if both sets are equal
template<class V1, class V2>
bool copyable_only(V1 *shmvector, V2 *stdvector, boost::true_type)
{
   typedef typename V1::value_type IntType;
   std::size_t size = shmvector->size();
   stdvector->insert(stdvector->end(), 50, 1);
   shmvector->insert(shmvector->end(), 50, 1);
   if(!test::CheckEqualContainers(shmvector, stdvector)) return false;

   {
   IntType move_me(1);
   stdvector->insert(stdvector->begin()+size/2, 50, 1);
   shmvector->insert(shmvector->begin()+size/2, 50, move(move_me));
   if(!test::CheckEqualContainers(shmvector, stdvector)) return false;
   }
   {
   IntType move_me(2);
   shmvector->assign(shmvector->size()/2, move(move_me));
   stdvector->assign(stdvector->size()/2, 2);
   if(!test::CheckEqualContainers(shmvector, stdvector)) return false;
   }
   {
   IntType move_me(3);
   shmvector->assign(shmvector->size()*3-1, move(move_me));
   stdvector->assign(stdvector->size()*3-1, 3);
   if(!test::CheckEqualContainers(shmvector, stdvector)) return false;
   }
   return true;
}

template<class IntType, template<class T, class SegmentManager> class AllocatorType >
bool do_test()
{
   //Customize managed_shared_memory class
   typedef basic_managed_shared_memory
      <char,
      //simple_seq_fit<mutex_family>,
      rbtree_best_fit<mutex_family>,
      flat_map_index
      > my_managed_shared_memory;

   //Alias AllocatorType type
   typedef AllocatorType<IntType, my_managed_shared_memory::segment_manager>
      shmem_allocator_t;

   //Alias vector types
   typedef vector<IntType, shmem_allocator_t>   MyShmVector;
   typedef std::vector<int>                     MyStdVector;
   const int Memsize = 65536;
   const char *const shMemName = "MySharedMemory";
   const int max = 100;

   {
      //Compare several shared memory vector operations with std::vector
      //Create shared memory
      shared_memory_object::remove(shMemName);
      try{
         my_managed_shared_memory segment(create_only, shMemName, Memsize);

         segment.reserve_named_objects(100);

         //Shared memory allocator must be always be initialized
         //since it has no default constructor
         MyShmVector *shmvector = segment.template construct<MyShmVector>("MyShmVector")
                                 (segment.get_segment_manager());
         MyStdVector *stdvector = new MyStdVector;

         shmvector->resize(100);
         stdvector->resize(100);
         if(!test::CheckEqualContainers(shmvector, stdvector)) return false;         

         shmvector->resize(200);
         stdvector->resize(200);
         if(!test::CheckEqualContainers(shmvector, stdvector)) return false;         

         shmvector->resize(0);
         stdvector->resize(0);
         if(!test::CheckEqualContainers(shmvector, stdvector)) return false;         

         for(int i = 0; i < max; ++i){
            IntType new_int(i);
            shmvector->insert(shmvector->end(), move(new_int));
            stdvector->insert(stdvector->end(), i);
         }
         if(!test::CheckEqualContainers(shmvector, stdvector)) return false;

         typename MyShmVector::iterator it;
         typename MyShmVector::const_iterator cit = it;

         shmvector->erase(shmvector->begin()++);
         stdvector->erase(stdvector->begin()++);
         if(!test::CheckEqualContainers(shmvector, stdvector)) return false;

         shmvector->erase(shmvector->begin());
         stdvector->erase(stdvector->begin());
         if(!test::CheckEqualContainers(shmvector, stdvector)) return false;

         {
            //Initialize values
            IntType aux_vect[50];
            for(int i = 0; i < 50; ++i){
               IntType new_int(-1);
               aux_vect[i] = move(new_int);
            }
            int aux_vect2[50];
            for(int i = 0; i < 50; ++i){
               aux_vect2[i] = -1;
            }

            shmvector->insert(shmvector->end()
                              ,detail::make_move_iterator(&aux_vect[0])
                              ,detail::make_move_iterator(aux_vect + 50));
            stdvector->insert(stdvector->end(), aux_vect2, aux_vect2 + 50);
            if(!test::CheckEqualContainers(shmvector, stdvector)) return false;

            for(int i = 0, j = static_cast<int>(shmvector->size()); i < j; ++i){
               shmvector->erase(shmvector->begin());
               stdvector->erase(stdvector->begin());
            }
            if(!test::CheckEqualContainers(shmvector, stdvector)) return false;
         }
         {
            IntType aux_vect[50];
            for(int i = 0; i < 50; ++i){
               IntType new_int(-1);
               aux_vect[i] = move(new_int);
            }
            int aux_vect2[50];
            for(int i = 0; i < 50; ++i){
               aux_vect2[i] = -1;
            }
            shmvector->insert(shmvector->begin()
                              ,detail::make_move_iterator(&aux_vect[0])
                              ,detail::make_move_iterator(aux_vect + 50));
            stdvector->insert(stdvector->begin(), aux_vect2, aux_vect2 + 50);
            if(!test::CheckEqualContainers(shmvector, stdvector)) return false;
         }

         shmvector->reserve(shmvector->size()*2);
         stdvector->reserve(stdvector->size()*2);
         if(!test::CheckEqualContainers(shmvector, stdvector)) return false;

         IntType push_back_this(1);
         shmvector->push_back(move(push_back_this));
         stdvector->push_back(int(1));
         if(!test::CheckEqualContainers(shmvector, stdvector)) return false;

         if(!copyable_only(shmvector, stdvector
                        ,boost::integral_constant
                        <bool, !is_movable<IntType>::value>())){
            return false;
         }

         shmvector->erase(shmvector->begin());
         stdvector->erase(stdvector->begin());
         if(!test::CheckEqualContainers(shmvector, stdvector)) return false;

         for(int i = 0; i < max; ++i){
            IntType insert_this(i);
            shmvector->insert(shmvector->begin(), move(insert_this));
            stdvector->insert(stdvector->begin(), i);
         }
         if(!test::CheckEqualContainers(shmvector, stdvector)) return false;

         delete stdvector;
         segment.template destroy<MyShmVector>("MyShmVector");
      }
      catch(std::exception &ex){
         shared_memory_object::remove(shMemName);
         std::cout << ex.what() << std::endl;
         return false;
      }
   }
   shared_memory_object::remove(shMemName);
   std::cout << std::endl << "Test OK!" << std::endl;
   return true;
}

bool test_expand_bwd()
{
   //Now test all back insertion possibilities
   typedef test::expand_bwd_test_allocator<test::int_holder>
      int_allocator_type;
   typedef vector<test::int_holder, int_allocator_type>
      int_vector;

   if(!test::test_all_expand_bwd<int_vector>())
      return false;

   typedef test::expand_bwd_test_allocator<test::triple_int_holder>
      triple_allocator_type;

   typedef vector<test::triple_int_holder, triple_allocator_type>
      triple_int_vector;

   if(!test::test_all_expand_bwd<triple_int_vector>())
      return false;

   return true;
}

int main()
{
   if(!do_test<int, allocator>())
      return 1;

   if(!do_test<test::movable_int, allocator>())
      return 1;

   if(!do_test<test::movable_and_copyable_int, allocator>())
      return 1;

   if(!do_test<int, test::allocator_v1>())
      return 1;

   if(!test_expand_bwd())
      return 1;

   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
