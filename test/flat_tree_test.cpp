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
#include <boost/interprocess/containers/flat_set.hpp>
#include <boost/interprocess/containers/flat_map.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/indexes/flat_map_index.hpp>
#include "printcontainer.hpp"
#include "set_test.hpp"

/*****************************************************************/
/*                                                               */
/*  This example repeats the same operations with std::set and   */
/*  shmem_set using the node allocator                           */
/*  and compares the values of both containers                   */
/*                                                               */
/*****************************************************************/

//Explicit instantiation to detect compilation errors
template class boost::interprocess::flat_set<int, std::less<int>, std::allocator<int> >;
template class boost::interprocess::flat_multiset<int, std::less<int>, std::allocator<int> >;
template class boost::interprocess::flat_map<int, int, std::less<int>, std::allocator<std::pair<int, int> > >;
template class boost::interprocess::flat_multimap<int, int, std::less<int>, std::allocator<std::pair<int, int> > >;

using namespace boost::interprocess;

//Customize managed_shared_memory class
typedef basic_managed_shared_memory
   <char,
    simple_seq_fit<mutex_family>,
    flat_map_index
   > my_managed_shared_memory;

//Alias allocator type
typedef allocator<int, managed_shared_memory::segment_manager> 
   shmem_node_allocator_t;

//Alias set types
typedef flat_set<int, std::less<int>, shmem_node_allocator_t>        MyShmSet;
typedef std::set<int>                                                MyStdSet;
typedef flat_multiset<int, std::less<int>, shmem_node_allocator_t>   MyShmMultiSet;
typedef std::multiset<int>                                           MyStdMultiSet;

int main()
{
   using namespace boost::interprocess::test;
   return set_test<my_managed_shared_memory
                  ,MyShmSet
                  ,MyStdSet
                  ,MyShmMultiSet
                  ,MyStdMultiSet>();
}

#include <boost/interprocess/detail/config_end.hpp>
