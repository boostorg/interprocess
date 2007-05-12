//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_NAMED_OBJECT_ALGO_HPP
#define BOOST_INTERPROCESS_NAMED_OBJECT_ALGO_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/config.hpp>
#include <boost/detail/no_exceptions_support.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/type_traits/has_trivial_destructor.hpp>
#include <boost/type_traits/has_nothrow_destructor.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>
#include <boost/interprocess/detail/named_proxy.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/offset_ptr.hpp>
#include <boost/interprocess/indexes/flat_map_index.hpp>
#include <boost/interprocess/indexes/iset_index.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/smart_ptr/scoped_ptr.hpp>
#include <boost/aligned_storage.hpp>
#include <cstddef>
#include <climits>

#include <string>
#include <new>
#include <utility>
#include <cstddef>
#include <typeinfo>
#ifndef BOOST_NO_EXCEPTIONS
#include <exception>
#endif

/*!\file
   Describes the object placed in a memory segment that provides
   named object allocation capabilities for single-segment and
   multi-segment allocations.
*/

namespace boost{  namespace interprocess {  

/// @cond
/*!An integer that describes the type of the instance constructed in memory*/
enum InstanceType {   anonymous_type, named_type, unique_type, max_allocation_type };

template<class CharT>
struct intrusive_compare_key
{
   typedef CharT char_type;

   intrusive_compare_key(const CharT *str, std::size_t len)
      :  mp_str(str), m_len(len)
   {}

   const CharT *  mp_str;
   std::size_t    m_len;
};

namespace detail{

/*!
   This struct indicates an anonymous object creation allocation
*/
class anonymous_instance_t
{
 private:
   anonymous_instance_t(){}
};

/*!
   This struct indicates an unique type indexed instance allocation
*/
class unique_instance_t         
{
 private:
   unique_instance_t(){}
};

}   //namespace detail{
/// @endcond

//These pointers are the ones the user will use to 
//indicate previous allocation types
static const detail::anonymous_instance_t   *anonymous_instance;
static const detail::unique_instance_t      *unique_instance;

}}   //namespace boost{  namespace interprocess{

namespace boost { namespace interprocess { namespace detail {

/// @cond
/*!The key of the the named allocation information index. Stores an offset pointer 
   to a null terminated string and the length of the string to speed up sorting*/
template<class CharT, class VoidPointer>
struct index_key
{
   typedef typename detail::
      pointer_to_other<VoidPointer, const CharT>::type   const_char_ptr_t;
   typedef CharT                                         char_type;

   //Offset pointer to the object's name
   const_char_ptr_t  mp_str;
   //Length of the name buffer (null NOT included)
   std::size_t       m_len;

   /*!Constructor of the key*/
   index_key (const char_type *name, std::size_t length)
      : mp_str(name), m_len(length) {}

   /*!Less than function for index ordering*/
   bool operator < (const index_key & right) const
   {
      return (m_len < right.m_len) || 
               (m_len == right.m_len && 
               std::char_traits<char_type>::compare 
                  (detail::get_pointer(mp_str)
                  ,detail::get_pointer(right.mp_str), m_len) < 0);
   }

