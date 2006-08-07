//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_NODE_ALLOCATOR_ALGO_HPP
#define BOOST_INTERPROCESS_NODE_ALLOCATOR_ALGO_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/math/common_factor_ct.hpp>
#include <cstddef>
#include <assert.h>

/*!\file
   Describes the real pool allocator shared by many Interprocess pool allocators
*/

namespace boost {

namespace interprocess {

namespace detail {

/*!Pooled shared memory allocator using single segregated storage. Includes
   a reference count but the class does not delete itself, this is  
   responsibility of user classes. Node size (NodeSize) and the number of
   nodes allocated per chunk (NumAlloc) are known at compile time*/
template<class SegmentManager, std::size_t NodeSize, std::size_t NumAlloc >
class private_node_pool
{
   //Non-copyable
   private_node_pool();
   private_node_pool(const private_node_pool &);
   private_node_pool &operator=(const private_node_pool &);

   typedef typename SegmentManager::void_pointer void_pointer;
   typedef typename pointer_to_other<void_pointer, char>::type char_ptr_t;

   enum { RealNodeSize  = 
            boost::math::static_lcm<NodeSize, sizeof(char_ptr_t)>::value };
   enum { BlockSize     = 
            detail::ct_rounded_size<RealNodeSize*NumAlloc, sizeof(char_ptr_t)>::value  };
   enum { ChunkSize     = BlockSize + sizeof(char_ptr_t) };

 public:
   /*!Segment manager typedef*/
   typedef SegmentManager                 segment_manager;

   /*!Constructor from a segment manager. Never throws*/
   private_node_pool(segment_manager *segment_mngr)
   :  //To indicate null, we set pointers to themselves
      //General purpose allocator
      mp_segment_mngr(segment_mngr),
      mp_firstBlock(0),
      mp_free(0),
      //Debug node count
      m_allocated(0),
      m_free_nodes(0)
   {}

   /*!Destructor. Deallocates all allocated chunks. Never throws*/
   ~private_node_pool()
      {  priv_clear();  }

   /*!Returns the segment manager. Never throws*/
   segment_manager* get_segment_manager()const
      {  return mp_segment_mngr.get();  }

   /*!Allocates array of count elements. Can throw boost::interprocess::bad_alloc*/
   void *allocate(std::size_t count)
   {
      std::size_t bytes = count*NodeSize;
      if(bytes > RealNodeSize)//Normal allocation, no pooling used
         return mp_segment_mngr->allocate(bytes);
      else                    //Node allocation, pooling used
         return priv_alloc_node();
   }
   
   /*!Deallocates an array pointed by ptr. Never throws*/
   void deallocate(void *ptr, std::size_t count)
   {
      std::size_t bytes = count*NodeSize;
      if(bytes > RealNodeSize)//Normal allocation was used
         mp_segment_mngr->deallocate(ptr);
      else                    //Node allocation was used
         priv_dealloc_node(ptr);
   }

   /*!Allocates a singly linked list of n nodes ending in null pointer. 
      can throw boost::interprocess::bad_alloc*/
   void *allocate_nodes(std::size_t n)
   {
      while(m_free_nodes < n){
         priv_alloc_chunk();
      }

      m_allocated  += n;
      m_free_nodes -= n;
      void *ret = mp_free.get();

      while(n--){
         //We take the first free node
         if(n == 0){
            //Last node we must mark next node as null
            char *end = mp_free.get();
            mp_free = next_node(end);
            //Mark next node of last node as null
            next_node(end) = 0;
         }
         else{
            mp_free = next_node(mp_free.get());
         }
      }
      return ret;
   }

   /*!Deallocates a linked list of nodes ending in null pointer. Never throws*/
   void deallocate_nodes(void *nodes)
   {
      char *node = char_ptr_cast(nodes);
      char_ptr_t tmp;
      while(node){
         tmp = next_node(node); 
         priv_dealloc_node(node);
         node = tmp.get();
      }
   }

 private:   

   /*!Deallocates all used memory. Never throws*/
   void priv_clear()
   {
      //check for memory leaks
      assert(m_allocated==0);

      //We iterate though the NodeBlock list to free the memory
      char_ptr_t pNextBlock;
      while(mp_firstBlock){
         pNextBlock     = priv_next_chunk(mp_firstBlock.get());
         mp_segment_mngr->deallocate(mp_firstBlock.get());
         mp_firstBlock  = pNextBlock;
      }  

      mp_firstBlock  = 0;
      mp_free        = 0;
   }

   /*!Allocates one node, using single segregated storage algorithm. 
      Never throws*/
   void *priv_alloc_node()
   {
      //If there are no free nodes we allocate a new block
      if (!mp_free) 
         priv_alloc_chunk();

      //We take the first free node
      char_ptr_t pTmp = mp_free;
      mp_free = next_node(mp_free.get());
      ++m_allocated;
      --m_free_nodes;
      return pTmp.get();
   }

