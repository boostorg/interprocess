//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_MEM_ALGO_RBTREE_BEST_FIT_HPP
#define BOOST_INTERPROCESS_MEM_ALGO_RBTREE_BEST_FIT_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/allocators/allocation_type.hpp>
#include <boost/interprocess/offset_ptr.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/min_max.hpp>
#include <boost/interprocess/detail/gcd_lcm.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include <boost/type_traits/type_with_alignment.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/assert.hpp>
#include <boost/static_assert.hpp>
#include <algorithm>
#include <utility>
#include <limits.h>

#include <assert.h>
#include <new>

/*!\file
   Describes a best-fit algorithm based in an intrusive red-black tree used to allocate
   objects in shared memory. This class is intended as a base class for single segment
   and multi-segment implementations.
*/

namespace boost {
namespace interprocess {

//!This class implements an algorithm that stores the free nodes in a red-black tree
//!to have logarithmic search/insert times.
template<class MutexFamily, class VoidPointer>
class rbtree_best_fit
{
   /// @cond
   //Non-copyable
   rbtree_best_fit();
   rbtree_best_fit(const rbtree_best_fit &);
   rbtree_best_fit &operator=(const rbtree_best_fit &);
   /// @endcond

   public:
   //!Shared interprocess_mutex family used for the rest of the Interprocess framework
   typedef MutexFamily        mutex_family;
   //!Pointer type to be used with the rest of the Interprocess framework
   typedef VoidPointer        void_pointer;

   /// @cond
   private:
   struct block_ctrl;
   typedef typename detail::
      pointer_to_other<void_pointer, block_ctrl>::type block_ctrl_ptr;

   typedef boost::intrusive::set_base_hook
      < boost::intrusive::tag
      , boost::intrusive::normal_link
      , VoidPointer>   TreeHook;

   struct SizeHolder
   {
      //!This block's memory size (including block_ctrl 
      //!header) in Alignment units
      std::size_t m_prev_size :  sizeof(std::size_t)*CHAR_BIT - 1;
      std::size_t m_end       :  1;
      std::size_t m_size      :  sizeof(std::size_t)*CHAR_BIT - 1;
      std::size_t m_allocated :  1;
   };

   //!Block control structure
   struct block_ctrl
      :  public SizeHolder, public TreeHook
   {
      block_ctrl()
      {  this->m_end = 0;  this->m_size = 0; this->m_allocated = 0;  }

      friend bool operator<(const block_ctrl &a, const block_ctrl &b)
      {  return a.m_size < b.m_size;  }
      friend bool operator==(const block_ctrl &a, const block_ctrl &b)
      {  return a.m_size == b.m_size;  }
   };

   struct size_block_ctrl_compare
   {
      bool operator()(std::size_t size, const block_ctrl &block) const
      {  return size < block.m_size;  }

      bool operator()(const block_ctrl &block, std::size_t size) const
      {  return block.m_size < size;  }      
   };

   //!Shared interprocess_mutex to protect memory allocate/deallocate
   typedef typename MutexFamily::mutex_type        interprocess_mutex;

   typedef boost::intrusive::multiset
         <typename TreeHook::template value_traits<block_ctrl> > Imultiset;
   typedef typename Imultiset::iterator imultiset_iterator;

   //!This struct includes needed data and derives from
   //!interprocess_mutex to allow EBO when using null interprocess_mutex
   struct header_t : public interprocess_mutex
   {
      Imultiset            m_imultiset;

      //!The extra size required by the segment
      std::size_t       m_extra_hdr_bytes;
      //!Allocated bytes for internal checking
      std::size_t       m_allocated;
      //!The size of the memory segment
      std::size_t       m_size;
   }  m_header;
   /// @endcond

   public:
   //!Constructor. "size" is the total size of the managed memory segment, 
   //!"extra_hdr_bytes" indicates the extra bytes beginning in the sizeof(rbtree_best_fit)
   //!offset that the allocator should not use at all.
   rbtree_best_fit           (std::size_t size, std::size_t extra_hdr_bytes);

   //!Destructor.
   ~rbtree_best_fit();

   //!Obtains the minimum size needed by the algorithm
   static std::size_t get_min_size (std::size_t extra_hdr_bytes);

   //Functions for single segment management

   //!Allocates bytes, returns 0 if there is not more memory
   void* allocate             (std::size_t nbytes);

   //!Deallocates previously allocated bytes
   void   deallocate          (void *addr);

   //!Returns the size of the memory segment
   std::size_t get_size()  const;

   //!Returns the number of free bytes of the segment
   std::size_t get_free_memory()  const;

   //!Initializes to zero all the memory that's not in use.
   //!This function is normally used for security reasons.
   void zero_free_memory();

   //!Increases managed memory in extra_size bytes more
   void grow(std::size_t extra_size);

   //!Returns true if all allocated memory has been deallocated
   bool all_memory_deallocated();

   //!Makes an internal sanity check and returns true if success
   bool check_sanity();

   std::pair<void *, bool>
      allocation_command  (allocation_type command,   std::size_t limit_size,
                           std::size_t preferred_size,std::size_t &received_size, 
                           void *reuse_ptr = 0, std::size_t backwards_multiple = 1);

   //!Returns the size of the buffer previously allocated pointed by ptr
   std::size_t size(const void *ptr) const;

   //!Allocates aligned bytes, returns 0 if there is not more memory.
   //!Alignment must be power of 2
   void* allocate_aligned     (std::size_t nbytes, std::size_t alignment);

   /// @cond
   private:
   //!Real allocation algorithm with min allocation option
   std::pair<void *, bool> priv_allocate(allocation_type command
                                        ,std::size_t min_size
                                        ,std::size_t preferred_size
                                        ,std::size_t &received_size
                                        ,void *reuse_ptr = 0
                                        ,std::size_t backwards_multiple = 1);

   //!Obtains the block control structure of the user buffer
   static block_ctrl *priv_get_block(const void *ptr);

   //!Obtains the pointer returned to the user from the block control
   static void *priv_get_user_buffer(const block_ctrl *block);

