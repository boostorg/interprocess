//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_NODE_POOL_HPP
#define BOOST_INTERPROCESS_DETAIL_NODE_POOL_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#  pragma warning (disable : 4503)
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/intrusive/slist.hpp>
#include <boost/math/common_factor_ct.hpp>
#include <boost/interprocess/allocators/detail/node_tools.hpp>
#include <cstddef>
#include <functional>
#include <assert.h>

/*!\file
   Describes the real adaptive pool shared by many Interprocess adaptive pool allocators
*/

namespace boost {
namespace interprocess {
namespace detail {

//!Pooled shared memory allocator using single segregated storage. Includes
//!a reference count but the class does not delete itself, this is  
//!responsibility of user classes. Node size (NodeSize) and the number of
//!nodes allocated per chunk (NumAlloc) are known at compile time
template< class SegmentManager, std::size_t NodeSize, std::size_t NumAlloc >
class private_node_pool
{
   //Non-copyable
   private_node_pool();
   private_node_pool(const private_node_pool &);
   private_node_pool &operator=(const private_node_pool &);

   typedef typename SegmentManager::void_pointer void_pointer;
   typedef typename pointer_to_other<void_pointer, char>::type char_ptr_t;

   //A node object will hold node_t when it's not allocated
   public:
   typedef typename node_slist<void_pointer>::slist_hook_t  slist_hook_t;
   typedef typename node_slist<void_pointer>::node_t        node_t;
   typedef typename node_slist<void_pointer>::node_slist_t  free_nodes_t;

   private:
   typedef boost::intrusive::slist
      <typename slist_hook_t::template value_traits<node_t>, false > chunkslist_t;

   enum { RealNodeSize  = 
            boost::math::static_lcm<NodeSize, sizeof(node_t)>::value };
   enum { BlockSize     = 
            detail::ct_rounded_size<RealNodeSize*NumAlloc, sizeof(node_t)>::value  };
   enum { ChunkSize     = BlockSize + sizeof(node_t) };
   enum { NumNode       = NumAlloc };

   public:

   enum  {  nodes_per_chunk   = NumNode     };
   enum  {  node_size         = NodeSize        };

   //!Segment manager typedef
   typedef SegmentManager                 segment_manager;

   //!Constructor from a segment manager. Never throws
   private_node_pool(segment_manager *segment_mngr)
   :  //General purpose allocator
      mp_segment_mngr(segment_mngr),
      //Debug node count
      m_allocated(0)
   {}

   //!Destructor. Deallocates all allocated chunks. Never throws
   ~private_node_pool()
   {  priv_clear();  }

   //!Returns the segment manager. Never throws
   segment_manager* get_segment_manager()const
   {  return detail::get_pointer(mp_segment_mngr);  }

   //!Allocates array of count elements. Can throw boost::interprocess::bad_alloc
   void *allocate(std::size_t count)
   {
      std::size_t bytes = count*NodeSize;
      if(bytes > RealNodeSize)//Normal allocation, no pooling used
         return mp_segment_mngr->allocate(bytes);
      else                    //Node allocation, pooling used
         return priv_alloc_node();
   }
   