   /*!Deallocates one node, using single segregated storage algorithm.
      Never throws*/
   void priv_dealloc_node(void *pElem)
   {
      //We put the node at the beginning of the free node list
      char *pTmp = detail::char_ptr_cast(pElem);
      //We must build the offset pointer in memory
      next_node(pTmp) = mp_free;
      mp_free        = pTmp;
      assert(m_allocated>0);
      --m_allocated;
      ++m_free_nodes;
   }

   /*!Allocates a chunk of nodes. Can throw boost::interprocess::bad_alloc*/
   void priv_alloc_chunk()
   {
      //We allocate a new NodeBlock and put it as first
      //element in the free Node list
      char *pNode = detail::char_ptr_cast(mp_segment_mngr->allocate(ChunkSize));
      char *pBlock = pNode;
      priv_next_chunk(pBlock) = mp_firstBlock;
      mp_firstBlock           = pBlock;

      //We initialize all Nodes in Node Block to insert 
      //them in the free Node list
      for(std::size_t i = 0; i < NumAlloc; ++i){ 
         next_node(pNode) = mp_free;
         mp_free  = pNode;
         pNode   += RealNodeSize;
      }
      m_free_nodes += NumAlloc;
   }

 public:
   /*!Returns a reference to the next node from a raw pointer pointing
      to a free node. Never throws*/
   static inline char_ptr_t &next_node    (void *node)
      {  return *static_cast<char_ptr_t*>(node);  }

 private:
   /*!Returns a reference to the next chunk from a raw pointer pointing
      to a chunk. Never throws*/
   static inline char_ptr_t & priv_next_chunk  (void *chunk)
   {  
      return *static_cast<char_ptr_t*>(
               static_cast<void*>((detail::char_ptr_cast(chunk)+BlockSize)));  
   }

 private:
   typedef typename pointer_to_other
      <void_pointer, segment_manager>::type   segment_mngr_ptr_t;

   segment_mngr_ptr_t   mp_segment_mngr;  //Segment manager
   char_ptr_t           mp_firstBlock;    //Ptr to the first block
   char_ptr_t           mp_free;          //Ptr to the first free node
   std::size_t          m_allocated;      //Used nodes for debugging
   std::size_t          m_free_nodes;     //Free nodes
};


/*!Pooled shared memory allocator using single segregated storage. Includes
   a reference count but the class does not delete itself, this is  
   responsibility of user classes. Node size (NodeSize) and the number of
   nodes allocated per chunk (NumAlloc) are known at compile time*/
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
   /*!Segment manager typedef*/
   typedef SegmentManager   segment_manager;

   /*!Constructor from a segment manager. Never throws*/
   shared_node_pool(segment_manager *segment_mngr)
   : private_node_allocator_t(segment_mngr){}

   /*!Destructor. Deallocates all allocated chunks. Never throws*/
  ~shared_node_pool()
      {}

   /*!Allocates array of count elements. Can throw boost::interprocess::bad_alloc*/
   void *allocate(std::size_t count)
   {
      //-----------------------
      boost::interprocess::scoped_lock<Mutex> guard(m_header);
      //-----------------------
      return private_node_allocator_t::allocate(count);
   }
   
   /*!Deallocates an array pointed by ptr. Never throws*/
   void deallocate(void *ptr, std::size_t count)
   {
      //-----------------------
      boost::interprocess::scoped_lock<Mutex> guard(m_header);
      //-----------------------
      private_node_allocator_t::deallocate(ptr, count);
   }

   /*!Allocates a singly linked list of n nodes ending in null pointer. 
      can throw boost::interprocess::bad_alloc*/
   void *allocate_nodes(std::size_t n)
   {
      //-----------------------
      boost::interprocess::scoped_lock<Mutex> guard(m_header);
      //-----------------------
      return private_node_allocator_t::allocate_nodes(n);
   }

   /*!Deallocates a linked list of nodes ending in null pointer. Never throws*/
   void deallocate_nodes(void * nodes)
   {
      //-----------------------
      boost::interprocess::scoped_lock<Mutex> guard(m_header);
      //-----------------------
      private_node_allocator_t::deallocate_nodes(nodes);
   }

   /*!Increments internal reference count and returns new count. Never throws*/
   std::size_t inc_ref_count()
   {
      //-----------------------
      boost::interprocess::scoped_lock<Mutex> guard(m_header);
      //-----------------------
      return ++m_header.m_usecount;
   }

   /*!Decrements internal reference count and returns new count. Never throws*/
   std::size_t dec_ref_count()
   {
      //-----------------------
      boost::interprocess::scoped_lock<Mutex> guard(m_header);
      //-----------------------
      assert(m_header.m_usecount > 0);
      return --m_header.m_usecount;
   }

 private:
   /*!This struct includes needed data and derives from
      interprocess_mutex to allow EBO when using null interprocess_mutex*/
   struct header_t : Mutex
   {
      std::size_t                 m_usecount;    //Number of attached allocators

      header_t()
      :  m_usecount(0) {}
   } m_header;
};

}  //namespace detail {

}  //namespace interprocess {

}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_NODE_ALLOCATOR_ALGO_HPP