   //!Returns the number of total units that a user buffer
   //!of "userbytes" bytes really occupies (including header)
   static std::size_t priv_get_total_units(std::size_t userbytes);

   //!Translates a size to a number of units (ceiling the result)
   static std::size_t priv_ceil_units(std::size_t size);

   //!Translates a size to a number of units (flooring the result)
   static std::size_t priv_floor_units(std::size_t size);

   //!Rounds the size (ceiling) to a multiple of units
   static std::size_t priv_multiple_of_units(std::size_t size);

   //!Asserts if the alignment is not correct
   static void priv_check_alignment(const void *ptr);

   //!Real expand function implementation
   bool priv_expand(void *ptr
                   ,const std::size_t min_size, const std::size_t preferred_size
                   ,std::size_t &received_size);

   //!Real expand to both sides implementation
   void* priv_expand_both_sides(allocation_type command
                               ,std::size_t min_size
                               ,std::size_t preferred_size
                               ,std::size_t &received_size
                               ,void *reuse_ptr
                               ,bool only_preferred_backwards
                               ,std::size_t backwards_multiple);

   //!Real shrink function implementation
   bool priv_shrink(void *ptr
                   ,const std::size_t max_size, const std::size_t preferred_size
                   ,std::size_t &received_size);

   //!Set the size in the tail of the block
   void priv_tail_size(block_ctrl *ptr, std::size_t size);

   //!Real private aligned allocation function
   void* priv_allocate_aligned     (std::size_t nbytes, std::size_t alignment);

   //!Get the size in the tail of the block
   std::size_t priv_tail_size(block_ctrl *ptr);

   //!Get the size in the tail of the previous block
   block_ctrl * priv_prev_block(block_ctrl *ptr);

   //!Get the size in the tail of the previous block
   block_ctrl * priv_next_block(block_ctrl *ptr);

   //!Check if this block is free (not allocated)
   bool priv_is_allocated_block(block_ctrl *ptr);

   //!Marks the block as allocated
   void priv_mark_as_allocated_block(block_ctrl *ptr);

   //!Marks the block as allocated
   void priv_mark_as_free_block(block_ctrl *ptr);

   //!Checks if block has enough memory and splits/unlinks the block
   //!returning the address to the users
   void* priv_check_and_allocate(std::size_t units
                                ,block_ctrl* block
                                ,std::size_t &received_size);
   //!Real deallocation algorithm
   void priv_deallocate(void *addr);

   //!Makes a new memory portion available for allocation
   void priv_add_segment(void *addr, std::size_t size);

   enum { Alignment           = boost::alignment_of<boost::detail::max_align>::value  };
   enum { AlignmentMask       = (Alignment - 1)  };
   enum { BlockCtrlBytes      = detail::ct_rounded_size<sizeof(block_ctrl), Alignment>::value  };
   enum { BlockCtrlUnits       = BlockCtrlBytes/Alignment   };
   enum { AllocatedCtrlBytes  = detail::ct_rounded_size<sizeof(SizeHolder), Alignment>::value   };
   enum { AllocatedCtrlUnits   = AllocatedCtrlBytes/Alignment   };
   enum { EndCtrlBlockBytes   = detail::ct_rounded_size<sizeof(SizeHolder), Alignment>::value  };
   enum { EndCtrlBlockUnits    = EndCtrlBlockBytes/Alignment   };
   enum { MinBlockUnits        = BlockCtrlUnits  };

