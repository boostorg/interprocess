//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
//[doc_unordered_map
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

#include <boost/unordered_map.hpp>     //boost::unordered_map
#include <functional>                  //std::equal_to
#include <boost/functional/hash.hpp>   //boost::hash

int main ()
{
   using namespace boost::interprocess;
   //Erase previous shared memory with the name
   shared_memory_object::remove("MySharedMemory");

   try{
      //Create shared memory
      managed_shared_memory segment(create_only ,"MySharedMemory" ,65536);

      //Note that unordered_map<Key, MappedType>'s value_type is std::pair<const Key, MappedType>,
      //so the allocator must allocate that pair.
      typedef int    KeyType;
      typedef float  MappedType;
      typedef std::pair<const int, float> ValueType;

      //Typedef the allocator
      typedef allocator<ValueType, managed_shared_memory::segment_manager> ShmemAllocator;

      //Alias an unordered_map of ints that uses the previous STL-like allocator.
      typedef boost::unordered_map
         < KeyType               , MappedType
         , boost::hash<KeyType>  ,std::equal_to<KeyType>
         , ShmemAllocator>
      MyHashMap;

      //Construct a shared memory hash map.
      //Note that the first parameter is the initial bucket count and
      //after that, the hash function, the equality function and the allocator
      MyHashMap *myhashmap = segment.construct<MyHashMap>("MyHashMap")  //object name
         ( 3, boost::hash<int>(), std::equal_to<int>()                  //
         , segment.get_allocator<ValueType>());                         //allocator instance

      //Insert data in the hash map
      for(int i = 0; i < 100; ++i){
         myhashmap->insert(ValueType(i, (float)i));
      }
   }
   catch(...){
      shared_memory_object::remove("MySharedMemory");
      throw;
   }
   shared_memory_object::remove("MySharedMemory");
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
