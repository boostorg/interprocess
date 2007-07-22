//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_ADAPTIVE_NODE_POOL_HPP
#define BOOST_INTERPROCESS_DETAIL_ADAPTIVE_NODE_POOL_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#  pragma warning (disable : 4503)

#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/min_max.hpp>
#include <boost/interprocess/detail/math_functions.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/slist.hpp>
#include <boost/math/common_factor_ct.hpp>
#include <boost/interprocess/detail/type_traits.hpp>
#include <boost/interprocess/allocators/detail/node_tools.hpp>
#include <cstddef>
#include <cmath>
#include <cassert>
#include <assert.h>

/*!\file
   Describes the real adaptive pool shared by many Interprocess pool allocators
*/

namespace boost {
namespace interprocess {
namespace detail {

//!Pooled shared memory allocator using an smart adaptive pool. Includes
//!a reference count but the class does not delete itself, this is  
//!responsibility of user classes. Node size (NodeSize) and the number of
//!nodes allocated per chunk (NodesPerChunk) are known at compile time.
template<class SegmentManagerBase>
class private_adaptive_node_pool_impl
{
   //Non-copyable
   private_adaptive_node_pool_impl();
   private_adaptive_node_pool_impl(const private_adaptive_node_pool_impl &);
   private_adaptive_node_pool_impl &operator=(const private_adaptive_node_pool_impl &);

   typedef typename SegmentManagerBase::void_pointer void_pointer;

   public:
   typedef typename node_slist<void_pointer>::node_t node_t;
   typedef typename node_slist<void_pointer>::node_slist_t free_nodes_t;

   private:
   //This hook will be used to chain the memory chunks
   typedef boost::intrusive::list_base_hook
      < boost::intrusive::tag, boost::intrusive::normal_link, void_pointer> list_hook_t;

   struct chunk_info_t
      :  public list_hook_t
   {
      //An intrusive list of free node from this chunk
      free_nodes_t free_nodes;
   };

   typedef boost::intrusive::list
      <typename list_hook_t::template value_traits<chunk_info_t> > chunk_list_t;

   public:
   //!Segment manager typedef
   typedef SegmentManagerBase                 segment_manager_base_type;

   //!Constructor from a segment manager. Never throws
   private_adaptive_node_pool_impl
      ( segment_manager_base_type *segment_mngr_base, std::size_t node_size
      , std::size_t nodes_per_chunk, std::size_t max_free_chunks)
   :  m_node_size(node_size)
   ,  m_max_free_chunks(max_free_chunks)
   ,  m_real_node_size(lcm(m_node_size, sizeof(node_t)))
   ,  m_header_size(min_value(get_rounded_size(sizeof(chunk_info_t), alignment_of<max_align>::value)
                             ,get_rounded_size(sizeof(chunk_info_t), m_real_node_size)))
      //Round the size to a power of two value.
      //This is the total memory size (including payload) that we want to
      //allocate from the general-purpose allocator
   ,  m_real_chunk_alignment(upper_power_of_2(m_header_size + m_real_node_size*nodes_per_chunk))
      //This is the real number of nodes per chunk
   ,  m_real_num_node((m_real_chunk_alignment - SegmentManagerBase::PayloadPerAllocation - m_header_size)/m_real_node_size)
      //General purpose allocator
   ,  mp_segment_mngr_base(segment_mngr_base)
   ,  m_chunklist()
   ,  m_first_free_chunk(m_chunklist.end())
      //Debug node count
   ,  m_allocated(0)
   ,  m_free_chunks(0)
   {}

   //!Destructor. Deallocates all allocated chunks. Never throws
   ~private_adaptive_node_pool_impl()
   {  priv_clear();  }

   //!Returns the segment manager. Never throws
   segment_manager_base_type* get_segment_manager_base()const
   {  return detail::get_pointer(mp_segment_mngr_base);  }

   //!Allocates array of count elements. Can throw boost::interprocess::bad_alloc
   void *allocate(std::size_t count)
   {
      std::size_t bytes = count*m_node_size;
      if(bytes > m_real_node_size){//Normal allocation, no pooling used
         void *addr = mp_segment_mngr_base->allocate(bytes);
         if(!addr)   throw bad_alloc();
         return addr;
      }
      else                    //Node allocation, pooling used
         return priv_alloc_node();
   }
   
