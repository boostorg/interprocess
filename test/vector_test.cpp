//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2004-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

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
#include <boost/interprocess/allocators/node_allocator.hpp>
#include <boost/interprocess/exceptions.hpp>
#include "printcontainer.hpp"
#include <memory.h>
#include "expand_bwd_test_allocator.hpp"
#include "expand_bwd_test_template.hpp"

//***************************************************************//
//                                                               //
//  This example repeats the same operations with std::vector    //
//  and interprocess::vector using the node allocator            //
//  and compares the values of both containers                   //
//                                                               //
//***************************************************************//

using namespace boost::interprocess;

//Customize managed_shared_memory class

typedef basic_managed_shared_memory
   <char,
    simple_seq_fit<mutex_family>,
    flat_map_index
   > my_managed_shared_memory;

//Alias allocator type
typedef allocator<int, my_managed_shared_memory::segment_manager>
   shmem_allocator_t;

typedef node_allocator<int, 64, my_managed_shared_memory::segment_manager>
   shmem_node_allocator_t;

//Explicit instantiation to detect compilation errors
template class boost::interprocess::vector<int, shmem_allocator_t >;
template class boost::interprocess::vector<int, shmem_node_allocator_t >;
template class boost::interprocess::vector<int, std::allocator<int> >;

//Alias vector types
typedef vector<int, shmem_node_allocator_t>     MyShmVector;
typedef std::vector<int>                        MyStdVector;

//Function to check if both sets are equal
bool CheckEqual(MyShmVector *shmvector, MyStdVector *stdvector)
{
   if(shmvector->size() != stdvector->size())
      return false;
   return std::equal(shmvector->begin(), shmvector->end(), stdvector->begin());
}


int main ()
{
   const int Memsize = 65536;
   const char *const shMemName = "MySharedMemory";
   const int max = 100;

   try{
      //Compare several shared memory vector operations with std::vector
      {
         //Create shared memory
         shared_memory_object::remove(shMemName);
         my_managed_shared_memory segment(create_only, shMemName, Memsize);

         segment.reserve_named_objects(100);

         //Shared memory allocator must be always be initialized
         //since it has no default constructor
         MyShmVector *shmvector = segment.construct<MyShmVector>("MyShmVector")
                                 (segment.get_segment_manager());

         segment.get_type(shmvector);
         segment.get_name(shmvector);

         MyStdVector *stdvector = new MyStdVector;

         int i, j;
         for(i = 0; i < max; ++i){
            shmvector->insert(shmvector->end(), i);
            stdvector->insert(stdvector->end(), i);
         }
         if(!CheckEqual(shmvector, stdvector)) return 1;

         MyShmVector::iterator it;
         MyShmVector::const_iterator cit = it;

         shmvector->erase(shmvector->begin()++);
         stdvector->erase(stdvector->begin()++);
         if(!CheckEqual(shmvector, stdvector)) return 1;

         shmvector->erase(shmvector->begin());
         stdvector->erase(stdvector->begin());
         if(!CheckEqual(shmvector, stdvector)) return 1;

         std::vector<int> aux_vect;
         #if !BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
         aux_vect.assign(50, -1);
         shmvector->insert(shmvector->end(), aux_vect.begin(), aux_vect.end());
         stdvector->insert(stdvector->end(), aux_vect.begin(), aux_vect.end());
         if(!CheckEqual(shmvector, stdvector)) return 1;
         #endif

         #if !BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
         for(i = 0, j = static_cast<int>(shmvector->size()); i < j; ++i){
            shmvector->erase(shmvector->begin());
            stdvector->erase(stdvector->begin());
         }
         if(!CheckEqual(shmvector, stdvector)) return 1;
         #endif

         #if !BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
         shmvector->insert(shmvector->begin(), aux_vect.begin(), aux_vect.end());
         shmvector->insert(shmvector->begin(), aux_vect.begin(), aux_vect.end());
         stdvector->insert(stdvector->begin(), aux_vect.begin(), aux_vect.end());
         stdvector->insert(stdvector->begin(), aux_vect.begin(), aux_vect.end());
         if(!CheckEqual(shmvector, stdvector)) return 1;
         #endif

         std::size_t size = shmvector->size();
         stdvector->insert(stdvector->end(), 50, 1);
         shmvector->insert(shmvector->end(), 50, 1);
         if(!CheckEqual(shmvector, stdvector)) return 1;

         stdvector->insert(stdvector->begin()+size/2, 50, 1);
         shmvector->insert(shmvector->begin()+size/2, 50, 1);
         if(!CheckEqual(shmvector, stdvector)) return 1;

         shmvector->erase(shmvector->begin());
         stdvector->erase(stdvector->begin());

         if(!CheckEqual(shmvector, stdvector)) return 1;

         for(i = 0; i < max; ++i){
            shmvector->insert(shmvector->begin(), i);
            stdvector->insert(stdvector->begin(), i);
         }
         if(!CheckEqual(shmvector, stdvector)) return 1;

         stdvector->assign(stdvector->size()/2, 2);
         shmvector->assign(shmvector->size()/2, 2);
         if(!CheckEqual(shmvector, stdvector)) return 1;

         stdvector->assign(shmvector->capacity()-1, 3);
         shmvector->assign(shmvector->capacity()-1, 3);
         if(!CheckEqual(shmvector, stdvector)) return 1;

         delete stdvector;
         segment.destroy<MyShmVector>("MyShmVector");
      }

      //Now test all back insertion possibilities
      {
         typedef test::expand_bwd_test_allocator<test::int_holder> allocator_type;
         typedef vector<test::int_holder, allocator_type>          vector_type;
         test::test_all_expand_bwd<vector_type>();
      }
   }
   catch(std::exception &ex){
      std::cout << ex.what() << std::endl;
      return 1;
   }
   
   std::cout << std::endl << "Test OK!" << std::endl;
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
