//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_BASIC_SEGMENT_MANAGER_HPP
#define BOOST_INTERPROCESS_BASIC_SEGMENT_MANAGER_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/detail/no_exceptions_support.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/in_place_interface.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <cstddef>   //std::size_t
#include <string>    //char_traits
#include <new>       //std::nothrow
#include <utility>   //std::pair
#ifndef BOOST_NO_EXCEPTIONS
#include <exception>
#endif

/*!\file
   Describes the object placed in a memory segment that provides
   named object allocation capabilities for single-segment and
   multi-segment allocations.
*/

namespace boost{
namespace interprocess{
namespace detail{

template<class MemoryAlgorithm>
class mem_algo_deallocator
{
   void *            m_ptr;
   MemoryAlgorithm & m_algo;

   public:
   mem_algo_deallocator(void *ptr, MemoryAlgorithm &algo)
      :  m_ptr(ptr), m_algo(algo)
   {}

   void release()
   {  m_ptr = 0;  }

   ~mem_algo_deallocator()
   {  if(m_ptr) m_algo.deallocate(m_ptr);  }
};

//!An integer that describes the type of the
//!instance constructed in memory
enum instance_type {   anonymous_type, named_type, unique_type, max_allocation_type };

/// @cond
struct block_header
{
   std::size_t    m_value_bytes;
   unsigned short m_num_char;
   unsigned char  m_value_alignment;
   unsigned char  m_alloc_type_sizeof_char;

   block_header(std::size_t value_bytes
               ,std::size_t value_alignment
               ,std::size_t allocation_type
               ,std::size_t sizeof_char
               ,std::size_t num_char
               )
      :  m_value_bytes(value_bytes)
      ,  m_num_char(num_char)
      ,  m_value_alignment(value_alignment)
      ,  m_alloc_type_sizeof_char
         ( ((unsigned char)allocation_type << 5u) | 
           ((unsigned char)sizeof_char & 0x1F)   )
   {};


   template<class T>
   block_header &operator= (const T& )
   {  return *this;  }

   std::size_t total_size() const
   {
      if(allocation_type() != detail::anonymous_type){
         return name_offset() + (m_num_char+1)*sizeof_char();
      }
      else{
         return value_offset() + m_value_bytes;
      }
   }

   template<class Header>
   std::size_t total_size_with_header() const
   {
      return get_rounded_size
               (  sizeof(Header)
               ,  boost::alignment_of<block_header>::value)
           + total_size();
   }

   std::size_t allocation_type() const
   {  return (m_alloc_type_sizeof_char >> 5u)&(unsigned char)0x7;  }

   std::size_t sizeof_char() const
   {  return m_alloc_type_sizeof_char & (unsigned char)0x1F;  }

   template<class CharType>
   CharType *name() const
   {  
      return reinterpret_cast<CharType*>
      (detail::char_ptr_cast(this) + name_offset());
   }

   std::size_t name_length() const
   {  return m_num_char;   }

   std::size_t name_offset() const
   {  
      return value_offset() + get_rounded_size(m_value_bytes, sizeof_char());
   }

   void *value() const
   {  
      return detail::char_ptr_cast(this) + value_offset();
   }

   std::size_t value_offset() const
   {  
      return get_rounded_size(sizeof(block_header), m_value_alignment);
   }

   template<class CharType>
   bool less(const block_header &b) const
   {
      return m_num_char < b.m_num_char ||
             (m_num_char < b.m_num_char && 
               std::char_traits<CharType>::compare
                  (name<CharType>(), b.name<CharType>(), m_num_char) < 0);
   }

   template<class CharType>
   bool equal(const block_header &b) const
   {
      return m_num_char == b.m_num_char &&
             std::char_traits<CharType>::compare
               (name<CharType>(), b.name<CharType>(), m_num_char) == 0;
   }

   template<class T>
   static block_header *block_header_from_value(T *value)
   {  return block_header_from_value(value, sizeof(T), boost::alignment_of<T>::value);  }