   //!Deallocates an array pointed by ptr. Never throws
   void deallocate(void *ptr, std::size_t count)
   {
      std::size_t bytes = count*NodeSize;
      if(bytes > RealNodeSize)//Normal allocation was used
         mp_segment_mngr->deallocate(ptr);
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
   {  priv_deallocate_free_chunks();   }

   std::size_t num_free_nodes()
   {  return m_freelist.size();  }

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

   struct push_in_list
   {
      push_in_list(free_nodes_t &l, typename free_nodes_t::iterator &it)
         :  slist_(l), last_it_(it)
      {}
      
      void operator()(typename free_nodes_t::pointer p) const
      {
         slist_.push_front(*p);
         if(slist_.size() == 1){ //Cache last element
            ++last_it_ = slist_.begin();
         }
      }

      private:
      free_nodes_t &slist_;
      typename free_nodes_t::iterator &last_it_;
   };

   struct is_between
      :  std::unary_function<typename free_nodes_t::value_type, bool>
   {
      is_between(const void *addr, std::size_t size)
         :  beg_((const char *)addr), end_(beg_+size)
      {}
      
      bool operator()(typename free_nodes_t::const_reference v) const
      {
         return (beg_ <= (const char *)&v && 
                 end_ >  (const char *)&v);
      }
      private:
      const char *      beg_;
      const char *      end_;
   };

   void priv_deallocate_free_chunks()
   {
      typedef typename free_nodes_t::iterator nodelist_iterator;
      typename chunkslist_t::iterator bit(m_chunklist.before_begin()),
                                      it(m_chunklist.begin()),
                                      itend(m_chunklist.end());
      free_nodes_t backup_list;
      nodelist_iterator backup_list_last = backup_list.before_begin();

      while(it != itend){
         //Collect all the nodes from the chunk pointed by it
         //and push them in the list
         free_nodes_t free_nodes;
         nodelist_iterator last_it = free_nodes.before_begin();
         const void *addr = get_chunk_from_hook(&*it);

         //Execute the algorithm and get an iterator to the last value
         m_freelist.remove_and_destroy_if
            (is_between(addr, BlockSize), push_in_list(free_nodes, last_it));

         //If the number of nodes is equal to nodes_per_chunk
         //this means that the block can be deallocated
         if(free_nodes.size() == nodes_per_chunk){
            //Unlink the nodes
            free_nodes.clear();
            it = m_chunklist.erase_after(bit);
            mp_segment_mngr->deallocate((void*)addr);
         }
         //Otherwise, insert them in the backup list, since the
         //next "remove_if" does not need to check them again.
         else{
            //Assign the iterator to the last value if necessary
            if(backup_list.empty() && !m_freelist.empty()){
               backup_list_last = last_it;
            }
            //Transfer nodes. This is constant time.
            backup_list.splice_after
               ( backup_list.before_begin()
               , free_nodes
               , free_nodes.before_begin()
               , last_it
               , free_nodes.size());
            bit = it;
            ++it;
         }
      }
      //We should have removed all the nodes from the free list
      assert(m_freelist.empty());

      //Now pass all the node to the free list again
      m_freelist.splice_after
         ( m_freelist.before_begin()
         , backup_list
         , backup_list.before_begin()
         , backup_list_last
         , backup_list.size());
   }

   //!Deallocates all used memory. Never throws
   void priv_clear()
   {
      //check for memory leaks
      assert(m_allocated==0);

      typename chunkslist_t::iterator
         it(m_chunklist.begin()), itend(m_chunklist.end()), aux;

      //We iterate though the NodeBlock list to free the memory
      while(!m_chunklist.empty()){
         void *addr = get_chunk_from_hook(&m_chunklist.front());
         m_chunklist.pop_front();
         mp_segment_mngr->deallocate(addr);
      }  
   }

   //!Allocates one node, using single segregated storage algorithm.
   //!Never throws
   node_t *priv_alloc_node()
   {
      //If there are no free nodes we allocate a new block
      if (m_freelist.empty())
         priv_alloc_chunk();

      //We take the first free node
      node_t *n = (node_t*)&m_freelist.front();
      m_freelist.pop_front();
      ++m_allocated;
      return n;
   }

   //!Deallocates one node, using single segregated storage algorithm.
   //!Never throws
   void priv_dealloc_node(void *pElem)
   {
      //We put the node at the beginning of the free node list
      node_t * to_deallocate = static_cast<node_t*>(pElem);
      m_freelist.push_front(*to_deallocate);
      assert(m_allocated>0);
      --m_allocated;
   }

   //!Allocates a chunk of nodes. Can throw boost::interprocess::bad_alloc
   void priv_alloc_chunk()
   {
      //We allocate a new NodeBlock and put it as first
      //element in the free Node list
      char *pNode = detail::char_ptr_cast(mp_segment_mngr->allocate(ChunkSize));
      char *pBlock = pNode;
      m_chunklist.push_front(get_chunk_hook(pBlock));

      //We initialize all Nodes in Node Block to insert 
      //them in the free Node list
      for(std::size_t i = 0; i < NumAlloc; ++i){
         m_freelist.push_front(*(node_t*)pNode );
         pNode   += RealNodeSize;
      }
   }

 private:
   //!Returns a reference to the chunk hook placed in the end of the chunk
   static inline node_t & get_chunk_hook (void *chunk)
   {  
      return *static_cast<node_t*>(
               static_cast<void*>((detail::char_ptr_cast(chunk)+BlockSize)));  
   }

   //!Returns the starting address of the chunk reference to the chunk hook placed in the end of the chunk
   static inline void *get_chunk_from_hook (node_t *hook)
   {  
      return static_cast<void*>((detail::char_ptr_cast(hook) - BlockSize));  
   }

 private:
   typedef typename pointer_to_other
      <void_pointer, segment_manager>::type   segment_mngr_ptr_t;

   segment_mngr_ptr_t   mp_segment_mngr;  //Segment manager
   
   chunkslist_t   m_chunklist;      //Intrusive container of chunks
   free_nodes_t   m_freelist;       //Intrusive container of free nods
   std::size_t    m_allocated;      //Used nodes for debugging
};


//!Pooled shared memory allocator using single segregated storage. Includes
//!a reference count but the class does not delete itself, this is  
//!responsibility of user classes. Node size (NodeSize) and the number of
//!nodes allocated per chunk (NumAlloc) are known at compile time
template<class SegmentManager, class Mutex,
         std::size_t NodeSize, std::size_t NumAlloc>
class shared_node_pool 
   : public private_node_pool 
      <SegmentManager, NodeSize, NumAlloc> 
{
 private:
   typedef typename SegmentManager::void_pointer void_pointer;
   typedef typename pointer_to_other<void_pointer, char>::type char_ptr_t;
   typedef private_node_pool
      <SegmentManager, 
       NodeSize, NumAlloc>                private_node_allocator_t;
 public:
   //!Segment manager typedef
   typedef SegmentManager   segment_manager;

   typedef typename private_node_allocator_t::free_nodes_t  free_nodes_t;

   //!Constructor from a segment manager. Never throws
   shared_node_pool(segment_manager *segment_mngr)
   : private_node_allocator_t(segment_mngr){}

   //!Destructor. Deallocates all allocated chunks. Never throws
  ~shared_node_pool()
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
   void allocate_nodes(const std::size_t n, free_nodes_t &nodes)
   {
      //-----------------------
      boost::interprocess::scoped_lock<Mutex> guard(m_header);
      //-----------------------
      private_node_allocator_t::allocate_nodes(n, nodes);
   }

   //!Deallocates a linked list of nodes ending in null pointer. Never throws
   void deallocate_nodes(free_nodes_t &nodes, std::size_t n)
   {
      //-----------------------
      boost::interprocess::scoped_lock<Mutex> guard(m_header);
      //-----------------------
      private_node_allocator_t::deallocate_nodes(nodes, n);
   }

   void deallocate_nodes(free_nodes_t &nodes)
   {
      //-----------------------
      boost::interprocess::scoped_lock<Mutex> guard(m_header);
      //-----------------------
      private_node_allocator_t::deallocate_nodes(nodes);
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

#endif   //#ifndef BOOST_INTERPROCESS_DETAIL_NODE_POOL_HPP
