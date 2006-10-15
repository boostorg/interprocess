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

#include <set>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/allocators/node_allocator.hpp>
#include <boost/interprocess/indexes/map_index.hpp>
#include "print_container.hpp"
#include "movable_int.hpp"
#include "dummy_test_allocator.hpp"
#include "set_test.hpp"
#include "map_test.hpp"

///////////////////////////////////////////////////////////////////
//                                                               //
//  This example repeats the same operations with std::set and   //
//  shmem_set using the node allocator                           //
//  and compares the values of both containers                   //
//                                                               //
///////////////////////////////////////////////////////////////////

using namespace boost::interprocess;

//Explicit instantiation to detect compilation errors
template class boost::interprocess::set
   <test::movable_and_copyable_int
   ,std::less<test::movable_and_copyable_int>
   ,test::dummy_test_allocator<test::movable_and_copyable_int> >;

template class boost::interprocess::map
   <test::movable_and_copyable_int
   ,test::movable_and_copyable_int
   ,std::less<test::movable_and_copyable_int>
   ,test::dummy_test_allocator<std::pair<const test::movable_and_copyable_int
                                        ,test::movable_and_copyable_int> > >;

template class boost::interprocess::multiset
   <test::movable_and_copyable_int
   ,std::less<test::movable_and_copyable_int>
   ,test::dummy_test_allocator<test::movable_and_copyable_int> >;

template class boost::interprocess::multimap
   <test::movable_and_copyable_int
   ,test::movable_and_copyable_int
   ,std::less<test::movable_and_copyable_int>
   ,test::dummy_test_allocator<std::pair<const test::movable_and_copyable_int
                                        ,test::movable_and_copyable_int> > >;

//Customize managed_shared_memory class
typedef basic_managed_shared_memory
   <char,
    simple_seq_fit<mutex_family>,
    map_index
   > my_managed_shared_memory;

//We will work with narrow characters for shared memory objects
//Alias <integer, 64 element per chunk> node allocator type
typedef node_allocator<int, 64, my_managed_shared_memory::segment_manager>   
   shmem_node_allocator_t;
typedef node_allocator<std::pair<const int, int>, 64, my_managed_shared_memory::segment_manager>   
   shmem_node_pair_allocator_t;
typedef node_allocator<test::movable_int, 64, my_managed_shared_memory::segment_manager>   
   shmem_movable_node_allocator_t;
typedef node_allocator<std::pair<const test::movable_int, test::movable_int>, 64, my_managed_shared_memory::segment_manager>   
   shmem_movable_node_pair_allocator_t;
typedef node_allocator<test::movable_and_copyable_int, 64, my_managed_shared_memory::segment_manager>   
   shmem_move_copy_node_allocator_t;
typedef node_allocator<std::pair<const test::movable_and_copyable_int, test::movable_and_copyable_int>, 64, my_managed_shared_memory::segment_manager>   
   shmem_move_copy_node_pair_allocator_t;

//Alias standard types
typedef std::set<int>                                          MyStdSet;
typedef std::multiset<int>                                     MyStdMultiSet;
typedef std::map<int, int>                                     MyStdMap;
typedef std::multimap<int, int>                                MyStdMultiMap;

//Alias non-movable types
typedef set<int, std::less<int>, shmem_node_allocator_t>       MyShmSet;
typedef multiset<int, std::less<int>, shmem_node_allocator_t>  MyShmMultiSet;
typedef map<int, int, std::less<int>, shmem_node_pair_allocator_t>  MyShmMap;
typedef multimap<int, int, std::less<int>, shmem_node_pair_allocator_t>  MyShmMultiMap;

//Alias movable types
typedef set<test::movable_int, std::less<test::movable_int>
            ,shmem_movable_node_allocator_t>                   MyMovableShmSet;
typedef multiset<test::movable_int, 
      std::less<test::movable_int>, 
      shmem_movable_node_allocator_t>                          MyMovableShmMultiSet;
typedef map<test::movable_int, test::movable_int, 
      std::less<test::movable_int>, 
      shmem_movable_node_pair_allocator_t>                     MyMovableShmMap;