   //!Deallocates an array pointed by ptr. Never throws
   void deallocate(void *ptr, std::size_t count)
   {
      std::size_t bytes = count*m_node_size;
      if(bytes > m_real_node_size)//Normal allocation was used
         mp_segment_mngr_base->deallocate(ptr);
      else                    //Node allocation was used
         priv_dealloc_node(ptr);
   }

   //!Allocates a singly linked list of n nodes ending in null pointer. 
   //!can throw boost::interprocess::bad_alloc
   void allocate_nodes(const std::size_t n, free_nodes_t &nodes)
   {
      std::size_t i = 0;
      try{
         for(; i < n; ++i){
            nodes.push_front(*priv_alloc_node());
         }
      }
      catch(...){
         priv_deallocate_nodes(nodes, i);
         throw;
      }
   }

   //!Deallocates a linked list of nodes. Never throws
   void deallocate_nodes(free_nodes_t &nodes)
   {  priv_deallocate_nodes(nodes, nodes.size());  }

   //!Deallocates the first n nodes of a linked list of nodes. Never throws
   void deallocate_nodes(free_nodes_t &nodes, std::size_t n)
   {  priv_deallocate_nodes(nodes, n);  }

   void deallocate_free_chunks()
   {  priv_deallocate_free_chunks(0);  }

   std::size_t num_free_nodes()
   {
      typedef typename chunk_list_t::const_iterator citerator;
      std::size_t count = 0;
      citerator it (m_first_free_chunk), itend(m_chunklist.end());
      for(; it != itend; ++it){
         count += it->size();
      }
      return count;
   }

   void swap(private_adaptive_node_pool_impl &other)
   {
      std::swap(mp_segment_mngr_base, other.mp_segment_mngr_base);
      m_chunklist.swap(other.m_chunklist);
      std::swap(m_first_free_chunk, other.m_first_free_chunk);
      std::swap(m_allocated, other.m_allocated);
      std::swap(m_free_chunks, other.m_allocated);
   }

   private:

   void priv_deallocate_nodes(free_nodes_t &nodes, const std::size_t num)
   {
      assert(nodes.size() >= num);
      for(std::size_t i = 0; i < num; ++i){
         node_t *to_deallocate = &nodes.front();
         nodes.pop_front();
         deallocate(to_deallocate, 1);
      }
   }

   class chunk_destroyer;
   friend class chunk_destroyer;

   class chunk_destroyer
   {
      public:
      chunk_destroyer(segment_manager_base_type *mngr, std::size_t real_num_node)
         :  mngr_(mngr), m_real_num_node(real_num_node)
      {}

      void operator()(typename chunk_list_t::pointer to_deallocate)
      {
         std::size_t free_nodes = to_deallocate->free_nodes.size();
         (void)free_nodes;
         assert(free_nodes == m_real_num_node);
         mngr_->deallocate(detail::get_pointer(to_deallocate));
      }
      segment_manager_base_type *mngr_;
      const std::size_t m_real_num_node;
   };

   //This macro will activate invariant checking. Slow, but helpful for debugging the code.
   //#define BOOST_INTERPROCESS_ADAPTIVE_NODE_POOL_CHECK_INVARIANTS
   void priv_invariants()
   #ifdef BOOST_INTERPROCESS_ADAPTIVE_NODE_POOL_CHECK_INVARIANTS
   #undef BOOST_INTERPROCESS_ADAPTIVE_NODE_POOL_CHECK_INVARIANTS
   {
      typedef typename chunk_list_t::iterator chunk_iterator;
      //We iterate though the chunk list to free the memory
      chunk_iterator it(m_chunklist.begin()), 
                     itend(m_chunklist.end()), to_deallocate;
      for(++it; it != itend; ++it){
         chunk_iterator prev(it);
         --prev;
         std::size_t sp = prev->free_nodes.size(),
                     si = it->free_nodes.size();
         assert(sp <= si);
         (void)sp;   (void)si;
      }

      //Check that the total free nodes are correct
      it    = m_chunklist.begin();
      itend = m_chunklist.end();
      std::size_t total_free = 0;
      for(; it != itend; ++it){
         total_free += it->free_nodes.size();
      }
      assert(total_free >= m_free_chunks*m_real_num_node);

      //Check that the total totally free chunks are correct
      it    = m_chunklist.begin();
      itend = m_chunklist.end();
      total_free = 0;
      for(; it != itend; ++it){
         total_free += it->free_nodes.size() == m_real_num_node;
      }
      assert(total_free >= m_free_chunks);
      
      //The chunk pointed by m_first_free_chunk should point
      //to end or to a non-empty chunk
      if(m_first_free_chunk != m_chunklist.end()){
         std::size_t s = m_first_free_chunk->free_nodes.size();
         assert(s != 0);
      }

      //All previous nodes of m_first_free_chunk should be 0
      it    = m_chunklist.begin();
      itend = m_first_free_chunk;
      for(; it != itend; ++it){
         std::size_t s = it->free_nodes.size();
         assert(s == 0);
      }
   }
   #else
   {} //empty
   #endif