   /*!Equal to function for index ordering*/
   bool operator == (const index_key & right) const
   {
      return   m_len == right.m_len && 
               std::char_traits<char_type>::compare 
                  (detail::get_pointer(mp_str),
                   detail::get_pointer(right.mp_str), m_len) == 0;
   }
};

/*!The index_data stores a pointer to a buffer and the element count needed
   to know how many destructors must be called when calling destroy*/
template<class VoidPointer>
struct index_data
{
   typedef VoidPointer void_ptr;
   void_ptr    m_ptr;
   index_data(void *ptr) : m_ptr(ptr){}
};

template<class Base, class CharType>
struct block_header
   :  public Base
{
   std::size_t m_value_bytes;
   std::size_t m_value_alignment :  8;
   std::size_t m_allocation_type :  3;
   std::size_t m_sizeof_char     :  5;
   std::size_t m_num_char        :  16;

   block_header(std::size_t value_bytes
               ,std::size_t value_alignment
               ,std::size_t allocation_type
               ,std::size_t sizeof_char
               ,std::size_t num_char
               )
      :  m_value_bytes(value_bytes)
      ,  m_value_alignment(value_alignment)
      ,  m_allocation_type(allocation_type)
      ,  m_sizeof_char(sizeof_char)
      ,  m_num_char(num_char)
   {};

   block_header &operator= (const Base &b)
   {  Base::operator=(b);  return *this;  }

   template<class T>
   block_header &operator= (const T& )
   {  return *this;  }

   std::size_t total_size() const
   {  
      return detail::char_ptr_cast(this->name()) + (m_num_char+1)*m_sizeof_char
           - detail::char_ptr_cast(this);
   }

   void *value() const
   {  
      return detail::char_ptr_cast(this) + 
         get_rounded_size(sizeof(block_header), m_value_alignment);
   }

   CharType *name() const
   {  
      return reinterpret_cast<CharType*>(detail::char_ptr_cast(this->value()) + 
         get_rounded_size(m_value_bytes, m_sizeof_char));
   }

   bool operator <(const block_header &b) const
   {
      return m_num_char < b.m_num_char ||
             (m_num_char < b.m_num_char && 
               std::char_traits<CharType>::compare(name(), b.name(), m_num_char) < 0);
   }

   bool operator ==(const block_header &b) const
   {
      return m_num_char == b.m_num_char &&
             std::char_traits<CharType>::compare
               (name(), b.name(), m_num_char) == 0;
   }

   template<class T>
   static block_header *block_header_from_value(T *value)
   {  
      
      block_header * hdr = 
         reinterpret_cast<block_header*>(detail::char_ptr_cast(value) - 
         get_rounded_size(sizeof(block_header), boost::alignment_of<T>::value));

      //Some sanity checks
      assert(hdr->m_value_alignment == boost::alignment_of<T>::value);
      assert(hdr->m_value_bytes % sizeof(T) == 0);
      return hdr;
   }
};


template <class InpPtr, class OutPtr>
struct get_construct_name;

template <class OutPtr>
struct get_construct_name<OutPtr, OutPtr>
{
   static const OutPtr *get(const OutPtr *name) 
   {  return name;   }
};

template <class OutPtr>
struct get_construct_name<anonymous_instance_t, OutPtr>
{
   static const OutPtr *get(const anonymous_instance_t *) 
   {  return reinterpret_cast<const OutPtr *>(0);   }
};

template <class OutPtr>
struct get_construct_name<unique_instance_t, OutPtr>
{
   static const OutPtr *get(const unique_instance_t *)
   {  return reinterpret_cast<const OutPtr *>(-1);   }
};

}  //namespace detail {

template<class MemoryAlgorithm>
class restricted_segment_manager
   :  private MemoryAlgorithm
{
   public:
   typedef typename MemoryAlgorithm::void_pointer  void_pointer;
   typedef typename MemoryAlgorithm::mutex_family  mutex_family;

   enum {   PayloadPerAllocation = MemoryAlgorithm::PayloadPerAllocation };

   restricted_segment_manager(std::size_t size, std::size_t reserved_bytes)
      :  MemoryAlgorithm(size, reserved_bytes)
   {
      assert((sizeof(restricted_segment_manager<MemoryAlgorithm>) == sizeof(MemoryAlgorithm)));
   }

   /*!Returns the size of the memory segment*/
   std::size_t get_size() const
   {  return MemoryAlgorithm::get_size();  }

   /*!Obtains the minimum size needed by the segment manager*/
   static std::size_t get_min_size (std::size_t size)
   {  return MemoryAlgorithm::get_min_size(size);  }

   /*!Allocates nbytes bytes. This function is only used in 
      single-segment management. Never throws*/
   void * allocate (std::size_t nbytes, std::nothrow_t)
   {  return MemoryAlgorithm::allocate(nbytes);   }

   /*!Allocates nbytes bytes. This function is only used in 
      single-segment management. Throws bad_alloc when fails*/
   void * allocate(std::size_t nbytes)
   {  
      void * ret = MemoryAlgorithm::allocate(nbytes);
      if(!ret)
         throw bad_alloc();
      return ret;
   }

   /*!Allocates nbytes bytes. This function is only used in 
      single-segment management. Never throws*/
   void * allocate_aligned (std::size_t nbytes, std::size_t alignment, std::nothrow_t)
   {  return MemoryAlgorithm::allocate_aligned(nbytes, alignment);   }

   /*!Allocates nbytes bytes. This function is only used in 
      single-segment management. Throws bad_alloc when fails*/
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

   /*!Deallocates the bytes allocated with allocate/allocate_at_least()
      pointed by addr*/
   void   deallocate          (void *addr)
   {  MemoryAlgorithm::deallocate(addr);   }

   /*!Increases managed memory in extra_size bytes more. This only works
      with single-segment management*/
   void grow(std::size_t extra_size)
   {  MemoryAlgorithm::grow(extra_size);   }

   /*!Returns the result of "all_memory_deallocated()" function
      of the used memory algorithm*/
   bool all_memory_deallocated()
   {   return MemoryAlgorithm::all_memory_deallocated(); }

   /*!Returns the result of "check_sanity()" function
      of the used memory algorithm*/
   bool check_sanity()
   {   return MemoryAlgorithm::check_sanity(); }

   //!Writes to zero free memory (memory not yet allocated) of the memory algorithm
   void clear_free_memory()
   {   MemoryAlgorithm::clear_free_memory(); }
};
/// @endcond

/*!
   This object is placed in the beginning of memory segment and
   implements the allocation (named or anonymous) of portions
   of the segment. This object contains two indexes that
   maintain an association between a name and a portion of the segment. 
   
   The first index contains the mappings for normal named objects using the 
   char type specified in the template parameter.

   The second index contains the association for unique instances. The key will
   be the const char * returned from type_info.name() function for the unique
   type to be constructed.
*/
template<class CharType
        ,class MemoryAlgorithm
        ,template<class IndexConfig> class IndexType>
class segment_manager
   :  private restricted_segment_manager<MemoryAlgorithm>
{ 
   /// @cond
   //Non-copyable
   segment_manager();
   segment_manager(const segment_manager &);
   segment_manager &operator=(const segment_manager &);
   typedef restricted_segment_manager<MemoryAlgorithm> Base;
   /// @endcond

   public:
   typedef typename Base::void_pointer void_pointer;

   enum {   PayloadPerAllocation = Base::PayloadPerAllocation };

   /// @cond
   private:
   template<class CharT>
   struct index_config
   {
      typedef CharT                                   char_type;
      typedef detail::index_key<CharT, void_pointer>  key_type;
      typedef detail::index_data<void_pointer>        mapped_type;
      typedef Base                                    restricted_segment_manager;

      template<class BlockBase>
      struct intrusive_value_type
      {  typedef detail::block_header<BlockBase, CharT>  type; };

      typedef intrusive_compare_key<CharT> intrusive_compare_key_type;
   };

   template<class Index, class CharT>
   struct intrusive_header
   {
      typedef typename Index::value_type type;
   };

   template<class Index, class CharT>
   struct node_header
   {
      typedef detail::block_header<typename Index::iterator, CharT>  type;
   };

   template<class Index, class CharT>
   struct normal_header
   {
      struct empty{};
      typedef detail::block_header<empty, CharT>  type;
   };

   template<class CharT>
   struct select_block_header
   {
      typedef IndexType<index_config<CharT> > index;

      typedef typename boost::mpl::if_c
         < is_intrusive_index<index>::value
         , intrusive_header  <index, CharT>
         , typename boost::mpl::if_c
            < is_node_index<index>::value
            , node_header  <index, CharT>
            , normal_header<index, CharT> 
            >::type
         >::type  header_type;

      typedef typename header_type::type type;
   };

   struct raw_deleter_t
   {
      typedef void * pointer;

      raw_deleter_t(segment_manager &mngr)
         : m_mngr(mngr)
      {}

      void operator()(pointer p)
      {  m_mngr.deallocate(p);      }

      private:
      segment_manager &m_mngr;
   };
   /// @endcond

   public:
   /// @cond
   typedef IndexType<index_config<CharType> >         named_index_t;
   typedef IndexType<index_config<char> >             unique_index_t;

   class char_ptr_holder_t
   {
      public:
      char_ptr_holder_t(const CharType *name) 
         : m_name(detail::get_construct_name<CharType, CharType>::get(name))
      {}

      char_ptr_holder_t(const detail::anonymous_instance_t *name) 
         : m_name(detail::get_construct_name<detail::anonymous_instance_t, CharType>::get(name))
      {}

      char_ptr_holder_t(const detail::unique_instance_t *name) 
         : m_name(detail::get_construct_name<detail::unique_instance_t, CharType>::get(name))
      {}

      operator const CharType *()
      {  return m_name;  }

      private:
      const CharType *m_name;
   };
   /// @endcond

   typedef typename Base::mutex_family        mutex_family;

   /*!Constructor proxy object definition helper class*/
   template<class T, bool dothrow>
   struct construct_proxy
   {
      typedef detail::named_proxy<CharType, T, segment_manager, 
                          false, dothrow, true>   type;
   };

   /*!Find or construct proxy object definition helper class*/
   template<class T, bool dothrow>
   struct find_construct_proxy
   {
      typedef detail::named_proxy<CharType, T, segment_manager, 
                          true, dothrow, true>    type;
   };

   /*!Constructor proxy object definition helper class*/
   template<class T, bool dothrow>
   struct construct_iter_proxy
   {
      typedef detail::named_proxy<CharType, T, segment_manager, 
                             false, dothrow, false>   type;
   };

   /*!Find or construct proxy object definition helper class*/
   template<class T, bool dothrow>
   struct find_construct_iter_proxy
   {
      typedef detail::named_proxy<CharType, T, segment_manager, 
                          true,  dothrow, false>    type;
   };

   /*!Constructor. Can throw*/
   segment_manager(std::size_t size)
      :  Base(size, priv_get_reserved_bytes())
      ,  m_header(static_cast<Base*>(get_this_pointer()))
   {  (void) anonymous_instance;   (void) unique_instance;   }

   /*!Tries to find a previous named allocation address. Returns a memory
      buffer and the object count. */
   template <class T>
   std::pair<T*, std::size_t> find  (const CharType* name)
   {  
      //The name can't be null, no anonymous object can be found by name
      assert(name != 0);
      if(name == reinterpret_cast<const CharType*>(-1)){
         return priv_generic_find<T, char> (typeid(T).name(), m_header.m_unique_index);   
      }
      else{
         return priv_generic_find<T, CharType> (name, m_header.m_named_index);
      }
   }

   template <class T>
   std::pair<T*, std::size_t> find (const detail::unique_instance_t* name)
   {  return priv_generic_find<T, char> (name, m_header.m_unique_index); }

   /*!Returns throwing "construct" proxy object*/
   template <class T>
   typename construct_proxy<T, true>::type      
      construct(char_ptr_holder_t name)
   {  return typename construct_proxy<T, true>::type (name, this);  }

   /*!Returns throwing "search or construct" proxy object*/
   template <class T>
   typename find_construct_proxy<T, true>::type find_or_construct(char_ptr_holder_t name)
   {  return typename find_construct_proxy<T, true>::type(name, this);  }

   /*!Returns no throwing "construct" proxy object*/
   template <class T>
   typename construct_proxy<T, false>::type     
      construct(char_ptr_holder_t name, std::nothrow_t)
   {  return typename construct_proxy<T, false>::type (name, this);  }

   /*!Returns no throwing "search or construct" proxy object*/
   template <class T>
   typename find_construct_proxy<T, false>::type   
      find_or_construct(char_ptr_holder_t name, std::nothrow_t)
   {  return typename find_construct_proxy<T, false>::type  (name, this);  }

   /*!Returns throwing "construct from iterators" proxy object*/
   template <class T>
   typename construct_iter_proxy<T, true>::type     
      construct_it(char_ptr_holder_t name)
   {  return typename construct_iter_proxy<T, true>::type (name, this);  }

   /*!Returns throwing "search or construct from iterators" proxy object*/
   template <class T>
   typename find_construct_iter_proxy<T, true>::type   
      find_or_construct_it(char_ptr_holder_t name)
   {  return typename find_construct_iter_proxy<T, true>::type(name, this);  }

   /*!Returns no throwing "construct from iterators" proxy object*/
   template <class T>
   typename construct_iter_proxy<T, false>::type   
      construct_it(char_ptr_holder_t name, std::nothrow_t)
   {  return typename construct_iter_proxy<T, false>::type (name, this);  }

   /*!Returns no throwing "search or construct from iterators" proxy object*/
   template <class T>
   typename find_construct_iter_proxy<T, false>::type 
      find_or_construct_it(char_ptr_holder_t name, std::nothrow_t)
   {  return typename find_construct_iter_proxy<T, false>::type  (name, this);  }

   /*!Calls object function blocking recursive interprocess_mutex and guarantees that 
      no new named_alloc or destroy will be executed by any process while 
      executing the object function call*/
   template <class Func>
   void atomic_func(Func &f)
   {  boost::interprocess::scoped_lock<rmutex> guard(m_header);  f();  }

   /*!Calls the destructor and makes an unique deallocate*/
   template <class T>
   bool destroy(const detail::unique_instance_t *)
   {
      return this->priv_generic_named_destroy<T, char>
               (typeid(T).name(), m_header.m_unique_index);
   }

   template <class T>
   bool destroy(const CharType *name)
   {
      return this->priv_generic_named_destroy<T, CharType>
               (name, m_header.m_named_index);
   }

   template <class T>
   bool destroy_ptr(const T *ptr)
   {
      //Allow void pointer deletion instantiation to be able to
      //construct void shared_ptrs
      //If we call the destroy_ptr(const void *), the function will return 0
      typedef typename boost::mpl::if_c<boost::is_same<T, void>::value,
                                       char, T>::type data_t;
      return priv_destroy_ptr((const data_t*)ptr);
   }

   /*!Generic named/anonymous new function. Offers all the possibilities, 
      such as throwing, search before creating, and the constructor is 
      encapsulated in an object function.*/
   template<class T, class CtorFunc>
   T * generic_construct(const CharType *name, 
                         std::size_t num, 
                         bool try2find, 
                         bool dothrow,
                         CtorFunc &ctor)
   {
      //Security overflow check
      if(num > ((std::size_t)-1)/sizeof(T)){
         if(dothrow)
            throw bad_alloc();
         else
            return 0;
      }
      if(name == 0){
         return this->priv_generic_anonymous_construct<T, CtorFunc>(num, dothrow, ctor);
      }
      else if(name == reinterpret_cast<const CharType*>(-1)){
         return this->priv_generic_named_construct<char, T, CtorFunc>
                  (unique_type, typeid(T).name(), num, try2find, dothrow, ctor, m_header.m_unique_index);
      }
      else{
         return this->priv_generic_named_construct<CharType, T, CtorFunc>
                  (named_type, name, num, try2find, dothrow, ctor, m_header.m_named_index);
      }
   }

   /*!Returns the name of an object created with construct/find_or_construct
      functions. Does not throw*/
   template<class T>
   const CharType *get_name(const T *ptr)
   {
      //Get header
      typedef typename select_block_header<CharType>::type block_header_t;
      block_header_t *ctrl_data = block_header_t::block_header_from_value(ptr);
      CharType *name = static_cast<CharType*>(ctrl_data->name());
   
      //Sanity checks
      assert(ctrl_data->m_sizeof_char == sizeof(CharType));
      assert(ctrl_data->m_num_char == std::char_traits<CharType>::length(name));

      return name;
   }

   /*!Returns is the the name of an object created with construct/find_or_construct
      functions. Does not throw*/
   template<class T>
   InstanceType get_type(const T *ptr)
   {
      //Get header
      typedef typename select_block_header<CharType>::type block_header_t;
      block_header_t *ctrl_data = block_header_t::block_header_from_value(ptr);
      assert((InstanceType)ctrl_data->m_allocation_type < max_allocation_type);
      return (InstanceType)ctrl_data->m_allocation_type;
   }

   /*!Preallocates needed index resources to optimize the 
      creation of "num" named objects in the managed memory segment.
      Can throw boost::interprocess::bad_alloc if there is no enough memory.*/
   void reserve_named_objects(std::size_t num)
   {  
      //-------------------------------
      boost::interprocess::scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      m_header.m_named_index.reserve(num);  
   }

   /*!Preallocates needed index resources to optimize the 
      creation of "num" unique objects in the managed memory segment.
      Can throw boost::interprocess::bad_alloc if there is no enough memory.*/
   void reserve_unique_objects(std::size_t num)
   {  
      //-------------------------------
      boost::interprocess::scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      m_header.m_unique_index.reserve(num);
   }

   /*!Returns the number of named objects stored in the segment.*/
   std::size_t get_num_named_objects()
   {  
      //-------------------------------
      boost::interprocess::scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      return m_header.m_named_index.size();  
   }

   /*!Returns the number of unique objects stored in the segment.*/
   std::size_t get_num_unique_objects()
   {  
      //-------------------------------
      boost::interprocess::scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      return m_header.m_unique_index.size();  
   }

   /*!Returns the size of the memory segment*/
   std::size_t get_size() const
   {  return Base::get_size();  }

   /*!Obtains the minimum size needed by the segment manager*/
   static std::size_t get_min_size()
   {  return Base::get_min_size(priv_get_reserved_bytes());  }

   /*!Allocates nbytes bytes. Throws boost::interprocess::bad_alloc when
      it can allocate*/
   void * allocate(std::size_t nbytes)
   {  return Base::allocate(nbytes);   }

   /*!Allocates nbytes bytes. This function is only used in 
      single-segment management. Never throws*/
   void * allocate (std::size_t nbytes, std::nothrow_t nothrow)
   {  return Base::allocate(nbytes, nothrow);   }


   /*!Allocates aligned bytes, returns 0 if there is not more memory.
      Alignment must be power of 2*/
   void* allocate_aligned     (std::size_t nbytes, std::size_t alignment)
   {  return Base::allocate_aligned(nbytes, alignment);  }

   /*!Allocates aligned bytes, throws bad_alloc when
      it can allocate. Alignment must be power of 2*/
   void* allocate_aligned     (std::size_t nbytes, std::size_t alignment, std::nothrow_t nothrow)
   {  return Base::allocate_aligned(nbytes, alignment, nothrow);  }

   std::pair<void *, bool>
      allocation_command  (allocation_type command,   std::size_t limit_size,
                           std::size_t preferred_size,std::size_t &received_size,
                           void *reuse_ptr = 0, std::size_t backwards_multiple = 1)
   {  
      return Base::allocation_command
         (command, limit_size, preferred_size, received_size, reuse_ptr, backwards_multiple);
   }

   /*!Deallocates the bytes allocated with allocate/allocate_at_least()
      pointed by addr*/
   void   deallocate          (void *addr)
   {  Base::deallocate(addr);   }

   /*!Increases managed memory in extra_size bytes more. This only works
      with single-segment management*/
   void grow(std::size_t extra_size)
   {  Base::grow(extra_size);   }

   /*!Returns the result of "all_memory_deallocated()" function
      of the used memory algorithm*/
   bool all_memory_deallocated()
   {   return Base::all_memory_deallocated(); }

   /*!Returns the result of "check_sanity()" function
      of the used memory algorithm*/
   bool check_sanity()
   {   return Base::check_sanity(); }

   //!Writes to zero free memory (memory not yet allocated) of the memory algorithm
   void clear_free_memory()
   {   Base::clear_free_memory(); }

   typename named_index_t::iterator named_begin()
   {  return m_header.m_named_index.begin(); }

   typename named_index_t::iterator named_begin() const
   {  return m_header.m_named_index.begin(); }

   typename named_index_t::iterator named_end()
   {  return m_header.m_named_index.end(); }

   typename named_index_t::iterator named_end() const
   {  return m_header.m_named_index.end(); }

   typename unique_index_t::iterator unique_begin()
   {  return m_header.m_unique_index.begin(); }

   typename unique_index_t::iterator unique_begin() const
   {  return m_header.m_unique_index.begin(); }

   typename unique_index_t::iterator unique_end()
   {  return m_header.m_unique_index.end(); }

   typename unique_index_t::iterator unique_end() const
   {  return m_header.m_unique_index.end(); }

   /// @cond
   private:
   static std::size_t priv_get_reserved_bytes()
   {
      
      //Get the number of bytes until the end of (*this)
      //beginning in the end of the Base base.
      return   (detail::char_ptr_cast((segment_manager*)0) + sizeof(segment_manager))   -
               (detail::char_ptr_cast(static_cast<Base*>((segment_manager*)0)) + sizeof(Base));
   }

   template <class T>
   bool priv_destroy_ptr(const T *ptr)
   {
      //TO-DO: Although we now that the block_header is equal for both indexes
      //we should find a cleaner way to do this.
      typedef typename select_block_header<CharType>::type block_header_t;
      block_header_t *ctrl_data = block_header_t::block_header_from_value(ptr);
      switch(ctrl_data->m_allocation_type){
         case anonymous_type:
            return this->priv_generic_anonymous_destroy<T>(ptr);
         break;

         case named_type:
            {
               return this->priv_generic_named_destroy<T, CharType>
                  (ctrl_data, m_header.m_named_index);
            }
         break;

         case unique_type:
            {
               typedef typename select_block_header<char>::type block_header_unique_t;
               return this->priv_generic_named_destroy<T, char>
                  (block_header_unique_t::block_header_from_value(ptr), m_header.m_unique_index);
            }
         break;

         default:
            //This type is unknown, bad pointer passed to this function!
            assert(0);
            return false;
         break;
      }
      return false;
   }

   template <class T, class CharT>
   std::pair<T*, std::size_t> priv_generic_find
      (const CharT* name, 
       IndexType<index_config<CharT> > &index)
   {
      const bool is_intrusive = 
         is_intrusive_index<IndexType<index_config<CharT> > >::value;
      return priv_generic_find<T, CharT>
               (name, index, boost::mpl::bool_<is_intrusive>());
   }

   template <class T, class CharT>
   std::pair<T*, std::size_t> priv_generic_find
      (const CharT* name, 
       IndexType<index_config<CharT> > &index,
       boost::mpl::true_)
   {
      typedef typename select_block_header<CharT>::type  block_header_t;
      typedef IndexType<index_config<CharT> >         index_type;
      typedef detail::index_key<CharT, void_pointer>  index_key_t;
      typedef typename index_type::iterator        index_it;

      //-------------------------------
      boost::interprocess::scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      //Find name in index
      intrusive_compare_key<CharT> key
         (name, std::char_traits<CharT>::length(name));
      index_it it = index.find(key);

      //Initialize return values
      T *ret_ptr           = 0;
      std::size_t ret_num  = 0;

      //If found, assign values
      if(it != index.end()){
         //Get header
         block_header_t *ctrl_data = &*it;

         //Sanity check
         assert((ctrl_data->m_value_bytes % sizeof(T)) == 0);
         assert(ctrl_data->m_sizeof_char == sizeof(CharT));
         ret_ptr  = static_cast<T*>(ctrl_data->value());
         ret_num  = ctrl_data->m_value_bytes/sizeof(T);
      }
      return std::make_pair(ret_ptr, ret_num);
   }

   template <class T, class CharT>
   std::pair<T*, std::size_t> priv_generic_find
      (const CharT* name, 
       IndexType<index_config<CharT> > &index,
       boost::mpl::false_)
   {
      typedef IndexType<index_config<CharT> >      index_type;
      typedef typename index_type::key_type        key_type;
      typedef typename index_type::iterator        index_it;
      typedef typename select_block_header<CharT>::type  block_header_t;

      //-------------------------------
      boost::interprocess::scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      //Find name in index
      index_it it = index.find(key_type(name, std::char_traits<CharT>::length(name)));

      //Initialize return values
      T *ret_ptr           = 0;
      std::size_t ret_num  = 0;

      //If found, assign values
      if(it != index.end()){
         //Get header
         block_header_t *ctrl_data = reinterpret_cast<block_header_t*>
                                    (detail::get_pointer(it->second.m_ptr));

         //Sanity check
         assert((ctrl_data->m_value_bytes % sizeof(T)) == 0);
         assert(ctrl_data->m_sizeof_char == sizeof(CharT));
         ret_ptr  = static_cast<T*>(ctrl_data->value());
         ret_num  = ctrl_data->m_value_bytes/sizeof(T);
      }
      return std::make_pair(ret_ptr, ret_num);
   }

   /*!Calls the destructor and makes an anonymous deallocate*/
   template <class T>
   bool priv_generic_anonymous_destroy(const T *object)
   {
      if(!object)
         return false;

      //Get control data from associated with this object    
      typedef typename select_block_header<CharType>::type block_header_t;
      block_header_t *ctrl_data = block_header_t::block_header_from_value(object);

      //-------------------------------
      boost::interprocess::scoped_lock<rmutex> guard(m_header);
      //-------------------------------

      if(ctrl_data->m_allocation_type != anonymous_type){
         //This is not an anonymous object, the pointer is wrong!
         assert(0);
         return false;
      }

      //Call destructors and free memory
      this->priv_array_destroy
            (ctrl_data, object, ctrl_data->m_value_bytes/sizeof(T));
      return true;
   }

   template <class T, class CharT>
   bool priv_generic_named_destroy(typename select_block_header<CharT>::type *ctrl_data, 
                                   IndexType<index_config<CharT> > &index)
   {
      typedef IndexType<index_config<CharT> >            index_type;
      return this->priv_generic_named_destroy<T, CharT>
         (ctrl_data, index, boost::mpl::bool_<is_node_index<index_type>::value>());
   }

   template <class T, class CharT>
   bool priv_generic_named_destroy(typename select_block_header<CharT>::type *ctrl_data, 
                                   IndexType<index_config<CharT> > &index,
                                   boost::mpl::true_)
   {
      return this->priv_generic_named_destroy_impl<T, CharT>(*ctrl_data, index);
   }

   template <class T, class CharT>
   bool priv_generic_named_destroy(typename select_block_header<CharT>::type *ctrl_data, 
                                   IndexType<index_config<CharT> > &index,
                                   boost::mpl::false_)
   {
      CharT *name = static_cast<CharT*>(ctrl_data->name());
      return this->priv_generic_named_destroy<T, CharT>(name, index);
   }

   template <class T, class CharT>
   bool priv_generic_named_destroy(const CharT *name, 
                                   IndexType<index_config<CharT> > &index)
   {
      const bool is_intrusive = 
         is_intrusive_index<IndexType<index_config<CharT> > >::value;
      return priv_generic_named_destroy<T, CharT>
               (name, index,boost::mpl::bool_<is_intrusive>());
   }

   template <class T, class CharT>
   bool priv_generic_named_destroy(const CharT *name, 
                                   IndexType<index_config<CharT> > &index,
                                   boost::mpl::true_)
   {
      typedef typename select_block_header<CharT>::type  block_header_t;
      typedef IndexType<index_config<CharT> >         index_type;
      typedef detail::index_key<CharT, void_pointer>  index_key_t;
      typedef typename index_type::iterator        index_it;
      
      //-------------------------------
      boost::interprocess::scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      //Find name in index
      intrusive_compare_key<CharT> key
         (name, std::char_traits<CharT>::length(name));
      index_it it = index.find(key);

      //If not found, return false
      if(it == index.end()){
         //This name is not present in the index, wrong pointer or name!
         //assert(0);
         return false;
      }

      block_header_t *ctrl_data = &*it;
      void *memory = ctrl_data;
      T *values = static_cast<T*>(ctrl_data->value());
      std::size_t num = ctrl_data->m_value_bytes/sizeof(T);
      
      //Sanity check
      assert((ctrl_data->m_value_bytes % sizeof(T)) == 0);
      assert(sizeof(CharT) == ctrl_data->m_sizeof_char);

      //Erase node from index
      index.erase(it);

      //Destroy the header
      ctrl_data->~block_header_t();

      //Call destructors and free memory
      priv_array_destroy(memory, values, num);
      return true;
   }

   template <class T, class CharT>
   bool priv_generic_named_destroy(const CharT *name, 
                                   IndexType<index_config<CharT> > &index,
                                   boost::mpl::false_)
   {
      typedef IndexType<index_config<CharT> >            index_type;
      typedef typename index_type::iterator              index_it;
      typedef typename index_type::key_type              key_type;

      //-------------------------------
      boost::interprocess::scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      //Try to find the name in the index
      index_it it = index.find(key_type (name, 
                                     std::char_traits<CharT>::length(name)));

      //If not found, return false
      if(it == index.end()){
         //This name is not present in the index, wrong pointer or name!
         assert(0);
         return false;
      }
      return this->priv_generic_named_destroy_impl<T, CharT>(it, index);
   }

   template <class T, class CharT>
   bool priv_generic_named_destroy
      (const typename IndexType<index_config<CharT> >::iterator &it,
       IndexType<index_config<CharT> > &index)
   {
      //-------------------------------
      boost::interprocess::scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      return this->priv_generic_named_destroy_impl<T, CharT>(it, index);
   }

   template <class T, class CharT>
   bool priv_generic_named_destroy_impl
      (const typename IndexType<index_config<CharT> >::iterator &it,
      IndexType<index_config<CharT> > &index)
   {
      typedef IndexType<index_config<CharT> >      index_type;
      typedef typename index_type::iterator        index_it;
      typedef typename select_block_header<CharT>::type     block_header_t;

      //Get allocation parameters
      void *memory            = detail::get_pointer(it->second.m_ptr);
      char *stored_name       = detail::char_ptr_cast
                                 (detail::get_pointer(it->first.mp_str));
      (void)stored_name;
      //Check if the distance between the name pointer and the memory pointer 
      //is correct (this can detect incorrect T type in destruction)
      block_header_t *ctrl_data = reinterpret_cast<block_header_t*>(memory);
      std::size_t num = ctrl_data->m_value_bytes/sizeof(T);
      T *values = static_cast<T*>(ctrl_data->value());
      
      //Sanity check
      assert((ctrl_data->m_value_bytes % sizeof(T)) == 0);
      assert((void*)stored_name == ctrl_data->name());
      assert(sizeof(CharT) == ctrl_data->m_sizeof_char);

      //Erase node from index
      index.erase(it);

      //Destroy the header
      ctrl_data->~block_header_t();

      //Call destructors and free memory
      priv_array_destroy(memory, values, num);
      return true;
   }

   template <class T>
   void priv_array_destroy(const void * memory, T *objects, std::size_t count)
   {
      //Build scoped ptr to avoid leaks with destructor exception
      boost::interprocess::scoped_ptr<void, raw_deleter_t> mem((void*)memory, *this);

      if(boost::has_trivial_destructor<T>::value)
         return;

      //Call destructors in reverse order, if one throws catch it and
      //continue destroying ignoring further exceptions. 
      if(!boost::has_nothrow_destructor<T>::value){
         while(count--){
            objects[count].~T();
         }
      }
      else{
         BOOST_TRY {
            while(count--){
               objects[count].~T();
            }
         }
         BOOST_CATCH(...){
            //Destroy the rest of objects ignoring exceptions
            while(count--){
               BOOST_TRY         { objects[count].~T(); }
               //Ignore exceptions
               BOOST_CATCH(...)  { }
               BOOST_CATCH_END
            }
            //throws first exception
            BOOST_RETHROW
         }
         BOOST_CATCH_END
      }
   }

   template<class Ctor>
   void priv_array_construct(typename Ctor::target_t *ptr, 
                               std::size_t num, Ctor &ctor)
   {
      typedef typename Ctor::target_t target_t;
      //Try constructors
      std::size_t counter = 0;
      BOOST_TRY{
         for(; counter < num; ++counter, ++ctor){
            ctor(&ptr[counter]);
         }
      }
      //If there is an exception call destructors and erase index node
      BOOST_CATCH(...){
         if(!boost::has_trivial_destructor<target_t>::value){
            for(; counter; --counter){
               BOOST_TRY         { ptr[counter].~target_t(); }
               //Ignore exceptions, we will throw the original 
               //exception to free acquired locks
               BOOST_CATCH(...)  {}
               BOOST_CATCH_END
            }
         }
         BOOST_RETHROW;
      }
      BOOST_CATCH_END
   }

   /*!Generic named new function for named functions*/
   template<class CharT, class T, class CtorFunc>
   T * priv_generic_named_construct(std::size_t type,  
                               const CharT *name,
                               std::size_t num, 
                               bool try2find, 
                               bool dothrow,
                               CtorFunc &ctor,
                               IndexType<index_config<CharT> > &index)
   {
      const bool is_intrusive = 
         is_intrusive_index<IndexType<index_config<CharT> > >::value;
      return priv_generic_named_construct<CharT, T, CtorFunc>
               (type, name, num, try2find, dothrow, ctor
               ,index, boost::mpl::bool_<is_intrusive>());

   }

   template<class CharT, class T, class CtorFunc>
   T * priv_generic_named_construct(std::size_t type,  
                               const CharT *name,
                               std::size_t num, 
                               bool try2find, 
                               bool dothrow,
                               CtorFunc &ctor,
                               IndexType<index_config<CharT> > &index,
                               boost::mpl::true_)
   {
      std::size_t namelen  = std::char_traits<CharT>::length(name);

      typedef typename select_block_header<CharT>::type block_header_t;
      block_header_t block_info ( sizeof(T)*num
                                 , boost::alignment_of<T>::value
                                 , type
                                 , sizeof(CharT)
                                 , namelen);

      typedef IndexType<index_config<CharT> >            index_type;
      typedef typename index_type::iterator              index_it;
      typedef std::pair<index_it, bool>                  index_ib;

      //-------------------------------
      boost::interprocess::scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      //Insert the node. This can throw.
      //First, we want to know if the key is already present before
      //we allocate any memory, and if the key is not present, we 
      //want to allocate all memory in a single buffer that will
      //contain the name and the user buffer.
      //
      //Since equal_range(key) + insert(hint, value) approach is
      //quite inefficient in container implementations 
      //(they re-test if the position is correct), I've chosen
      //to insert the node, do an ugly un-const cast and modify
      //the key (which is a smart pointer) to an equivalent one
      index_ib insert_ret;

      typename index_type::insert_commit_data commit_data;

      BOOST_TRY{
         intrusive_compare_key<CharT> key(name, namelen);
         insert_ret = index.insert_check(key, commit_data);
      }
      //Ignore exceptions
      BOOST_CATCH(...){
         if(dothrow)
            BOOST_RETHROW;
         return 0;
      }
      BOOST_CATCH_END

      index_it it = insert_ret.first;

      //If found and this is find or construct, return data
      //else return null
      if(!insert_ret.second){
         if(try2find){
            return static_cast<T*>(it->value());
         }
         return 0;
      }

      //Allocates buffer for name + data, this can throw (it hurts)
      void *buffer_ptr; 

      //Check if there is enough memory
      if(dothrow){
         buffer_ptr = this->allocate(block_info.total_size());
      }
      else{
         buffer_ptr = this->allocate(block_info.total_size(), std::nothrow_t());
         if(!buffer_ptr)
            return 0; 
      }

      //Now construct the header
      block_header_t * hdr = new(buffer_ptr) block_header_t(block_info);
      T *ptr = static_cast<T*>(hdr->value());

      //Copy name to memory segment and insert data
      CharT *name_ptr = static_cast<CharT *>(hdr->name());
      std::char_traits<CharT>::copy(name_ptr, name, namelen+1);

      BOOST_TRY{
         //Now commit the insertion using previous context data
         it = index.insert_commit(*hdr, commit_data);
      }
      //Ignore exceptions
      BOOST_CATCH(...){
         if(dothrow)
            BOOST_RETHROW;
         return 0;
      }
      BOOST_CATCH_END

      //Initialize the node value_eraser to erase inserted node
      //if something goes wrong
      detail::value_eraser<index_type> value_eraser(index, it);
      
      //Avoid constructions if constructor is trivial
      if(!CtorFunc::is_trivial){
         //Build scoped ptr to avoid leaks with constructor exception
         boost::interprocess::scoped_ptr<void, raw_deleter_t> mem(buffer_ptr, *this);

         //Construct array, this can throw
         this->priv_array_construct<CtorFunc>(ptr, num, ctor);

         //All constructors successful, we don't want to release memory
         mem.release();
      }
      //Release node value_eraser since construction was successful
      value_eraser.release();
      return ptr;
   }

   /*!Generic named new function for named functions*/
   template<class CharT, class T, class CtorFunc>
   T * priv_generic_named_construct(std::size_t type,  
                               const CharT *name,
                               std::size_t num, 
                               bool try2find, 
                               bool dothrow,
                               CtorFunc &ctor,
                               IndexType<index_config<CharT> > &index,
                               boost::mpl::false_)
   {
      std::size_t namelen  = std::char_traits<CharT>::length(name);

      typedef typename select_block_header<CharT>::type block_header_t;
      block_header_t block_info ( sizeof(T)*num
                                 , boost::alignment_of<T>::value
                                 , type
                                 , sizeof(CharT)
                                 , namelen);

      typedef IndexType<index_config<CharT> >            index_type;
      typedef typename index_type::key_type              key_type;
      typedef typename index_type::mapped_type           mapped_type;
      typedef typename index_type::value_type            value_type;
      typedef typename index_type::iterator              index_it;
      typedef std::pair<index_it, bool>                  index_ib;

      //-------------------------------
      boost::interprocess::scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      //Insert the node. This can throw.
      //First, we want to know if the key is already present before
      //we allocate any memory, and if the key is not present, we 
      //want to allocate all memory in a single buffer that will
      //contain the name and the user buffer.
      //
      //Since equal_range(key) + insert(hint, value) approach is
      //quite inefficient in container implementations 
      //(they re-test if the position is correct), I've chosen
      //to insert the node, do an ugly un-const cast and modify
      //the key (which is a smart pointer) to an equivalent one
      index_ib insert_ret;
      BOOST_TRY{
         insert_ret = index.insert(value_type(key_type (name, namelen), 
                                                      mapped_type(0)));
      }
      //Ignore exceptions
      BOOST_CATCH(...){
         if(dothrow)
            BOOST_RETHROW;
         return 0;
      }
      BOOST_CATCH_END

      index_it it = insert_ret.first;

      //If found and this is find or construct, return data
      //else return null
      if(!insert_ret.second){
         if(try2find){
            block_header_t *hdr = static_cast<block_header_t*>
               (detail::get_pointer(it->second.m_ptr));
            return static_cast<T*>(hdr->value());
         }
         return 0;
      }
      //Initialize the node value_eraser to erase inserted node
      //if something goes wrong
      detail::value_eraser<index_type> value_eraser(index, it);
      const bool NodeIndex = is_node_index<index_type>::value;   //change this

      //Allocates buffer for name + data, this can throw (it hurts)
      void *buffer_ptr; 

      //Check if there is enough memory
      if(dothrow){
         buffer_ptr = this->allocate(block_info.total_size());
      }
      else{
         buffer_ptr = this->allocate(block_info.total_size(), std::nothrow_t());
         if(!buffer_ptr)
            return 0; 
      }

      //Now construct the header
      block_header_t * hdr = new(buffer_ptr) block_header_t(block_info);
      T *ptr = static_cast<T*>(hdr->value());

      //Copy name to memory segment and insert data
      CharT *name_ptr = static_cast<CharT *>(hdr->name());
      std::char_traits<CharT>::copy(name_ptr, name, namelen+1);

      //If this is a node container, store also the iterator
      if(NodeIndex){
         *hdr = it;
      }

      //Do the ugly cast, please mama, forgive me!
      //This new key points to an identical string, so it must have the 
      //same position than the overwritten key according to the predicate
      const_cast<key_type &>(it->first).mp_str  = name_ptr;
      it->second.m_ptr                          = buffer_ptr;

      //Avoid constructions if constructor is trivial
      if(!CtorFunc::is_trivial){
         //Build scoped ptr to avoid leaks with constructor exception
         boost::interprocess::scoped_ptr<void, raw_deleter_t> mem(buffer_ptr, *this);

         //Construct array, this can throw
         this->priv_array_construct<CtorFunc>(ptr, num, ctor);

         //All constructors successful, we don't want to release memory
         mem.release();
      }
      //Release node value_eraser since construction was successful
      value_eraser.release();
      return ptr;
   }

   /*!Generic anonymous "new" function.*/
   template<class T, class CtorFunc>
   T * priv_generic_anonymous_construct(std::size_t num, 
                                        bool dothrow,
                                        CtorFunc &ctor)
   {
      typedef typename select_block_header<CharType>::type block_header_t;
      block_header_t block_info ( sizeof(T)*num
                                 , boost::alignment_of<T>::value
                                 , anonymous_type
                                 , sizeof(CharType)
                                 , 0);

      //-------------------------------
      boost::interprocess::scoped_lock<rmutex> guard(m_header);
      //-------------------------------

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
      boost::interprocess::scoped_ptr<void, raw_deleter_t> mem(ptr_struct, *this);

      //Now construct the header
      block_header_t * hdr = new(ptr_struct) block_header_t(block_info);
      T *ptr = static_cast<T*>(hdr->value());

      if(!CtorFunc::is_trivial){
         //Construct array, this can throw
         priv_array_construct<CtorFunc>(ptr, num, ctor);
      }

      //All constructors successful, we don't want erase memory
      mem.release();
      return ptr;
   }

   private:

   /*!Returns the this pointer*/ 
   segment_manager *get_this_pointer()
   {  return this;  }

   typedef typename MemoryAlgorithm::mutex_family::recursive_mutex_type   rmutex;

   /*!This struct includes needed data and derives from
      rmutex to allow EBO when using null interprocess_mutex*/
   struct header_t
      :  public rmutex
   {
      named_index_t           m_named_index;
      unique_index_t          m_unique_index;
   
      header_t(Base *restricted_segment_mngr)
         :  m_named_index (restricted_segment_mngr)
         ,  m_unique_index(restricted_segment_mngr)
      {}
   }  m_header;
   /// @endcond
};


}} //namespace boost { namespace interprocess

#include <boost/interprocess/detail/config_end.hpp>

#endif //#ifndef BOOST_INTERPROCESS_NAMED_OBJECT_ALGO_HPP