   static block_header *block_header_from_value(const void *value, std::size_t sz, std::size_t algn)
   {  
      block_header * hdr = 
         reinterpret_cast<block_header*>(detail::char_ptr_cast(value) - 
         get_rounded_size(sizeof(block_header), algn));
      (void)sz;
      //Some sanity checks
      assert(hdr->m_value_alignment == algn);
      assert(hdr->m_value_bytes % sz == 0);
      return hdr;
   }

   template<class Header>
   static block_header *from_first_header(Header *header)
   {  
      block_header * hdr = 
         reinterpret_cast<block_header*>(detail::char_ptr_cast(header) + 
         get_rounded_size(sizeof(Header), boost::alignment_of<block_header>::value));
      //Some sanity checks
      return hdr;
   }

   template<class Header>
   static Header *to_first_header(block_header *bheader)
   {  
      Header * hdr = 
         reinterpret_cast<Header*>(detail::char_ptr_cast(bheader) - 
         get_rounded_size(sizeof(Header), boost::alignment_of<block_header>::value));
      //Some sanity checks
      return hdr;
   }
};

inline void array_construct(void *mem, std::size_t num, detail::in_place_interface &table)
{
   //Try constructors
   std::size_t constructed = 0;
   BOOST_TRY{
      table.construct_n(mem, num, constructed);
   }
   //If there is an exception call destructors and erase index node
   BOOST_CATCH(...){
      std::size_t destroyed = 0;
      table.destroy_n(mem, constructed, destroyed);
      BOOST_RETHROW;
   }
   BOOST_CATCH_END
}

template<class MemoryAlgorithm>
class basic_segment_manager
   :  private MemoryAlgorithm
{
   public:
   typedef typename MemoryAlgorithm::void_pointer  void_pointer;
   typedef typename MemoryAlgorithm::mutex_family  mutex_family;

   MemoryAlgorithm &memory_algorithm()
   {  return *this;  }

   const MemoryAlgorithm &memory_algorithm() const
   {  return *this;  }

   enum {   PayloadPerAllocation = MemoryAlgorithm::PayloadPerAllocation };

   basic_segment_manager(std::size_t size, std::size_t reserved_bytes)
      :  MemoryAlgorithm(size, reserved_bytes)
   {
      assert((sizeof(basic_segment_manager<MemoryAlgorithm>) == sizeof(MemoryAlgorithm)));
   }

   //!Returns the size of the memory segment
   std::size_t get_size() const
   {  return MemoryAlgorithm::get_size();  }

   //!Returns the number of unallocated bytes of the memory segment
   std::size_t get_free_memory() const
   {  return MemoryAlgorithm::get_free_memory();  }

   //!Obtains the minimum size needed by the segment manager
   static std::size_t get_min_size (std::size_t size)
   {  return MemoryAlgorithm::get_min_size(size);  }

   //!Allocates nbytes bytes. This function is only used in 
   //!single-segment management. Never throws
   void * allocate (std::size_t nbytes, std::nothrow_t)
   {  return MemoryAlgorithm::allocate(nbytes);   }

   //!Allocates nbytes bytes. This function is only used in 
   //!single-segment management. Throws bad_alloc when fails
   void * allocate(std::size_t nbytes)
   {  
      void * ret = MemoryAlgorithm::allocate(nbytes);
      if(!ret)
         throw bad_alloc();
      return ret;
   }

   //!Allocates nbytes bytes. This function is only used in 
   //!single-segment management. Never throws
   void * allocate_aligned (std::size_t nbytes, std::size_t alignment, std::nothrow_t)
   {  return MemoryAlgorithm::allocate_aligned(nbytes, alignment);   }

   //!Allocates nbytes bytes. This function is only used in 
   //!single-segment management. Throws bad_alloc when fails
   void * allocate_aligned(std::size_t nbytes, std::size_t alignment)
   {  
      void * ret = MemoryAlgorithm::allocate_aligned(nbytes, alignment);
      if(!ret)
         throw bad_alloc();
      return ret;
   }

   std::pair<void *, bool>
      allocation_command  (allocation_type command,   std::size_t limit_size,
                           std::size_t preferred_size,std::size_t &received_size,
                           void *reuse_ptr = 0, std::size_t backwards_multiple = 1)
   {
      std::pair<void *, bool> ret = MemoryAlgorithm::allocation_command
         ( command | nothrow_allocation, limit_size, preferred_size, received_size
         , reuse_ptr, backwards_multiple);
      if(!(command & nothrow_allocation) && !ret.first)
         throw bad_alloc();
      return ret;
   }

   //!Deallocates the bytes allocated with allocate/allocate_at_least()
   //!pointed by addr
   void   deallocate          (void *addr)
   {  MemoryAlgorithm::deallocate(addr);   }

   //!Increases managed memory in extra_size bytes more. This only works
   //!with single-segment management*
   void grow(std::size_t extra_size)
   {  MemoryAlgorithm::grow(extra_size);   }

   //!Returns the result of "all_memory_deallocated()" function
   //!of the used memory algorithm
   bool all_memory_deallocated()
   {   return MemoryAlgorithm::all_memory_deallocated(); }

   //!Returns the result of "check_sanity()" function
   //!of the used memory algorithm
   bool check_sanity()
   {   return MemoryAlgorithm::check_sanity(); }

   //!Writes to zero free memory (memory not yet allocated) of the memory algorithm
   void zero_free_memory()
   {   MemoryAlgorithm::zero_free_memory(); }

   /// @cond
   protected:
   void * prot_anonymous_construct
      (std::size_t num, bool dothrow, detail::in_place_interface &table)
   {
      typedef detail::block_header block_header_t;
      block_header_t block_info (  table.size*num
                                 , table.alignment
                                 , detail::anonymous_type
                                 , 1
                                 , 0);

      //Allocate memory
      void *ptr_struct = this->allocate(block_info.total_size(), std::nothrow_t());

      //Check if there is enough memory
      if(!ptr_struct){
         if(dothrow){
            throw bad_alloc();
         }
         else{
            return 0; 
         }
      }

      //Build scoped ptr to avoid leaks with constructor exception
      detail::mem_algo_deallocator<MemoryAlgorithm> mem(ptr_struct, *this);

      //Now construct the header
      block_header_t * hdr = new(ptr_struct) block_header_t(block_info);
      void *ptr = hdr->value();

      //Now call constructors
      detail::array_construct(ptr, num, table);

      //All constructors successful, we don't want erase memory
      mem.release();
      return ptr;
   }

   //!Calls the destructor and makes an anonymous deallocate
   bool prot_anonymous_destroy(const void *object, detail::in_place_interface &table)
   {
      if(!object)
         return false;

      //Get control data from associated with this object    
      typedef detail::block_header block_header_t;
      block_header_t *ctrl_data = block_header_t::block_header_from_value(object, table.size, table.alignment);

      //-------------------------------
      //boost::interprocess::scoped_lock<rmutex> guard(m_header);
      //-------------------------------

      if(ctrl_data->allocation_type() != detail::anonymous_type){
         //This is not an anonymous object, the pointer is wrong!
         assert(0);
         return false;
      }

      //Call destructors and free memory
      //Build scoped ptr to avoid leaks with destructor exception
      std::size_t destroyed = 0;
      table.destroy_n((void*)object, ctrl_data->m_value_bytes/table.size, destroyed);
      this->deallocate(ctrl_data);
      return true;
   }
};

}  //namespace detail {
}} //namespace boost { namespace interprocess

#include <boost/interprocess/detail/config_end.hpp>

#endif //#ifndef BOOST_INTERPROCESS_BASIC_SEGMENT_MANAGER_HPP