   //!Deallocates all used memory. Never throws
   void priv_clear()
   {
      //Check for memory leaks
      assert(m_allocated==0);
      priv_invariants();
      m_first_free_chunk = m_chunklist.end();
      m_chunklist.clear_and_dispose
         (chunk_destroyer(detail::get_pointer(mp_segment_mngr_base), m_real_num_node));
      m_free_chunks = 0;
   }

   chunk_info_t *priv_chunk_from_node(void *node)
   {
      return (chunk_info_t *)((std::size_t)node & std::size_t(~(m_real_chunk_alignment - 1)));
   }

   //!Allocates one node, using the adaptive pool algorithm.
   //!Never throws
   node_t *priv_alloc_node()
   {
      priv_invariants();
      //If there are no free nodes we allocate a new block
      if (m_first_free_chunk == m_chunklist.end()){ 
         priv_alloc_chunk();
         --m_first_free_chunk;
      }
      //We take the first free node since m_first_free_chunk can't be end()
      chunk_info_t &chunk_info = *m_first_free_chunk;
      assert(!chunk_info.free_nodes.empty());
      node_t *first_node = &chunk_info.free_nodes.front();
      if(chunk_info.free_nodes.size() == 1){
         ++m_first_free_chunk;
      }
      else if(chunk_info.free_nodes.size() == m_real_num_node){
         --m_free_chunks;
      }
      chunk_info.free_nodes.pop_front();
      ++m_allocated;
      priv_invariants();
      return detail::get_pointer(first_node);
   }

   //!Deallocates one node, using the adaptive pool algorithm.
   //!Never throws
   void priv_dealloc_node(void *pElem)
   {
      typedef typename chunk_list_t::iterator chunk_iterator;
      priv_invariants();
      chunk_info_t *chunk_info = priv_chunk_from_node(pElem);
      assert(chunk_info->free_nodes.size() < m_real_num_node);
      //We put the node at the beginning of the free node list
      node_t * to_deallocate = static_cast<node_t*>(pElem);
      chunk_info->free_nodes.push_front(*to_deallocate);
      chunk_iterator this_chunk(chunk_list_t::iterator_to(*chunk_info));
      chunk_iterator next_chunk(this_chunk);
      ++next_chunk;

      //If this chunk has more free nodes than the next ones,
      //we have to move the chunk in the list to maintain it ordered.
      //Check if we have to move it
      while(next_chunk != m_chunklist.end() &&
            this_chunk->free_nodes.size() > next_chunk->free_nodes.size()){
         ++next_chunk;
      }
      //Check if the chunk must be moved
      if(++chunk_iterator(this_chunk) != next_chunk){
         //Update m_first_free_chunk iterator if it was pointing to this_chunk
         if(m_first_free_chunk == this_chunk){
            ++m_first_free_chunk;
         }
         //Update m_first_free_chunk if the moved chunk crosses the empty boundary
         else if(this_chunk->free_nodes.size() == 1){
            m_first_free_chunk = chunk_list_t::iterator_to(*chunk_info);
         }
         //Now move the chunk to the new position
         m_chunklist.erase(this_chunk);
         m_chunklist.insert(next_chunk, *chunk_info);
      }
      //Update m_first_free_chunk if the chunk crosses the empty boundary
      else if(this_chunk->free_nodes.size() == 1){
         --m_first_free_chunk;
      }
   
      if(this_chunk->free_nodes.size() == m_real_num_node){
         ++m_free_chunks;
      }

      assert(m_allocated>0);
      --m_allocated;
      priv_invariants();
      priv_deallocate_free_chunks(m_max_free_chunks);
      priv_invariants();
   }

