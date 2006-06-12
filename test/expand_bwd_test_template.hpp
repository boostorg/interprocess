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
#include "expand_bwd_test_allocator.hpp"
#include <algorithm>

namespace boost { namespace interprocess { namespace test {

struct int_holder
{
   int_holder(int val): m_value(val){}
   int_holder(): m_value(0){}
   ~int_holder(){ m_value = 0; }
   bool operator == (const int_holder &other) const
   {  return m_value == other.m_value; }
   bool operator != (const int_holder &other) const
   {  return m_value != other.m_value; }

   int m_value;
};

//Function to check if both sets are equal
template <class Vector1, class Vector2>
static bool CheckEqualVector(const Vector1 &vector1, const Vector2 &vector2)
{
   if(vector1.size() != vector2.size())
      return false;
   return std::equal(vector1.begin(), vector1.end(), vector2.begin());
}

template<class Vector>
static bool CheckUninitializedIsZero(const Vector & v)
{
   typename Vector::size_type sz    = v.size();
   typename Vector::size_type extra = v.capacity() - v.size();
   int_holder comp;

   const int_holder *holder = &v[0] + sz;

   while(extra--){
      if(*holder++ != comp)
         return false;
   }
   return true;
}


//This function tests all the possible combinations when
//inserting data in a vector and expanding backwards
template<class IntVectorWithExpandBwdAllocator>
bool test_insert_with_expand_bwd()
{
   typedef std::vector<int_holder> IntVect;
   const int MemorySize = 1000;

   //Distance old and new buffer
   const int Offset[]      = 
      {  350,  250,  150,  150,
         150,  50,   50,   50    };
   //Insert position
   const int Position[]    = 
      {  100,  100,  100,  100,
         100,  100,  100,  100   };
   //Initial vector size
   const int InitialSize[] = 
      {  200,  200,  200,  200,
         200,  200,  200,  200   };
   //Size of the data to insert
   const int InsertSize[]  = 
      {  100,  100,  100,  200,
         300,  25,   100,  200   };
   //Number of tests
   const int Iterations    = sizeof(InsertSize)/sizeof(int);

   for(int iteration = 0; iteration < Iterations; ++iteration)
   {
      IntVect memory;
      memory.resize(MemorySize);

      IntVect initial_data;
      initial_data.resize(InitialSize[iteration]);
      for(int i = 0; i < InitialSize[iteration]; ++i){
         initial_data[i] = i;
      }

      IntVect data_to_insert;
      data_to_insert.resize(InsertSize[iteration]);
      for(int i = 0; i < InsertSize[iteration]; ++i){
         data_to_insert[i] = -i;
      }

      expand_bwd_test_allocator<int_holder> alloc
         (&memory[0], memory.size(), Offset[iteration]);
      IntVectorWithExpandBwdAllocator vector(alloc);
      vector.insert( vector.begin()
                   , initial_data.begin(), initial_data.end());
      vector.insert( vector.begin() + Position[iteration]
                   , data_to_insert.begin(), data_to_insert.end());
      initial_data.insert(initial_data.begin() + Position[iteration]
                         , data_to_insert.begin(), data_to_insert.end());
      //Now check that values are equal
      if(!CheckEqualVector(vector, initial_data)){
         std::cout << "test_assign_with_expand_bwd::CheckEqualVector failed." << std::endl 
                   << "   Class: " << typeid(IntVectorWithExpandBwdAllocator).name() << std::endl
                   << "   Iteration: " << iteration << std::endl;
         return false;
      }

      //Now check that uninitialized values are zero
      if(!CheckUninitializedIsZero(vector)){
         std::cout << "test_assign_with_expand_bwd::CheckUninitializedIsZero failed." << std::endl 
                   << "   Class: " << typeid(IntVectorWithExpandBwdAllocator).name() << std::endl
                   << "   Iteration: " << iteration << std::endl;
         return false;
      }
   }

   return true;
}

//This function tests all the possible combinations when
//inserting data in a vector and expanding backwards
template<class IntVectorWithExpandBwdAllocator>
bool test_assign_with_expand_bwd()
{
   typedef std::vector<int_holder> IntVect;
   const int MemorySize = 200;

   const int Offset[]      = { 50, 50, 50};
   const int InitialSize[] = { 25, 25, 25};
   const int AssignSize[]  = { 40, 60, 80};
   const int Iterations    = sizeof(AssignSize)/sizeof(int);

   for(int iteration = 0; iteration <Iterations; ++iteration)
   {
      IntVect memory;
      memory.resize(MemorySize);

      //Create initial data
      IntVect initial_data;
      initial_data.resize(InitialSize[iteration]);
      for(int i = 0; i < InitialSize[iteration]; ++i){
         initial_data[i] = i;
      }

      //Create data to assign
      IntVect data_to_assign;
      data_to_assign.resize(AssignSize[iteration]);
      for(int i = 0; i < AssignSize[iteration]; ++i){
         data_to_assign[i] = -i;
      }

      //Insert initial data to the vector to test
      expand_bwd_test_allocator<int_holder> alloc
         (&memory[0], memory.size(), Offset[iteration]);
      IntVectorWithExpandBwdAllocator vector(alloc);
      vector.insert( vector.begin()
                   , initial_data.begin(), initial_data.end());

      //Assign data 
      vector.assign(data_to_assign.begin(), data_to_assign.end());
      initial_data.assign(data_to_assign.begin(), data_to_assign.end());

      //Now check that values are equal
      if(!CheckEqualVector(vector, initial_data)){
         std::cout << "test_assign_with_expand_bwd::CheckEqualVector failed." << std::endl 
                   << "   Class: " << typeid(IntVectorWithExpandBwdAllocator).name() << std::endl
                   << "   Iteration: " << iteration << std::endl;
         return false;
      }

      //Now check that uninitialized values are zero
      if(!CheckUninitializedIsZero(vector)){
         std::cout << "test_assign_with_expand_bwd::CheckUninitializedIsZero failed." << std::endl 
                   << "   Class: " << typeid(IntVectorWithExpandBwdAllocator).name() << std::endl
                   << "   Iteration: " << iteration << std::endl;
         return false;
      }
   }

   return true;
}

//This function calls all tests
template<class IntVectorWithExpandBwdAllocator>
bool test_all_expand_bwd()
{
   std::cout << "Starting test_insert_with_expand_bwd." << std::endl << "  Class: "
             << typeid(IntVectorWithExpandBwdAllocator).name() << std::endl;

   if(!test_insert_with_expand_bwd<IntVectorWithExpandBwdAllocator>()){
      std::cout << "test_allocation_direct_deallocation failed. Class: "
                << typeid(IntVectorWithExpandBwdAllocator).name() << std::endl;
      return false;
   }

   std::cout << "Starting test_assign_with_expand_bwd." << std::endl << "  Class: "
             << typeid(IntVectorWithExpandBwdAllocator).name() << std::endl;

   if(!test_assign_with_expand_bwd<IntVectorWithExpandBwdAllocator>()){
      std::cout << "test_allocation_direct_deallocation failed. Class: "
                << typeid(IntVectorWithExpandBwdAllocator).name() << std::endl;
      return false;
   }

   return true;
}

}}}   //namespace boost { namespace interprocess { namespace test {

#endif   //BOOST_INTERPROCESS_TEST_ALLOCATION_TEST_TEMPLATE_HEADER