   //Make sure the maximum alignment is power of two
   BOOST_STATIC_ASSERT((0 == (Alignment & (Alignment - std::size_t(1u)))));
   /// @endcond
   public:
   enum{ PayloadPerAllocation = AllocatedCtrlBytes };
};

template<class MutexFamily, class VoidPointer>
inline rbtree_best_fit<MutexFamily, VoidPointer>::
   rbtree_best_fit(std::size_t size, std::size_t extra_hdr_bytes)
{
   //Initialize the header
   m_header.m_allocated       = 0;
   m_header.m_size            = size;
   m_header.m_extra_hdr_bytes = extra_hdr_bytes;

   //Now write calculate the offset of the first big block that will
   //cover the whole segment
   std::size_t block1_off  = priv_multiple_of_units(sizeof(*this)+extra_hdr_bytes);
   assert(get_min_size(extra_hdr_bytes) <= size);

   priv_add_segment(detail::char_ptr_cast(this) + block1_off, size - block1_off);
}

template<class MutexFamily, class VoidPointer>
inline rbtree_best_fit<MutexFamily, VoidPointer>::~rbtree_best_fit()
{
   //There is a memory leak!
//   assert(m_header.m_allocated == 0);
//   assert(m_header.m_root.m_next->m_next == block_ctrl_ptr(&m_header.m_root));
}

template<class MutexFamily, class VoidPointer>
void rbtree_best_fit<MutexFamily, VoidPointer>::grow(std::size_t extra_size)
{  
   //Get the address of the first block
   std::size_t block1_off  = 
      priv_multiple_of_units(sizeof(*this) + m_header.m_extra_hdr_bytes);

   block_ctrl *first_block = reinterpret_cast<block_ctrl *>
                                 (detail::char_ptr_cast(this) + block1_off);
   block_ctrl *old_end_block   = priv_prev_block(first_block);
   assert(priv_is_allocated_block(old_end_block));
   assert(old_end_block->m_end);
   std::size_t old_border_offset = (detail::char_ptr_cast(old_end_block) - 
                                    detail::char_ptr_cast(this)) + EndCtrlBlockBytes;

   //Update managed buffer's size
   m_header.m_size += extra_size;

   //We need at least MinBlockUnits blocks to create a new block
//   assert((m_header.m_size - old_end) >= MinBlockUnits);
   if((m_header.m_size - old_border_offset) < MinBlockUnits){
      return;
   }

   //Now create a new block between the old end and the new end
   std::size_t align_offset = (m_header.m_size - old_border_offset)/Alignment;
   block_ctrl *new_end_block = reinterpret_cast<block_ctrl*>
      (detail::char_ptr_cast(old_end_block) + align_offset*Alignment);
   new_end_block->m_size      = (detail::char_ptr_cast(first_block) - 
                                 detail::char_ptr_cast(new_end_block))/Alignment;
   assert(first_block == priv_next_block(new_end_block));
   priv_mark_as_allocated_block(new_end_block);
   new_end_block->m_end       = 1;

   //This will update the link of the first block
   priv_tail_size(new_end_block, new_end_block->m_size);
   assert(new_end_block == priv_prev_block(first_block));

   //The old end block is the new block
   std::size_t old_end_prev = old_end_block->m_prev_size;
   block_ctrl *new_block = new(old_end_block)block_ctrl;
   new_block->m_size = (detail::char_ptr_cast(new_end_block) - 
                        detail::char_ptr_cast(new_block))/Alignment;
   new_block->m_prev_size = old_end_prev;
   assert(new_block->m_size >= BlockCtrlUnits);
   priv_tail_size(new_block, new_block->m_size);
   assert(priv_next_block(new_block) == new_end_block);
   priv_mark_as_allocated_block(new_block);

   m_header.m_allocated += new_block->m_size*Alignment;

   //Now deallocate the newly created block
   this->priv_deallocate(priv_get_user_buffer(new_block));
}

template<class MutexFamily, class VoidPointer>
void rbtree_best_fit<MutexFamily, VoidPointer>::
   priv_add_segment(void *addr, std::size_t size)
{  
   //Check alignment
   assert(((detail::char_ptr_cast(addr) - ((char*)0)) % Alignment) == 0 );
   //Check size
   assert(size >= (BlockCtrlBytes + EndCtrlBlockBytes));

   //Initialize the first big block and the "end" node
   block_ctrl *first_big_block = new(addr)block_ctrl;
   first_big_block->m_size = size/Alignment - EndCtrlBlockUnits;
   assert(first_big_block->m_size >= BlockCtrlUnits);

   //The "end" node is just a node of size 0 with the "end" bit set
   block_ctrl *end_block = static_cast<block_ctrl*> 
      (new (reinterpret_cast<SizeHolder*>
         (detail::char_ptr_cast(addr) + first_big_block->m_size*Alignment))SizeHolder);

   //This will overwrite the prev part of the "end" node
   priv_tail_size(first_big_block, first_big_block->m_size);
   priv_mark_as_free_block (first_big_block);
   first_big_block->m_prev_size = end_block->m_size =
      (detail::char_ptr_cast(first_big_block) - detail::char_ptr_cast(end_block))/Alignment;
   end_block->m_end        = 1;
   end_block->m_allocated  = 1;

   assert(priv_next_block(first_big_block) == end_block);
   assert(priv_prev_block(end_block) == first_big_block);
   assert(priv_next_block(end_block) == first_big_block);
   assert(priv_prev_block(first_big_block) == end_block);

   //Some check to validate the algorithm, since it makes some assumptions
   //to optimize the space wasted in bookkeeping:

   //Check that the sizes of the header are placed before the rbtree
   assert((void*)(SizeHolder*)first_big_block < (void*)(TreeHook*)first_big_block);

   //Check that the alignment is power of two (we use some optimizations based on this)
   //assert((Alignment % 2) == 0);

   //Insert it in the intrusive containers
   m_header.m_imultiset.insert(*first_big_block);
}

template<class MutexFamily, class VoidPointer>
inline std::size_t rbtree_best_fit<MutexFamily, VoidPointer>::get_size()  const
{  return m_header.m_size;  }

template<class MutexFamily, class VoidPointer>
inline std::size_t rbtree_best_fit<MutexFamily, VoidPointer>::get_free_memory()  const
{
   return m_header.m_size - m_header.m_allocated - 
      priv_multiple_of_units(sizeof(*this) + m_header.m_extra_hdr_bytes);
}

template<class MutexFamily, class VoidPointer>
inline std::size_t rbtree_best_fit<MutexFamily, VoidPointer>::
   get_min_size (std::size_t extra_hdr_bytes)
{
   return (priv_ceil_units(sizeof(rbtree_best_fit) + extra_hdr_bytes) + 
           MinBlockUnits + EndCtrlBlockUnits)*Alignment;
}

template<class MutexFamily, class VoidPointer>
inline bool rbtree_best_fit<MutexFamily, VoidPointer>::
    all_memory_deallocated()
{
   //-----------------------
   boost::interprocess::scoped_lock<interprocess_mutex> guard(m_header);
   //-----------------------
   std::size_t block1_off  = 
      priv_multiple_of_units(sizeof(*this)+m_header.m_extra_hdr_bytes);

   return m_header.m_allocated == 0 && 
      m_header.m_imultiset.begin() != m_header.m_imultiset.end() &&
       (++m_header.m_imultiset.begin()) == m_header.m_imultiset.end()
       && m_header.m_imultiset.begin()->m_size == 
         (m_header.m_size - block1_off - EndCtrlBlockBytes)/Alignment;
}

template<class MutexFamily, class VoidPointer>
bool rbtree_best_fit<MutexFamily, VoidPointer>::
    check_sanity()
{
   //-----------------------
   boost::interprocess::scoped_lock<interprocess_mutex> guard(m_header);
   //-----------------------
   imultiset_iterator ib(m_header.m_imultiset.begin()), ie(m_header.m_imultiset.end());

   std::size_t free_memory = 0;

   //Iterate through all blocks obtaining their size
   for(; ib != ie; ++ib){
      free_memory += ib->m_size*Alignment;
   }

   //Check allocated bytes are less than size
   if(m_header.m_allocated > m_header.m_size){
      return false;
   }

   std::size_t block1_off  = 
      priv_multiple_of_units(sizeof(*this)+m_header.m_extra_hdr_bytes);

   //Check free bytes are less than size
   if(free_memory > (m_header.m_size - block1_off)){
      return false;
   }
   return true;
}

template<class MutexFamily, class VoidPointer>
inline void* rbtree_best_fit<MutexFamily, VoidPointer>::
   allocate(std::size_t nbytes)
{  
   //-----------------------
   boost::interprocess::scoped_lock<interprocess_mutex> guard(m_header);
   //-----------------------
   std::size_t ignore;
   return priv_allocate(allocate_new, nbytes, nbytes, ignore).first;
}

template<class MutexFamily, class VoidPointer>
inline void* rbtree_best_fit<MutexFamily, VoidPointer>::
   allocate_aligned(std::size_t nbytes, std::size_t alignment)
{  
   //-----------------------
   boost::interprocess::scoped_lock<interprocess_mutex> guard(m_header);
   //-----------------------
   return priv_allocate_aligned(nbytes, alignment);
}

template<class MutexFamily, class VoidPointer>
inline std::pair<void *, bool> rbtree_best_fit<MutexFamily, VoidPointer>::
   allocation_command  (allocation_type command,   std::size_t min_size,
                        std::size_t preferred_size,std::size_t &received_size, 
                        void *reuse_ptr, std::size_t backwards_multiple)
{
   //-----------------------
   boost::interprocess::scoped_lock<interprocess_mutex> guard(m_header);
   //-----------------------
   return priv_allocate(command, min_size, preferred_size, received_size, reuse_ptr, backwards_multiple);
}

template<class MutexFamily, class VoidPointer>
inline std::size_t rbtree_best_fit<MutexFamily, VoidPointer>::
   size(const void *ptr) const
{
   //We need no synchronization since this block's size is not going
   //to be modified by anyone else
   //Obtain the real size of the block
   return (priv_get_block(ptr)->m_size - AllocatedCtrlUnits)*Alignment;
}

template<class MutexFamily, class VoidPointer>
inline void rbtree_best_fit<MutexFamily, VoidPointer>::zero_free_memory()
{
   //-----------------------
   boost::interprocess::scoped_lock<interprocess_mutex> guard(m_header);
   //-----------------------
   imultiset_iterator ib(m_header.m_imultiset.begin()), ie(m_header.m_imultiset.end());

   //Iterate through all blocks obtaining their size
   for(; ib != ie; ++ib){
      //Just clear user the memory part reserved for the user      
      std::memset( detail::char_ptr_cast(&*ib) + BlockCtrlBytes
                 , 0
                 , ib->m_size*Alignment - BlockCtrlBytes);
   }
}

template<class MutexFamily, class VoidPointer>
void* rbtree_best_fit<MutexFamily, VoidPointer>::
   priv_expand_both_sides(allocation_type command
                         ,std::size_t min_size
                         ,std::size_t preferred_size
                         ,std::size_t &received_size
                         ,void *reuse_ptr
                         ,bool only_preferred_backwards
                         ,std::size_t backwards_multiple)
{
   if(command & expand_fwd){
      if(priv_expand(reuse_ptr, min_size, preferred_size, received_size))
         return reuse_ptr;
   }
   else{
      received_size = this->size(reuse_ptr);
      if(received_size >= preferred_size)
         return reuse_ptr;
   }

   if(backwards_multiple){
      BOOST_ASSERT(0 == (min_size       % backwards_multiple));
      BOOST_ASSERT(0 == (preferred_size % backwards_multiple));
   }

   if(command & expand_bwd){
      //Obtain the real size of the block
      block_ctrl *reuse = priv_get_block(reuse_ptr);

      //Sanity check 
      assert(reuse->m_size == priv_tail_size(reuse));
      priv_check_alignment(reuse);

      block_ctrl *prev_block;

      //If the previous block is not free, there is nothing to do
      if(priv_is_allocated_block(prev_block = priv_prev_block(reuse))){
         return 0;
      }

      //Some sanity checks
      assert(prev_block->m_size == priv_tail_size(prev_block));
      priv_check_alignment(prev_block);

      //Let's calculate the number of extra bytes of data before the current
      //block's begin. The value is a multiple of backwards_multiple
      std::size_t needs_backwards = preferred_size - 
         detail::get_truncated_size(received_size, backwards_multiple);

      const std::size_t lcm = detail::lcm(max_value(backwards_multiple, (std::size_t)Alignment)
                                         ,min_value(backwards_multiple, (std::size_t)Alignment));

      //If we want to use min_size data to get a buffer between preferred_size
      //and min_size if preferred_size can't be achieved, calculate the 
      //biggest of all possibilities
      if(!only_preferred_backwards){
         needs_backwards = min_size - detail::get_truncated_size(received_size, backwards_multiple);
      }

      assert((needs_backwards % backwards_multiple) == 0);

      const std::size_t needs_backwards_aligned = 
         detail::get_rounded_size(needs_backwards, lcm);

      //Check if previous block has enough size
      if(std::size_t(prev_block->m_size*Alignment) >= needs_backwards_aligned){
         //Now take all next space. This will succeed
         if(command & expand_fwd){
            if(!priv_expand(reuse_ptr, received_size, received_size, received_size)){
               assert(0);
            }
         }
         //We need a minimum size to split the previous one
         if(prev_block->m_size >= (needs_backwards_aligned/Alignment + BlockCtrlUnits)){
            block_ctrl *new_block = reinterpret_cast<block_ctrl *>
               (detail::char_ptr_cast(reuse) - needs_backwards_aligned);

            //Erase old previous block, since we will change it
            m_header.m_imultiset.erase(Imultiset::iterator_to(*prev_block));

            //Free old previous buffer
            new_block->m_size = 
               AllocatedCtrlUnits + (needs_backwards_aligned + received_size)/Alignment;
            assert(new_block->m_size >= BlockCtrlUnits);
            priv_tail_size(new_block, new_block->m_size);
            priv_mark_as_allocated_block(new_block);

            prev_block->m_size = (detail::char_ptr_cast(new_block) - 
                                  detail::char_ptr_cast(prev_block))/Alignment;
            assert(prev_block->m_size >= BlockCtrlUnits);
            priv_tail_size(prev_block, prev_block->m_size);
            priv_mark_as_free_block(prev_block);

            //Insert the remaining previous block in the free tree
            m_header.m_imultiset.insert( m_header.m_imultiset.begin(), *prev_block);
            received_size = needs_backwards_aligned + received_size;
            m_header.m_allocated += needs_backwards_aligned;
         
            //Check alignment
            priv_check_alignment(new_block);

            //If the backwards expansion has remaining bytes in the
            //first bytes, fill them with a pattern
            void *p = priv_get_user_buffer(new_block);
            void *user_ptr = detail::char_ptr_cast(p);
            assert(((char*)reuse_ptr - (char*)user_ptr) % backwards_multiple == 0);
            return user_ptr;
         }
         //Check if there is no place to create a new block and
         //the whole new block is multiple of the backwards expansion multiple
         else if(prev_block->m_size >= needs_backwards_aligned/Alignment &&
                 0 == (prev_block->m_size % lcm)) {
            //Erase old previous block, since we will change it
            m_header.m_imultiset.erase(Imultiset::iterator_to(*prev_block));

            //Just merge the whole previous block
            const std::size_t needs_backwards_aligned = prev_block->m_size*Alignment;
            const std::size_t needs_backwards = detail::get_truncated_size(needs_backwards_aligned, backwards_multiple);
            received_size = received_size/backwards_multiple*backwards_multiple + needs_backwards;

            m_header.m_allocated += prev_block->m_size*Alignment;
            //Now update sizes
            prev_block->m_size = prev_block->m_size + reuse->m_size;
            assert(prev_block->m_size >= BlockCtrlUnits);
            priv_tail_size(prev_block, prev_block->m_size);
            priv_mark_as_allocated_block(prev_block);
            priv_check_alignment(prev_block);

            //If the backwards expansion has remaining bytes in the
            //first bytes, fill them with a pattern
            void *p = priv_get_user_buffer(prev_block);
            void *user_ptr = detail::char_ptr_cast(p);
            assert(((char*)reuse_ptr - (char*)user_ptr) % backwards_multiple == 0);
            return user_ptr;
         }
         else{
            //Alignment issues
         }
      }
   }
   return 0;
}

template<class MutexFamily, class VoidPointer>
inline void* rbtree_best_fit<MutexFamily, VoidPointer>::
   priv_allocate_aligned(std::size_t nbytes, std::size_t alignment)
{  
   //Ensure power of 2
   if ((alignment & (alignment - std::size_t(1u))) != 0){
      //Alignment is not power of two
      assert((alignment & (alignment - std::size_t(1u))) != 0);
      return 0;
   }

   std::size_t real_size;
   if(alignment <= Alignment){
      return priv_allocate(allocate_new, nbytes, nbytes, real_size).first;
   }
   
   //We can find a aligned portion if we allocate a chunk that has alignment
   //nbytes + alignment bytes or more.
   std::size_t minimum_allocation = max_value(nbytes + alignment, std::size_t(MinBlockUnits*Alignment));
   //Since we will split that chunk, we must request a bit more memory
   //if the alignment is near the beginning of the buffer, because otherwise,
   //there is no space for a new chunk before the alignment.
   // 
   //            ____ Aligned here
   //           |
   //  -----------------------------------------------------
   // | MBU | 
   //  -----------------------------------------------------
   std::size_t request = 
      minimum_allocation + (MinBlockUnits*Alignment - AllocatedCtrlBytes);

   //Now allocate the buffer
   void *buffer = priv_allocate(allocate_new, request, request, real_size).first;
   if(!buffer){
      return 0;
   }
   else if ((((std::size_t)(buffer)) % alignment) == 0){
      //If we are lucky and the buffer is aligned, just split it and
      //return the high part
      block_ctrl *first  = priv_get_block(buffer);
      std::size_t old_size = first->m_size;
      const std::size_t first_min_units =
         max_value(priv_ceil_units(nbytes) + AllocatedCtrlUnits, std::size_t(MinBlockUnits));
      //We can create a new block in the end of the segment
      if(old_size >= (first_min_units + MinBlockUnits)){
         block_ctrl *second =  new((char*)first + Alignment*first_min_units) block_ctrl;
         first->m_size  = first_min_units;
         second->m_size = old_size - first->m_size;
         BOOST_ASSERT(second->m_size >= MinBlockUnits);
         priv_tail_size(first, first->m_size);
         priv_tail_size(second, second->m_size);
         priv_mark_as_allocated_block(second);
         this->priv_deallocate(priv_get_user_buffer(second));
      }
      return buffer;
   }

   //Buffer not aligned, find the aligned part.
   // 
   //                    ____ Aligned here
   //                   |
   //  -----------------------------------------------------
   // | MBU +more | ACB |
   //  -----------------------------------------------------
   char *pos = (char*)
      ((std::size_t)((char*)buffer +
         //This is the minimum size of (2)
         (MinBlockUnits*Alignment - AllocatedCtrlBytes) +
         //This is the next MBU for the aligned memory
         AllocatedCtrlBytes +
         //This is the alignment trick
         alignment - 1) & -alignment);

   //Now obtain the address of the blocks
   block_ctrl *first  = priv_get_block(buffer);
   block_ctrl *second = priv_get_block(pos);

   //Set the new size of the first block
   std::size_t old_size = first->m_size;
   first->m_size  = ((char*)second - (char*)first)/Alignment;
   priv_tail_size(first, first->m_size);
   priv_mark_as_allocated_block(first);

   //Now check if we can create a new buffer in the end
   //
   //              __"second" block
   //             |      __Aligned here
   //             |     |      __"third" block
   //  -----------|-----|-----|------------------------------
   // | MBU +more | ACB | (3) | BCU |
   //  -----------------------------------------------------
   //This size will be the minimum size to be able to create a
   //new chunk in the end.
   const std::size_t second_min_units = max_value(std::size_t(MinBlockUnits),
                     priv_ceil_units(nbytes) + AllocatedCtrlUnits );

   //Check if we can create a new block (of size MinBlockUnits) in the end of the segment
   if((old_size - first->m_size) >= (second_min_units + MinBlockUnits)){
      //Now obtain the address of the end block
      block_ctrl *third = new ((char*)second + Alignment*second_min_units)block_ctrl;
      second->m_size = second_min_units;
      third->m_size  = old_size - first->m_size - second->m_size;
      BOOST_ASSERT(third->m_size >= MinBlockUnits);
      priv_tail_size(second, second->m_size);
      priv_tail_size(third, third->m_size);
      priv_mark_as_allocated_block(second);
      priv_mark_as_allocated_block(third);
      this->priv_deallocate(priv_get_user_buffer(third));
   }
   else{
      second->m_size = old_size - first->m_size;
      priv_tail_size(second, second->m_size);
      priv_mark_as_allocated_block(second);
   }

   this->priv_deallocate(priv_get_user_buffer(first));
   return priv_get_user_buffer(second);
}

template<class MutexFamily, class VoidPointer>
std::pair<void *, bool> rbtree_best_fit<MutexFamily, VoidPointer>::
   priv_allocate(allocation_type command
                ,std::size_t limit_size
                ,std::size_t preferred_size
                ,std::size_t &received_size
                ,void *reuse_ptr
               ,std::size_t backwards_multiple)
{
   //Remove me. Forbid backwards allocation
   //command &= (~expand_bwd);

   if(command & shrink_in_place){
      bool success = 
         this->priv_shrink(reuse_ptr, limit_size, preferred_size, received_size);
      return std::pair<void *, bool> ((success ? reuse_ptr : 0), true);
   }
   typedef std::pair<void *, bool> return_type;
   received_size = 0;

   if(limit_size > preferred_size)
      return return_type(0, false);

   //Number of units to request (including block_ctrl header)
   std::size_t preferred_units = priv_get_total_units(preferred_size);
   if(preferred_units < BlockCtrlUnits)
      preferred_units = BlockCtrlUnits;

   //Number of units to request (including block_ctrl header)
   std::size_t limit_units = priv_get_total_units(limit_size);
   if(limit_units < BlockCtrlUnits)
      limit_units = BlockCtrlUnits;

   //Expand in place
   if(reuse_ptr && (command & (expand_fwd | expand_bwd))){
      void *ret = priv_expand_both_sides
         (command, limit_size, preferred_size, received_size, reuse_ptr, true, backwards_multiple);
      if(ret)
         return return_type(ret, true);
   }

   if(command & allocate_new){
      size_block_ctrl_compare comp;
      imultiset_iterator it(m_header.m_imultiset.lower_bound(preferred_units, comp));

      if(it != m_header.m_imultiset.end()){
         return return_type(this->priv_check_and_allocate
            (limit_units, detail::get_pointer(&*it), received_size), false);
      }

      if(it != m_header.m_imultiset.begin()&&
              (--it)->m_size >= limit_units){
         return return_type(this->priv_check_and_allocate
            (it->m_size, detail::get_pointer(&*it), received_size), false);
      }
   }


   //Now try to expand both sides with min size
   if(reuse_ptr && (command & (expand_fwd | expand_bwd))){
      return return_type(priv_expand_both_sides
         (command, limit_size, preferred_size, received_size, reuse_ptr, false, backwards_multiple), true);
   }

   return return_type(0, false);
}

template<class MutexFamily, class VoidPointer>
inline
typename rbtree_best_fit<MutexFamily, VoidPointer>::block_ctrl *
   rbtree_best_fit<MutexFamily, VoidPointer>::priv_get_block(const void *ptr)
{
   std::size_t offset = 0;
   const void *p = (const void *)(std::size_t(ptr) & ~AlignmentMask);
   if(p != ptr){
      offset = detail::char_ptr_cast(p)[0]*Alignment;
   }
   return reinterpret_cast<block_ctrl*>
                     (detail::char_ptr_cast(p) - offset - AllocatedCtrlBytes);
}

template<class MutexFamily, class VoidPointer>
inline
void *rbtree_best_fit<MutexFamily, VoidPointer>::
      priv_get_user_buffer(const typename rbtree_best_fit<MutexFamily, VoidPointer>::block_ctrl *block)
{  return detail::char_ptr_cast(block) + AllocatedCtrlBytes;   }

template<class MutexFamily, class VoidPointer>
inline
std::size_t rbtree_best_fit<MutexFamily, VoidPointer>::
   priv_get_total_units(std::size_t userbytes)
{
   return detail::get_rounded_size(userbytes, Alignment)/Alignment + AllocatedCtrlUnits;
}

template<class MutexFamily, class VoidPointer>
inline
std::size_t rbtree_best_fit<MutexFamily, VoidPointer>::
   priv_ceil_units(std::size_t size)
{
   return detail::get_rounded_size(size, Alignment)/Alignment;
}

template<class MutexFamily, class VoidPointer>
inline
std::size_t rbtree_best_fit<MutexFamily, VoidPointer>::
   priv_floor_units(std::size_t size)
{  return size/Alignment;  }

template<class MutexFamily, class VoidPointer>
inline
std::size_t rbtree_best_fit<MutexFamily, VoidPointer>::
   priv_multiple_of_units(std::size_t size)
{  return detail::get_rounded_size(size, Alignment);  }

template<class MutexFamily, class VoidPointer>
inline
void rbtree_best_fit<MutexFamily, VoidPointer>::
   priv_check_alignment(const void *ptr)
{
   const std::size_t remaining(std::size_t(ptr) & AlignmentMask);
   if(remaining){
      const void *p = (void*)(std::size_t(ptr) & ~AlignmentMask);
      (void)p;
      for(std::size_t i = 1; i < remaining; ++i){
         BOOST_ASSERT(detail::char_ptr_cast(p)[i] == (char)i);
      }
   }
   else{ 
      BOOST_ASSERT(((std::size_t)ptr) % Alignment == 0);
   }
}

template<class MutexFamily, class VoidPointer>
bool rbtree_best_fit<MutexFamily, VoidPointer>::
   priv_expand (void *ptr
               ,const std::size_t min_size
               ,const std::size_t preferred_size
               ,std::size_t &received_size)
{
   //Obtain the real size of the block
   block_ctrl *block = priv_get_block(ptr);
   std::size_t old_block_size = block->m_size;

   //The block must be marked as allocated and the sizes must be equal
   assert(priv_is_allocated_block(block));
   assert(old_block_size == priv_tail_size(block));
   
   //Put this to a safe value
   received_size = (old_block_size - AllocatedCtrlUnits)*Alignment;

   if(received_size >= min_size)
      return true;

   //Now translate it to Alignment units
   const std::size_t min_units = priv_ceil_units(min_size);
   const std::size_t preferred_units = priv_ceil_units(preferred_size);

   //Some parameter checks
   assert(min_units <= preferred_units);

   block_ctrl *next_block;

   if(priv_is_allocated_block(next_block = priv_next_block(block)))
      return false;

   //Is "block" + "next_block" big enough?
   const std::size_t merged_size = old_block_size + next_block->m_size;

   //Now get the expansion size
   const std::size_t max_size_to_expand = merged_size - AllocatedCtrlUnits;

   if(max_size_to_expand < min_units){
      received_size = max_size_to_expand*Alignment;
      return false;
   }

   //Now get the maximum size the user can allocate
   std::size_t size_to_expand = (max_size_to_expand < preferred_units) 
                              ? max_size_to_expand : preferred_units;

   //Check if we can split the next one in two parts
   if((merged_size - (BlockCtrlUnits + AllocatedCtrlUnits)) >= size_to_expand){
      //Now we have to update the data in the tree
      m_header.m_imultiset.erase(Imultiset::iterator_to(*next_block));

      //This block is bigger than needed, split it in 
      //two blocks, the first one will be merged and
      //the second's size will be the remaining space
      assert(next_block->m_size == priv_tail_size(next_block));
      std::size_t new_block_offset = AllocatedCtrlUnits + size_to_expand;

      //This is the remaining block
      block_ctrl *new_block = new(reinterpret_cast<block_ctrl*>
                     (detail::char_ptr_cast(block) + new_block_offset*Alignment))block_ctrl;
      new_block->m_size  = merged_size - new_block_offset;
      assert(new_block->m_size >= BlockCtrlUnits);
      priv_tail_size(new_block, new_block->m_size);
      priv_mark_as_free_block(new_block);

      //Insert the new block in the container
      m_header.m_imultiset.insert(m_header.m_imultiset.begin(), *new_block);

      //Write the new length
      block->m_size = size_to_expand + AllocatedCtrlUnits;
      assert(block->m_size >= BlockCtrlUnits);
      priv_tail_size(block, block->m_size);
      m_header.m_allocated += (new_block_offset - old_block_size)*Alignment;
   }
   //There is no free space to create a new node: just merge both blocks
   else{
      //Now we have to update the data in the tree
      m_header.m_imultiset.erase(Imultiset::iterator_to(*next_block));

      //Write the new length
      block->m_size = merged_size;
      assert(block->m_size >= BlockCtrlUnits);
      priv_tail_size(block, merged_size);
      m_header.m_allocated += (merged_size - old_block_size)*Alignment;
   }

   received_size = (block->m_size - AllocatedCtrlUnits)*Alignment;
   return true;
}

template<class MutexFamily, class VoidPointer>
bool rbtree_best_fit<MutexFamily, VoidPointer>::
   priv_shrink (void *ptr
               ,const std::size_t max_size
               ,const std::size_t preferred_size
               ,std::size_t &received_size)
{
   //Obtain the real block
   block_ctrl *block = priv_get_block(ptr);
   std::size_t old_block_size = block->m_size;

   //The block must be marked as allocated
   assert(priv_is_allocated_block(block));

   //Check if alignment and block size are right
   priv_check_alignment(ptr);

   //Put this to a safe value
   received_size = (old_block_size - AllocatedCtrlUnits)*Alignment;

   //Now translate it to Alignment units
   const std::size_t max_units       = priv_floor_units(max_size);
   const std::size_t preferred_units = priv_ceil_units(preferred_size);

   //Check if rounded max and preferred are possible correct
   if(max_units < preferred_units)
      return false;

   //Check if the block is smaller than the requested minimum
   std::size_t old_data_size = old_block_size - AllocatedCtrlUnits;

   if(old_data_size < preferred_units)
      return false;

   //If the block is smaller than the requested minimum
   if(old_data_size == preferred_units)
      return true;

   std::size_t size_to_shrink = 
      ((BlockCtrlUnits - AllocatedCtrlUnits) > preferred_units)
      ? (BlockCtrlUnits - AllocatedCtrlUnits)
      : preferred_units;

   //Some parameter checks
   if(max_units < size_to_shrink)
      return false;

   //We must be able to create at least a new empty block
   if((old_data_size - size_to_shrink) < BlockCtrlUnits ){
      return false;
   }

   //Now we can just rewrite the size of the old buffer
   block->m_size = size_to_shrink + AllocatedCtrlUnits;
   assert(block->m_size >= BlockCtrlUnits);
   priv_tail_size(block, block->m_size);

   //Update new size
   received_size = size_to_shrink*Alignment;

   //We create the new block
   block_ctrl *new_block = new(reinterpret_cast<block_ctrl*>
               (detail::char_ptr_cast(block) + block->m_size*Alignment)) block_ctrl;

   //Write control data to simulate this new block was previously allocated
   //and deallocate it
   priv_mark_as_allocated_block(new_block);
   new_block->m_size = old_block_size - block->m_size;
   assert(new_block->m_size >= BlockCtrlUnits);
   priv_tail_size(new_block, new_block->m_size);
   this->priv_deallocate(priv_get_user_buffer(new_block));
   return true;
}

template<class MutexFamily, class VoidPointer> inline
void rbtree_best_fit<MutexFamily, VoidPointer>::priv_tail_size
   (typename rbtree_best_fit<MutexFamily, VoidPointer>::block_ctrl *ptr, std::size_t size)
{  priv_next_block(ptr)->m_prev_size = size;    }

template<class MutexFamily, class VoidPointer> inline
std::size_t rbtree_best_fit<MutexFamily, VoidPointer>::priv_tail_size
   (typename rbtree_best_fit<MutexFamily, VoidPointer>::block_ctrl *ptr)
{  return priv_next_block(ptr)->m_prev_size; }

template<class MutexFamily, class VoidPointer> inline
typename rbtree_best_fit<MutexFamily, VoidPointer>::block_ctrl *
   rbtree_best_fit<MutexFamily, VoidPointer>::priv_prev_block
      (typename rbtree_best_fit<MutexFamily, VoidPointer>::block_ctrl *ptr)
{
   return reinterpret_cast<block_ctrl *>
      (detail::char_ptr_cast(ptr) - ptr->m_prev_size*Alignment);
}

template<class MutexFamily, class VoidPointer> inline
typename rbtree_best_fit<MutexFamily, VoidPointer>::block_ctrl *
   rbtree_best_fit<MutexFamily, VoidPointer>::priv_next_block
      (typename rbtree_best_fit<MutexFamily, VoidPointer>::block_ctrl *ptr)
{
   return reinterpret_cast<block_ctrl *>
      (detail::char_ptr_cast(ptr) + ptr->m_size*Alignment);
}

template<class MutexFamily, class VoidPointer> inline
bool rbtree_best_fit<MutexFamily, VoidPointer>::priv_is_allocated_block
      (typename rbtree_best_fit<MutexFamily, VoidPointer>::block_ctrl *block)
{  return block->m_allocated != 0;  }

template<class MutexFamily, class VoidPointer> inline
void rbtree_best_fit<MutexFamily, VoidPointer>::priv_mark_as_allocated_block
      (typename rbtree_best_fit<MutexFamily, VoidPointer>::block_ctrl *block)
{  block->m_allocated = 1; }

template<class MutexFamily, class VoidPointer> inline
void rbtree_best_fit<MutexFamily, VoidPointer>::priv_mark_as_free_block
      (typename rbtree_best_fit<MutexFamily, VoidPointer>::block_ctrl *block)
{  block->m_allocated = 0; }

template<class MutexFamily, class VoidPointer> inline
void* rbtree_best_fit<MutexFamily, VoidPointer>::priv_check_and_allocate
   (std::size_t nunits
   ,typename rbtree_best_fit<MutexFamily, VoidPointer>::block_ctrl* block
   ,std::size_t &received_size)
{
   std::size_t upper_nunits = nunits + BlockCtrlUnits;
   imultiset_iterator ittree = Imultiset::iterator_to(*block);

   if (block->m_size >= upper_nunits){
      //Now we have to update the data in the tree
      m_header.m_imultiset.erase(ittree);

      //This block is bigger than needed, split it in 
      //two blocks, the first's size will be "units" and
      //the second's size "block->m_size-units"
      std::size_t block_old_size = block->m_size;
      block->m_size = nunits;
      assert(block->m_size >= BlockCtrlUnits);
      priv_tail_size(block, block->m_size);

      //This is the remaining block
      block_ctrl *new_block = new(reinterpret_cast<block_ctrl*>
                     (detail::char_ptr_cast(block) + Alignment*nunits))block_ctrl;
      new_block->m_size  = block_old_size - nunits;
      assert(new_block->m_size >= BlockCtrlUnits);
      priv_tail_size(new_block, new_block->m_size);
      priv_mark_as_free_block(new_block);

      //Insert the new block in the container
      m_header.m_imultiset.insert(m_header.m_imultiset.begin(), *new_block);
   }
   else if (block->m_size >= nunits){
      m_header.m_imultiset.erase(ittree);
   }
   else{
      assert(0);
      return 0;
   }

   //We need block_ctrl for deallocation stuff, so
   //return memory user can overwrite
   m_header.m_allocated += block->m_size*Alignment;
   received_size =  (block->m_size - AllocatedCtrlUnits)*Alignment;

   //Mark the block as allocated
   priv_mark_as_allocated_block(block);

   //Check alignment
   priv_check_alignment(block);

   //Clear the memory occupied by the tree hook, since this won't be
   //cleared with zero_free_memory
   TreeHook *t = static_cast<TreeHook*>(block);
   std::memset(t, 0, sizeof(*t));
   return priv_get_user_buffer(block);
}

template<class MutexFamily, class VoidPointer>
void rbtree_best_fit<MutexFamily, VoidPointer>::deallocate(void* addr)
{
   if(!addr)   return;
   //-----------------------
   boost::interprocess::scoped_lock<interprocess_mutex> guard(m_header);
   //-----------------------
   return this->priv_deallocate(addr);
}

template<class MutexFamily, class VoidPointer>
void rbtree_best_fit<MutexFamily, VoidPointer>::priv_deallocate(void* addr)
{
   if(!addr)   return;

   block_ctrl *block = priv_get_block(addr);
  
   //The blocks must be marked as allocated and the sizes must be equal
   assert(priv_is_allocated_block(block));
   assert(block->m_size == priv_tail_size(block));

   //Check if alignment and block size are right
   priv_check_alignment(addr);

   std::size_t block_old_size = Alignment*block->m_size;
   assert(m_header.m_allocated >= block_old_size);

   //Update used memory count
   m_header.m_allocated -= block_old_size;

   //The block to insert in the tree
   block_ctrl *block_to_insert = block;

   //Get the next block
   block_ctrl *next_block = priv_next_block(block);

   //Merge if the next is free
   if(!priv_is_allocated_block(next_block)){
      block->m_size += next_block->m_size;
      assert(block->m_size >= BlockCtrlUnits);
      priv_tail_size(block, block->m_size);
      m_header.m_imultiset.erase(Imultiset::iterator_to(*next_block));
   }

   //Get the previous block
   block_ctrl *prev_block = priv_prev_block(block);

   //Now check that tail size and control size are equal
   assert(prev_block->m_size == priv_tail_size(prev_block));

   //Merge if the previous is free
   if(!priv_is_allocated_block(prev_block)){
      prev_block->m_size += block->m_size;
      assert(prev_block->m_size >= BlockCtrlUnits);
      priv_tail_size(prev_block, prev_block->m_size);
      m_header.m_imultiset.erase(Imultiset::iterator_to(*prev_block));
      block_to_insert = prev_block;
   }

   priv_mark_as_free_block(block_to_insert);
   m_header.m_imultiset.insert(m_header.m_imultiset.begin(), *block_to_insert);
}

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_MEM_ALGO_RBTREE_BEST_FIT_HPP