typedef multimap<test::movable_int, test::movable_int, 
      std::less<test::movable_int>, 
      shmem_movable_node_pair_allocator_t>                     MyMovableShmMultiMap;

typedef set<test::movable_and_copyable_int, std::less<test::movable_and_copyable_int>
            ,shmem_move_copy_node_allocator_t>                 MyMoveCopyShmSet;
typedef multiset<test::movable_and_copyable_int, 
      std::less<test::movable_and_copyable_int>, 
      shmem_move_copy_node_allocator_t>                        MyMoveCopyShmMultiSet;
typedef map<test::movable_and_copyable_int, test::movable_and_copyable_int, 
      std::less<test::movable_and_copyable_int>, 
      shmem_move_copy_node_pair_allocator_t>                   MyMoveCopyShmMap;
typedef multimap<test::movable_and_copyable_int, test::movable_and_copyable_int, 
      std::less<test::movable_and_copyable_int>, 
      shmem_move_copy_node_pair_allocator_t>                   MyMoveCopyShmMultiMap;

int main ()
{
   using namespace boost::interprocess::detail;
   if(0 != test::set_test<my_managed_shared_memory
                        ,MyShmSet
                        ,MyStdSet
                        ,MyShmMultiSet
                        ,MyStdMultiSet>()){
      return 1;
   }

   if(0 != test::set_test<my_managed_shared_memory
                        ,MyMovableShmSet
                        ,MyStdSet
                        ,MyMovableShmMultiSet
                        ,MyStdMultiSet>()){
      return 1;
   }

   if(0 != test::set_test<my_managed_shared_memory
                        ,MyMoveCopyShmSet
                        ,MyStdSet
                        ,MyMoveCopyShmMultiSet
                        ,MyStdMultiSet>()){
      return 1;
   }

   if (0 != test::map_test<my_managed_shared_memory
                  ,MyShmMap
                  ,MyStdMap
                  ,MyShmMultiMap
                  ,MyStdMultiMap>()){
      return 1;
   }

//   if (0 != test::map_test<my_managed_shared_memory
//                  ,MyMovableShmMap
//                  ,MyStdMap
//                  ,MyMovableShmMultiMap
//                  ,MyStdMultiMap>()){
//      return 1;
//   }

   if (0 != test::map_test<my_managed_shared_memory
                  ,MyMoveCopyShmMap
                  ,MyStdMap
                  ,MyMoveCopyShmMultiMap
                  ,MyStdMultiMap>()){
      return 1;
   }

   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>

/*
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <cstdio>
#include <iostream>

using namespace boost::interprocess;

template<int NumDiffStrings, int NumStringRepeat, int NumTestRepeat>
int test()
{
   shared_memory_object::remove("shm");
   managed_shared_memory segment(create_only, "shm", 10000000);
   
   typedef allocator<char, managed_shared_memory::segment_manager>      char_allocator_t;
   typedef basic_string<char, std::char_traits<char>, char_allocator_t> string_t;
   typedef allocator<string_t, managed_shared_memory::segment_manager>  string_allocator_t;
   typedef multiset<string_t, std::less<string_t>, string_allocator_t>  multiset_t;
   typedef vector<string_t>                                             vector_t;

   boost::posix_time::ptime start_time, end_time;
   boost::posix_time::ptime::time_duration_type duration;
   string_allocator_t string_allocator(segment.get_segment_manager());

   multiset_t *mset = segment.construct<multiset_t>(anonymous_instance)
      (std::less<string_t>(), string_allocator);

   const char *prefix = "longlonglonglonglonglonglonglonglonglonglong prefix";

   vector_t &strings = *segment.construct<vector_t>(anonymous_instance)
      (NumDiffStrings, string_t(string_allocator));

   for(int i = 0; i < NumDiffStrings; ++i){
      char buffer[256];
      std::sprintf(buffer, "prefix%d", i);
      strings[i] = buffer;
      for(int n = 0; n < NumStringRepeat; ++n){
//         mset->insert(mset->end(), strings[i]);
         mset->insert(mset->lower_bound(strings[i]), strings[i]);
//         mset->insert(mset->upper_bound(strings[i]), strings[i]);
//         std::cout << std::endl << std::endl << std::endl << std::endl;
//         multiset_t::iterator ib(mset->begin()), ie(mset->end());
//         for(; ib != ie; ++ib){
//            std::cout << *ib << std::endl;
//         }
      }
   }

   start_time  = boost::posix_time::microsec_clock::universal_time();

   for(int n = 0; n < NumTestRepeat; ++n)
   for(int i = 0; i < NumDiffStrings; ++i){
      int count = (int)mset->count(strings[i]);
      if(count != NumStringRepeat){
         std::cout << "Error!" << std::endl;
         multiset_t::iterator ib(mset->begin()), ie(mset->end());
         for(; ib != ie; ++ib){
            std::cout << *ib << std::endl;
         }
         
         return 1;
      }
   }

   end_time = boost::posix_time::microsec_clock::universal_time();
   duration = end_time - start_time;

   std::cout << "duration: " << duration.total_seconds() << std::endl;
   return 0;
}

int main()
{
   if(0 != test<100000, 1, 50>()){
      return 1;
   }

   if(0 != test<10000, 1, 500>()){
      return 1;
   }

   if(0 != test<1000, 10, 1000>()){
      return 1;
   }

   if(0 != test<100, 100, 10000>()){
      return 1;
   }

   if(0 != test<10, 1000, 10000>()){
      return 1;
   }
   return 0;
}
*/
/*
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <set>
#include <string>
#include <cstdio>
#include <iostream>
#include <vector>

template<int NumDiffStrings, int NumStringRepeat, int NumTestRepeat>
int test()
{
   typedef std::multiset<std::string>  multiset_t;

   boost::posix_time::ptime start_time, end_time;
   boost::posix_time::ptime::time_duration_type duration;

   multiset_t mset;

   const char *prefix = "longlonglonglonglonglonglonglonglonglonglong prefix";

   std::vector<std::string> strings;
   strings.resize(NumDiffStrings);

   for(int i = 0; i < NumDiffStrings; ++i){
      char buffer[256];
      std::sprintf(buffer, "prefix%d", i);
      strings[i] = buffer;
      for(int n = 0; n < NumStringRepeat; ++n)
         mset.insert(strings[i]);
   }

   start_time  = boost::posix_time::microsec_clock::universal_time();

   for(int n = 0; n < NumTestRepeat; ++n)
   for(int i = 0; i < NumDiffStrings; ++i){
      int count = (int)mset.count(strings[i]);
      if(count != NumStringRepeat){
         std::cout << "Error!" << std::endl;
         return 1;
      }
   }

   end_time = boost::posix_time::microsec_clock::universal_time();
   duration = end_time - start_time;

   std::cout << "duration: " << duration.total_seconds() << std::endl;
   return 0;
}

int main()
{
   if(0 != test<100000, 1, 50>()){
      return 1;
   }

   if(0 != test<10000, 1, 500>()){
      return 1;
   }

   if(0 != test<1000, 10, 5000>()){
      return 1;
   }

   if(0 != test<100, 100, 5000>()){
      return 1;
   }

   if(0 != test<10, 1000, 5000>()){
      return 1;
   }
   return 0;
}
*/
/*
#include <boost/interprocess/containers/map.hpp>
#include <string>
#include <map>
#include <utility>
#include <iostream>

template<class Map>
void display(const Map &m)
{
   for(typename Map::const_iterator i = m.begin(), e = m.end(); i != e; ++i){
      std::cout << "(" << i->first << ", " << i->second << ")";
   }
   std::cout << '\n';
}

int main()
{
   using namespace boost::interprocess;
   typedef multimap<int, std::string> Map;
   typedef std::multimap<int, std::string> StdMap;
   typedef std::pair<int, std::string> value_type;
   Map m;
   StdMap stdm;
   m.insert(value_type(0, "zero"));
   m.insert(value_type(1, "one"));
   m.insert(value_type(2, "two"));
   display(m);
   m.insert(m.find(1), value_type(1, "ONE"));
   display(m);

   stdm.insert(value_type(0, "zero"));
   stdm.insert(value_type(1, "one"));
   stdm.insert(value_type(2, "two"));
   display(stdm);
   stdm.insert(stdm.find(1), value_type(1, "ONE"));
   display(stdm);
   return 0;
}
*/