   void priv_deallocate_free_chunks(std::size_t max_free_chunks)
   {
      typedef typename chunk_list_t::iterator chunk_iterator;
      //Now check if we've reached the free nodes limit
      //and check if we have free chunks. If so, deallocate as much
      //as we can to stay below the limit
      while(m_free_chunks > max_free_chunks &&
            m_chunklist.back().free_nodes.size() == m_real_num_node){
         chunk_iterator it(--m_chunklist.end());
         if(it == m_first_free_chunk)
            ++m_first_free_chunk; //m_first_free_chunk is now equal to end()
         m_chunklist.erase_and_dispose(it, chunk_destroyer(detail::get_pointer(mp_segment_mngr_base),m_real_num_node));
         --m_free_chunks;
      }
   }

   //!Allocates a chunk of nodes. Can throw boost::interprocess::bad_alloc
   void priv_alloc_chunk()
   {
      //We allocate a new NodeBlock and put it as first
      //element in the free Node list
      std::size_t real_chunk_size = m_real_chunk_alignment - SegmentManagerBase::PayloadPerAllocation;
      char *pNode = detail::char_ptr_cast
         (mp_segment_mngr_base->allocate_aligned(real_chunk_size, m_real_chunk_alignment));
      if(!pNode)   throw bad_alloc();
      chunk_info_t *c_info = new(pNode)chunk_info_t;
      m_chunklist.push_back(*c_info);
      
      pNode += m_header_size;
      //We initialize all Nodes in Node Block to insert 
      //them in the free Node list
      for(std::size_t i = 0; i < m_real_num_node; ++i){
         c_info->free_nodes.push_front(*new (pNode) node_t);
         pNode   += m_real_node_size;
      }
      ++m_free_chunks;
   }

   private:
   typedef typename pointer_to_other
      <void_pointer, segment_manager_base_type>::type   segment_mngr_base_ptr_t;

