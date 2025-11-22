//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2007-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#define BOOST_CONTAINER_ADAPTIVE_NODE_POOL_CHECK_INVARIANTS
#include "node_pool_test.hpp"
#include <boost/interprocess/allocators/detail/adaptive_node_pool.hpp>
#include <vector>

using namespace boost::interprocess;
typedef managed_shared_memory::segment_manager segment_manager_t;

int main ()
{
   typedef ipcdetail::private_adaptive_node_pool
      <segment_manager_t, 4, 64, 64, 5> node_pool_t;

   if(!test::test_all_node_pool<node_pool_t>())
      return 1;

   return 0;
}
/*

#include <boost/interprocess/allocators/private_adaptive_pool.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/managed_heap_memory.hpp>
#include <cstdint>

typedef boost::interprocess::managed_heap_memory::segment_manager
    segment_manager_t;

typedef boost::interprocess::map<
    uint32_t, uint32_t, std::less<uint32_t>,
    boost::interprocess::private_adaptive_pool<
        std::pair<const uint32_t, uint32_t>, segment_manager_t>>
    mymap_t;

int main() {
  boost::interprocess::managed_heap_memory heap_mem(1u << 20);

  {
    mymap_t bbmap(heap_mem.get_segment_manager());

    bbmap.emplace(1, 2);

    mymap_t bbmap2(boost::move(bbmap));
  } // <= CRASHES HERE

  return 0;
}*/