   const std::size_t m_node_size;
   const std::size_t m_max_free_chunks;
   const std::size_t m_real_node_size;
   const std::size_t m_header_size;
   //Round the size to a power of two value.
   //This is the total memory size (including payload) that we want to
   //allocate from the general-purpose allocator
   const std::size_t m_real_chunk_alignment;
   //This is the real number of nodes per chunk
   const std::size_t m_real_num_node;
   segment_mngr_base_ptr_t                mp_segment_mngr_base;//Segment manager
   chunk_list_t                           m_chunklist;         //Intrusive chunk list
   typename chunk_list_t::iterator        m_first_free_chunk;  //Iterator to the active chunk
   std::size_t                            m_allocated;         //Used nodes for debugging
   std::size_t                            m_free_chunks;       //Free chunks
};

//!Pooled shared memory allocator using an smart adaptive pool. Includes
//!a reference count but the class does not delete itself, this is  
//!responsibility of user classes. Node size (NodeSize) and the number of
//!nodes allocated per chunk (NodesPerChunk) are known at compile time.
template< class SegmentManager
        , std::size_t NodeSize
        , std::size_t NodesPerChunk
        , std::size_t MaxFreeChunks
        >
class private_adaptive_node_pool
   :  public private_adaptive_node_pool_impl
         <typename SegmentManager::segment_manager_base_type>
{
   typedef private_adaptive_node_pool_impl
      <typename SegmentManager::segment_manager_base_type> base_t;
   //Non-copyable
   private_adaptive_node_pool();
   private_adaptive_node_pool(const private_adaptive_node_pool &);
   private_adaptive_node_pool &operator=(const private_adaptive_node_pool &);

   public:
   typedef SegmentManager segment_manager;

   static const std::size_t nodes_per_chunk = NodesPerChunk;

   //!Constructor from a segment manager. Never throws
   private_adaptive_node_pool(segment_manager *segmeng_mngr)
      :  base_t(segmeng_mngr, NodeSize, NodesPerChunk, MaxFreeChunks)
   {}

   //!Returns the segment manager. Never throws
   segment_manager* get_segment_manager() const
   {  return static_cast<segment_manager*>(base_t::get_segment_manager_base()); }
};

//!Pooled shared memory allocator using adaptive pool. Includes
//!a reference count but the class does not delete itself, this is  
//!responsibility of user classes. Node size (NodeSize) and the number of
//!nodes allocated per chunk (NodesPerChunk) are known at compile time
template< class SegmentManager
        , class Mutex
        , std::size_t NodeSize
        , std::size_t NodesPerChunk
        , std::size_t MaxFreeChunks
        >
class shared_adaptive_node_pool 
   : public private_adaptive_node_pool 
      <SegmentManager, NodeSize, NodesPerChunk, MaxFreeChunks> 
{
 private:
   typedef typename SegmentManager::void_pointer void_pointer;
   typedef private_adaptive_node_pool
      <SegmentManager, 
       NodeSize, NodesPerChunk, MaxFreeChunks> private_node_allocator_t;
 public:
   //!Segment manager typedef
   typedef SegmentManager   segment_manager;
   typedef typename private_node_allocator_t::free_nodes_t  free_nodes_t;

   //!Constructor from a segment manager. Never throws
   shared_adaptive_node_pool(segment_manager *segment_mgnr)
   : private_node_allocator_t(segment_mgnr){}

   //!Destructor. Deallocates all allocated chunks. Never throws
   ~shared_adaptive_node_pool()
   {}

   //!Allocates array of count elements. Can throw boost::interprocess::bad_alloc
   void *allocate(std::size_t count)
   {
      //-----------------------
      boost::interprocess::scoped_lock<Mutex> guard(m_header);
      //-----------------------
      return private_node_allocator_t::allocate(count);
   }
   
   //!Deallocates an array pointed by ptr. Never throws
   void deallocate(void *ptr, std::size_t count)
   {
      //-----------------------
      boost::interprocess::scoped_lock<Mutex> guard(m_header);
      //-----------------------
      private_node_allocator_t::deallocate(ptr, count);
   }

   //!Allocates a singly linked list of n nodes ending in null pointer.
   //!can throw boost::interprocess::bad_alloc
   void allocate_nodes(std::size_t n, free_nodes_t &nodes)
   {
      //-----------------------
      boost::interprocess::scoped_lock<Mutex> guard(m_header);
      //-----------------------
      return private_node_allocator_t::allocate_nodes(n, nodes);
   }

   //!Deallocates a linked list of nodes ending in null pointer. Never throws
   void deallocate_nodes(free_nodes_t &nodes, std::size_t num)
   {
      //-----------------------
      boost::interprocess::scoped_lock<Mutex> guard(m_header);
      //-----------------------
      private_node_allocator_t::deallocate_nodes(nodes, num);
   }

   //!Deallocates a linked list of nodes ending in null pointer. Never throws
   void deallocate_nodes(free_nodes_t &nodes)
   {
      //-----------------------
      boost::interprocess::scoped_lock<Mutex> guard(m_header);
      //-----------------------
      private_node_allocator_t::deallocate_nodes(nodes);
   }

   //!Deallocates all the free chunks of memory. Never throws
   void deallocate_free_chunks()
   {
      //-----------------------
      boost::interprocess::scoped_lock<Mutex> guard(m_header);
      //-----------------------
      private_node_allocator_t::deallocate_free_chunks();
   }

   //!Increments internal reference count and returns new count. Never throws
   std::size_t inc_ref_count()
   {
      //-----------------------
      boost::interprocess::scoped_lock<Mutex> guard(m_header);
      //-----------------------
      return ++m_header.m_usecount;
   }

   //!Decrements internal reference count and returns new count. Never throws
   std::size_t dec_ref_count()
   {
      //-----------------------
      boost::interprocess::scoped_lock<Mutex> guard(m_header);
      //-----------------------
      assert(m_header.m_usecount > 0);
      return --m_header.m_usecount;
   }

   private:
   //!This struct includes needed data and derives from
   //!interprocess_mutex to allow EBO when using null_mutex
   struct header_t : Mutex
   {
      std::size_t m_usecount;    //Number of attached allocators

      header_t()
      :  m_usecount(0) {}
   } m_header;
};

}  //namespace detail {
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_DETAIL_ADAPTIVE_NODE_POOL_HPP

