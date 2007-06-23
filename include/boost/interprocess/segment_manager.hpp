//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_SEGMENT_MANAGER_HPP
#define BOOST_INTERPROCESS_SEGMENT_MANAGER_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/detail/no_exceptions_support.hpp>
#include <boost/interprocess/detail/type_traits.hpp>

#include <boost/interprocess/detail/iterators.hpp>

#include <boost/interprocess/detail/mpl.hpp>
#include <boost/interprocess/detail/basic_segment_manager.hpp>
#include <boost/interprocess/detail/named_proxy.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/offset_ptr.hpp>
#include <boost/interprocess/indexes/iset_index.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <cstddef>   //std::size_t
#include <string>    //char_traits
#include <new>       //std::nothrow
#include <utility>   //std::pair
#ifndef BOOST_NO_EXCEPTIONS
#include <exception>
#endif

//!\file
//!Describes the object placed in a memory segment that provides
//!named object allocation capabilities for single-segment and
//!multi-segment allocations.

namespace boost{
namespace interprocess{

/// @cond

namespace detail{

//Anti-exception node eraser
template<class Cont>
class value_eraser
{
   public:
   value_eraser(Cont & cont, typename Cont::iterator it) 
      : m_cont(cont), m_index_it(it), m_erase(true){}
   ~value_eraser()  
   {  if(m_erase) m_cont.erase(m_index_it);  }

   void release() {  m_erase = false;  }

   private:
   Cont                   &m_cont;
   typename Cont::iterator m_index_it;
   bool                    m_erase;
};

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

//!This struct indicates an anonymous object creation
//!allocation
template<instance_type type>
class instance_t
{
   instance_t(){}
};

template<class T>
struct char_if_void
{
   typedef T type;
};

template<>
struct char_if_void<void>
{
   typedef char type;
};

typedef instance_t<anonymous_type>  anonymous_instance_t;
typedef instance_t<unique_type>     unique_instance_t;

template<class Hook, class CharType>
struct intrusive_value_type_impl
   :  public Hook
{
   private:
   //Non-copyable
   intrusive_value_type_impl(const intrusive_value_type_impl &);
   intrusive_value_type_impl& operator=(const intrusive_value_type_impl &);

   public:
   typedef CharType char_type;

   intrusive_value_type_impl(){}

   enum  {  BlockHdrAlignment = detail::alignment_of<block_header>::value  };

   block_header *get_block_header() const
   {
      return (block_header *)(detail::char_ptr_cast(this) +
         get_rounded_size(sizeof(*this), BlockHdrAlignment));
   }

   bool operator <(const intrusive_value_type_impl<Hook, CharType> & other) const
   {  return this->get_block_header()->template less<CharType>(*other.get_block_header());  }

   bool operator ==(const intrusive_value_type_impl<Hook, CharType> & other) const
   {  return this->get_block_header()->template equal<CharType>(*other.get_block_header());  }

   static intrusive_value_type_impl *get_intrusive_value_type(block_header *hdr)
   {
      return (intrusive_value_type_impl *)(detail::char_ptr_cast(hdr) -
         get_rounded_size(sizeof(intrusive_value_type_impl), BlockHdrAlignment));
   }

   CharType *name() const
   {  return get_block_header()->template name<CharType>(); }

   std::size_t name_length() const
   {  return get_block_header()->name_length(); }

   void *value() const
   {  return get_block_header()->value(); }
};

template<class CharType>
class char_ptr_holder
{
   public:
   char_ptr_holder(const CharType *name) 
      : m_name(name)
   {}

   char_ptr_holder(const detail::anonymous_instance_t *) 
      : m_name((CharType*)0)
   {}

   char_ptr_holder(const detail::unique_instance_t *) 
      : m_name((CharType*)-1)
   {}

   operator const CharType *()
   {  return m_name;  }

   private:
   const CharType *m_name;
};

//!The key of the the named allocation information index. Stores an offset pointer 
//!to a null terminated string and the length of the string to speed up sorting
template<class CharT, class VoidPointer>
struct index_key
{
   typedef typename detail::
      pointer_to_other<VoidPointer, const CharT>::type   const_char_ptr_t;
   typedef CharT                                         char_type;

   private:
   //Offset pointer to the object's name
   const_char_ptr_t  mp_str;
   //Length of the name buffer (null NOT included)
   std::size_t       m_len;
   public:

   //!Constructor of the key
   index_key (const char_type *name, std::size_t length)
      : mp_str(name), m_len(length) {}

   //!Less than function for index ordering
   bool operator < (const index_key & right) const
   {
      return (m_len < right.m_len) || 
               (m_len == right.m_len && 
               std::char_traits<char_type>::compare 
                  (detail::get_pointer(mp_str)
                  ,detail::get_pointer(right.mp_str), m_len) < 0);
   }

   //!Equal to function for index ordering
   bool operator == (const index_key & right) const
   {
      return   m_len == right.m_len && 
               std::char_traits<char_type>::compare 
                  (detail::get_pointer(mp_str),
                   detail::get_pointer(right.mp_str), m_len) == 0;
   }

   void name(const CharT *name)
   {  mp_str = name; }

   void name_length(std::size_t len)
   {  m_len = len; }

   const CharT *name() const
   {  return detail::get_pointer(mp_str); }

   std::size_t name_length() const
   {  return m_len; }
};

//!The index_data stores a pointer to a buffer and the element count needed
//!to know how many destructors must be called when calling destroy
template<class VoidPointer>
struct index_data
{
   typedef VoidPointer void_ptr;
   void_ptr    m_ptr;
   index_data(void *ptr) : m_ptr(ptr){}

   void *value() const
   {  return (void*)detail::get_pointer(m_ptr);  }
};

template<class MemoryAlgorithm>
struct basic_segment_manager_type
{  typedef basic_segment_manager<MemoryAlgorithm> type;   };

template<class CharT, class MemoryAlgorithm>
struct index_config
{
   typedef typename MemoryAlgorithm::void_pointer        void_pointer;
   typedef CharT                                         char_type;
   typedef detail::index_key<CharT, void_pointer>        key_type;
   typedef detail::index_data<void_pointer>              mapped_type;
   typedef typename basic_segment_manager_type
      <MemoryAlgorithm>::type                            basic_segment_manager;

   template<class HeaderBase>
   struct intrusive_value_type
   {  typedef detail::intrusive_value_type_impl<HeaderBase, CharT>  type; };

   typedef intrusive_compare_key<CharT>            intrusive_compare_key_type;
};

template<class Iterator, bool intrusive>
class iterator_value_adaptor
{
   typedef typename Iterator::value_type        iterator_val_t;
   typedef typename iterator_val_t::char_type   char_type;

   public:
   iterator_value_adaptor(const typename Iterator::value_type &val)
      :  m_val(&val)
   {}

   const char_type *name() const
   {  return m_val->name(); }

   std::size_t name_length() const
   {  return m_val->name_length(); }

   const void *value() const
   {  return m_val->value(); }

   const typename Iterator::value_type *m_val;
};


template<class Iterator>
class iterator_value_adaptor<Iterator, false>
{
   typedef typename Iterator::value_type        iterator_val_t;
   typedef typename iterator_val_t::first_type  first_type;
   typedef typename iterator_val_t::second_type second_type;
   typedef typename first_type::char_type       char_type;

   public:
   iterator_value_adaptor(const typename Iterator::value_type &val)
      :  m_val(&val)
   {}

   const char_type *name() const
   {  return m_val->first.name(); }

   std::size_t name_length() const
   {  return m_val->first.name_length(); }

   const void *value() const
   {
      return reinterpret_cast<block_header*>
         (detail::get_pointer(m_val->second.m_ptr))->value();
   }

   const typename Iterator::value_type *m_val;
};

template<class Iterator, bool intrusive>
struct iterator_transform
   :  std::unary_function< typename Iterator::value_type
                         , iterator_value_adaptor<Iterator, intrusive> >
{
   typedef iterator_value_adaptor<Iterator, intrusive> result_type;
   
   result_type operator()(const typename Iterator::value_type &arg) const
   {  return result_type(arg); }
};

}  //namespace detail{

/// @endcond

//These pointers are the ones the user will use to 
//indicate previous allocation types
static const detail::anonymous_instance_t   * anonymous_instance = 0;
static const detail::unique_instance_t      * unique_instance = 0;

//!This object is placed in the beginning of memory segment and
//!implements the allocation (named or anonymous) of portions
//!of the segment. This object contains two indexes that
//!maintain an association between a name and a portion of the segment. 
//!
//!The first index contains the mappings for normal named objects using the 
//!char type specified in the template parameter.
//!
//!The second index contains the association for unique instances. The key will
//!be the const char * returned from type_info.name() function for the unique
//!type to be constructed.
template<class CharType
        ,class MemoryAlgorithm
        ,template<class IndexConfig> class IndexType>
class segment_manager
   :  private detail::basic_segment_manager<MemoryAlgorithm>
{ 
   /// @cond
   //Non-copyable
   segment_manager();
   segment_manager(const segment_manager &);
   segment_manager &operator=(const segment_manager &);
   typedef detail::basic_segment_manager<MemoryAlgorithm> Base;
   typedef detail::block_header block_header_t;
   /// @endcond

   public:
   typedef typename Base::void_pointer    void_pointer;
   typedef CharType                       char_type;

   enum {   PayloadPerAllocation = Base::PayloadPerAllocation };

   /// @cond
   private:
   typedef detail::index_config<CharType, MemoryAlgorithm>  index_config_named;
   typedef detail::index_config<char, MemoryAlgorithm>      index_config_unique;
   typedef IndexType<index_config_named>                    index_type;
   typedef detail::bool_<is_intrusive_index<index_type>::value >    is_intrusive_t;
   typedef detail::bool_<is_node_index<index_type>::value>          is_node_index_t;

   public:
   typedef IndexType<index_config_named>                    named_index_t;
   typedef IndexType<index_config_unique>                   unique_index_t;
   typedef detail::char_ptr_holder<CharType>                char_ptr_holder_t;
   typedef detail::iterator_transform
      <typename named_index_t::const_iterator
      ,is_intrusive_index<index_type>::value>   named_transform;

   typedef detail::iterator_transform
      <typename unique_index_t::const_iterator
      ,is_intrusive_index<index_type>::value>   unique_transform;
   /// @endcond

   typedef typename Base::mutex_family       mutex_family;

   typedef transform_iterator
      <typename named_index_t::const_iterator, named_transform> const_named_iterator;
   typedef transform_iterator
      <typename unique_index_t::const_iterator, unique_transform> const_unique_iterator;

   //!Constructor proxy object definition helper class
   template<class T>
   struct construct_proxy
   {
      typedef detail::named_proxy<segment_manager, T, true>   type;
   };

   //!Constructor proxy object definition helper class
   template<class T>
   struct construct_iter_proxy
   {
      typedef detail::named_proxy<segment_manager, T, false>   type;
   };

   //!Constructor. Can throw
   segment_manager(std::size_t size)
      :  Base(size, priv_get_reserved_bytes())
      ,  m_header(static_cast<Base*>(get_this_pointer()))
   {  (void) anonymous_instance;   (void) unique_instance;   }

   //!Tries to find a previous named allocation address. Returns a memory
   //!buffer and the object count.
   template <class T>
   std::pair<T*, std::size_t> find  (const CharType* name)
   {  
      //The name can't be null, no anonymous object can be found by name
      assert(name != 0);
      detail::placement_destroy<T> table;
      std::size_t size;
      void *ret;

      if(name == reinterpret_cast<const CharType*>(-1)){
         ret = priv_generic_find<char> (typeid(T).name(), m_header.m_unique_index, table, size, is_intrusive_t());
      }
      else{
         ret = priv_generic_find<CharType> (name, m_header.m_named_index, table, size, is_intrusive_t());
      }
      return std::pair<T*, std::size_t>(static_cast<T*>(ret), size);
   }

   template <class T>
   std::pair<T*, std::size_t> find (const detail::unique_instance_t* name)
   {
      detail::placement_destroy<T> table;
      std::size_t size;
      void *ret = priv_generic_find<char>(name, m_header.m_unique_index, table, size, is_intrusive_t()); 
      return std::pair<T*, std::size_t>(static_cast<T*>(ret), size);
   }

   //!Returns throwing "construct" proxy
   //!object
   template <class T>
   typename construct_proxy<T>::type      
      construct(char_ptr_holder_t name)
   {  return typename construct_proxy<T>::type (this, name, false, true);  }

   //!Returns throwing "search or construct" proxy
   //!object
   template <class T>
   typename construct_proxy<T>::type find_or_construct(char_ptr_holder_t name)
   {  return typename construct_proxy<T>::type (this, name, true, true);  }

   //!Returns no throwing "construct" proxy
   //!object
   template <class T>
   typename construct_proxy<T>::type
      construct(char_ptr_holder_t name, std::nothrow_t)
   {  return typename construct_proxy<T>::type (this, name, false, false);  }

   //!Returns no throwing "search or construct"
   //!proxy object
   template <class T>
   typename construct_proxy<T>::type   
      find_or_construct(char_ptr_holder_t name, std::nothrow_t)
   {  return typename construct_proxy<T>::type (this, name, true, false);  }

   //!Returns throwing "construct from iterators" proxy object
   template <class T>
   typename construct_iter_proxy<T>::type     
      construct_it(char_ptr_holder_t name)
   {  return construct_iter_proxy<T>::type (this, name, false, true);  }

   //!Returns throwing "search or construct from iterators"
   //!proxy object
   template <class T>
   typename construct_iter_proxy<T>::type   
      find_or_construct_it(char_ptr_holder_t name)
   {  return typename construct_iter_proxy<T>::type (this, name, true, true);  }

   //!Returns no throwing "construct from iterators"
   //!proxy object
   template <class T>
   typename construct_iter_proxy<T>::type   
      construct_it(char_ptr_holder_t name, std::nothrow_t)
   {  return typename construct_iter_proxy<T>::type (this, name, false, false);  }

   //!Returns no throwing "search or construct from iterators"
   //!proxy object
   template <class T>
   typename construct_iter_proxy<T>::type 
      find_or_construct_it(char_ptr_holder_t name, std::nothrow_t)
   {  return typename construct_iter_proxy<T>::type (this, name, true, false);  }

   //!Calls object function blocking recursive interprocess_mutex and guarantees that 
   //!no new named_alloc or destroy will be executed by any process while 
   //!executing the object function call*/
   template <class Func>
   void atomic_func(Func &f)
   {  boost::interprocess::scoped_lock<rmutex> guard(m_header);  f();  }

   //!Calls the destructor and makes an unique
   //!deallocate
   template <class T>
   bool destroy(const detail::unique_instance_t *)
   {
      detail::placement_destroy<T> dtor;
      return this->priv_generic_named_destroy<char>
         (typeid(T).name(), m_header.m_unique_index, dtor, is_intrusive_t());
   }

   template <class T>
   bool destroy(const CharType *name)
   {
      detail::placement_destroy<T> dtor;
      return this->priv_generic_named_destroy<CharType>
               (name, m_header.m_named_index, dtor, is_intrusive_t());
   }

   template <class T>
   bool destroy_ptr(const T *p)
   {
      //If T is void transform it to char
      typedef typename detail::char_if_void<T>::type data_t;
      detail::placement_destroy<data_t> dtor;
      return priv_destroy_ptr(p, dtor);
   }

   //!Returns the name of an object created with construct/find_or_construct
   //!functions. Does not throw
   template<class T>
   const CharType *get_name(const T *ptr)
   {
      //Get header
      block_header_t *ctrl_data = block_header_t::block_header_from_value(ptr);
      CharType *name = static_cast<CharType*>(ctrl_data->template name<CharType>());
   
      //Sanity checks
      assert(ctrl_data->sizeof_char() == sizeof(CharType));
      assert(ctrl_data->m_num_char == std::char_traits<CharType>::length(name));

      return name;
   }

   //!Returns is the the name of an object created with construct/find_or_construct
   //!functions. Does not throw
   template<class T>
   detail::instance_type get_type(const T *ptr)
   {
      //Get header
      block_header_t *ctrl_data = block_header_t::block_header_from_value(ptr);
      assert((detail::instance_type)ctrl_data->allocation_type() < detail::max_allocation_type);
      return (detail::instance_type)ctrl_data->allocation_type();
   }

   //!Preallocates needed index resources to optimize the 
   //!creation of "num" named objects in the managed memory segment.
   //!Can throw boost::interprocess::bad_alloc if there is no enough memory.
   void reserve_named_objects(std::size_t num)
   {  
      //-------------------------------
      boost::interprocess::scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      m_header.m_named_index.reserve(num);  
   }

   //!Preallocates needed index resources to optimize the 
   //!creation of "num" unique objects in the managed memory segment.
   //!Can throw boost::interprocess::bad_alloc if there is no enough memory.
   void reserve_unique_objects(std::size_t num)
   {  
      //-------------------------------
      boost::interprocess::scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      m_header.m_unique_index.reserve(num);
   }

   //!Returns the number of named objects stored in
   //!the segment.
   std::size_t get_num_named_objects()
   {  
      //-------------------------------
      boost::interprocess::scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      return m_header.m_named_index.size();  
   }

   //!Returns the number of unique objects stored in
   //!the segment.
   std::size_t get_num_unique_objects()
   {  
      //-------------------------------
      boost::interprocess::scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      return m_header.m_unique_index.size();  
   }

   //!Returns the size of the memory
   //!segment
   std::size_t get_size() const
   {  return Base::get_size();  }

   //!Returns the number of free bytes of the memory
   //!segment
   std::size_t get_free_memory() const
   {  return Base::get_free_memory();  }

   //!Obtains the minimum size needed by the
   //!segment manager
   static std::size_t get_min_size()
   {  return Base::get_min_size(priv_get_reserved_bytes());  }

   //!Allocates nbytes bytes. Throws boost::interprocess::bad_alloc
   //!when it can allocate
   void * allocate(std::size_t nbytes)
   {  return Base::allocate(nbytes);   }

   //!Allocates nbytes bytes. This function is only used in 
   //!single-segment management. Never throws
   void * allocate (std::size_t nbytes, std::nothrow_t nothrow)
   {  return Base::allocate(nbytes, nothrow);   }

   //!Allocates aligned bytes, returns 0 if there is not more memory.
   //!Alignment must be power of 2
   void* allocate_aligned     (std::size_t nbytes, std::size_t alignment)
   {  return Base::allocate_aligned(nbytes, alignment);  }

   //!Allocates aligned bytes, throws bad_alloc when
   //!it can allocate. Alignment must be power of 2
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

   //!Deallocates the bytes allocated with allocate/allocate_at_least()
   //!pointed by addr
   void   deallocate          (void *addr)
   {  Base::deallocate(addr);   }

   //!Increases managed memory in extra_size bytes more. This only works
   //!with single-segment management
   void grow(std::size_t extra_size)
   {  Base::grow(extra_size);   }

   //!Returns the result of "all_memory_deallocated()" function
   //!of the used memory algorithm
   bool all_memory_deallocated()
   {   return Base::all_memory_deallocated(); }

   //!Returns the result of "check_sanity()" function
   //!of the used memory algorithm
   bool check_sanity()
   {   return Base::check_sanity(); }

   //!Writes to zero free memory (memory not yet allocated) of
   //!the memory algorithm
   void zero_free_memory()
   {   Base::zero_free_memory(); }

   //!Returns a constant iterator to the beginning of the information about
   //the named allocations performed in this segment manager
   const_named_iterator named_begin() const
   {
      return make_transform_iterator
         (m_header.m_named_index.begin(), named_transform());
   }

   //!Returns a constant iterator to the end of the information about
   //the named allocations performed in this segment manager
   const_named_iterator named_end() const
   {
      return make_transform_iterator
         (m_header.m_named_index.end(), named_transform());
   }

   //!Returns a constant iterator to the beginning of the information about
   //the unique allocations performed in this segment manager
   const_unique_iterator unique_begin() const
   {
      return make_transform_iterator
         (m_header.m_unique_index.begin(), unique_transform());
   }

   //!Returns a constant iterator to the end of the information about
   //the unique allocations performed in this segment manager
   const_unique_iterator unique_end() const
   {
      return make_transform_iterator
         (m_header.m_unique_index.end(), unique_transform());
   }

   /// @cond

   //!Generic named/anonymous new function. Offers all the possibilities, 
   //!such as throwing, search before creating, and the constructor is 
   //!encapsulated in an object function.
   template<class T>
   T *generic_construct(const CharType *name, 
                         std::size_t num, 
                         bool try2find, 
                         bool dothrow,
                         detail::in_place_interface &table)
   {
      return static_cast<T*>
         (priv_generic_construct(name, num, try2find, dothrow, table));
   }

   private:
   void *priv_generic_construct(const CharType *name, 
                         std::size_t num, 
                         bool try2find, 
                         bool dothrow,
                         detail::in_place_interface &table)
   {
      void *ret;
      //Security overflow check
      if(num > ((std::size_t)-1)/table.size){
         if(dothrow)
            throw bad_alloc();
         else
            return 0;
      }
      if(name == 0){
         ret = this->prot_anonymous_construct(num, dothrow, table);
      }
      else if(name == reinterpret_cast<const CharType*>(-1)){
         ret = this->priv_generic_named_construct<char>
            (detail::unique_type, table.type_name, num, try2find, dothrow, table, m_header.m_unique_index, is_intrusive_t());
      }
      else{
         ret = this->priv_generic_named_construct<CharType>
            (detail::named_type, name, num, try2find, dothrow, table, m_header.m_named_index, is_intrusive_t());
      }
      return ret;
   }

   bool priv_destroy_ptr(const void *ptr, detail::in_place_interface &dtor)
   {
      block_header_t *ctrl_data = block_header_t::block_header_from_value(ptr, dtor.size, dtor.alignment);
      switch(ctrl_data->allocation_type()){
         case detail::anonymous_type:
            return this->prot_anonymous_destroy(ptr, dtor);
         break;

         case detail::named_type:
            return this->priv_generic_named_destroy<CharType>
               (ctrl_data, m_header.m_named_index, dtor, is_node_index_t());
         break;

         case detail::unique_type:
            return this->priv_generic_named_destroy<char>
               (ctrl_data, m_header.m_unique_index, dtor, is_node_index_t());
         break;

         default:
            //This type is unknown, bad pointer passed to this function!
            assert(0);
            return false;
         break;
      }
      return false;
   }

   static std::size_t priv_get_reserved_bytes()
   {
      //Get the number of bytes until the end of (*this)
      //beginning in the end of the Base base.
      return   (detail::char_ptr_cast((segment_manager*)0) + sizeof(segment_manager))   -
               (detail::char_ptr_cast(static_cast<Base*>((segment_manager*)0)) + sizeof(Base));
   }

   template <class CharT>
   void *priv_generic_find
      (const CharT* name, 
       IndexType<detail::index_config<CharT, MemoryAlgorithm> > &index,
       detail::in_place_interface &table,
       std::size_t &length,
       detail::true_ is_intrusive)
   {
      (void)is_intrusive;
      typedef IndexType<detail::index_config<CharT, MemoryAlgorithm> >         index_type;
      typedef detail::index_key<CharT, void_pointer>  index_key_t;
      typedef typename index_type::iterator           index_it;

      //-------------------------------
      boost::interprocess::scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      //Find name in index
      detail::intrusive_compare_key<CharT> key
         (name, std::char_traits<CharT>::length(name));
      index_it it = index.find(key);

      //Initialize return values
      void *ret_ptr  = 0;
      length         = 0;

      //If found, assign values
      if(it != index.end()){
         //Get header
         block_header_t *ctrl_data = it->get_block_header();

         //Sanity check
         assert((ctrl_data->m_value_bytes % table.size) == 0);
         assert(ctrl_data->sizeof_char() == sizeof(CharT));
         ret_ptr  = ctrl_data->value();
         length  = ctrl_data->m_value_bytes/table.size;
      }
      return ret_ptr;
   }

   template <class CharT>
   void *priv_generic_find
      (const CharT* name, 
       IndexType<detail::index_config<CharT, MemoryAlgorithm> > &index,
       detail::in_place_interface &table,
       std::size_t &length,
       detail::false_ is_intrusive)
   {
      (void)is_intrusive;
      typedef IndexType<detail::index_config<CharT, MemoryAlgorithm> >      index_type;
      typedef typename index_type::key_type        key_type;
      typedef typename index_type::iterator        index_it;

      //-------------------------------
      boost::interprocess::scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      //Find name in index
      index_it it = index.find(key_type(name, std::char_traits<CharT>::length(name)));

      //Initialize return values
      void *ret_ptr  = 0;
      length         = 0;

      //If found, assign values
      if(it != index.end()){
         //Get header
         block_header_t *ctrl_data = reinterpret_cast<block_header_t*>
                                    (detail::get_pointer(it->second.m_ptr));

         //Sanity check
         assert((ctrl_data->m_value_bytes % table.size) == 0);
         assert(ctrl_data->sizeof_char() == sizeof(CharT));
         ret_ptr  = ctrl_data->value();
         length  = ctrl_data->m_value_bytes/table.size;
      }
      return ret_ptr;
   }

   template <class CharT>
   bool priv_generic_named_destroy
     (block_header_t *block_header,
      IndexType<detail::index_config<CharT, MemoryAlgorithm> > &index,
      detail::in_place_interface &table,
      detail::true_ is_node_index)
   {
      (void)is_node_index;
      typedef typename IndexType<detail::index_config<CharT, MemoryAlgorithm> >::iterator index_it;

      index_it *ihdr = block_header_t::to_first_header<index_it>(block_header);
      return this->priv_generic_named_destroy_impl<CharT>(*ihdr, index, table);
   }

   template <class CharT>
   bool priv_generic_named_destroy
     (block_header_t *block_header,
      IndexType<detail::index_config<CharT, MemoryAlgorithm> > &index,
      detail::in_place_interface &table,
      detail::false_ is_node_index)
   {
      (void)is_node_index;
      CharT *name = static_cast<CharT*>(block_header->template name<CharT>());
      return this->priv_generic_named_destroy<CharT>(name, index, table, is_intrusive_t());
   }

   template <class CharT>
   bool priv_generic_named_destroy(const CharT *name, 
                                   IndexType<detail::index_config<CharT, MemoryAlgorithm> > &index,
                                   detail::in_place_interface &table,
                                   detail::true_ is_intrusive_index)
   {
      (void)is_intrusive_index;
      typedef IndexType<detail::index_config<CharT, MemoryAlgorithm> >         index_type;
      typedef detail::index_key<CharT, void_pointer>  index_key_t;
      typedef typename index_type::iterator           index_it;
      typedef typename index_type::value_type         intrusive_value_type;
      
      //-------------------------------
      boost::interprocess::scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      //Find name in index
      detail::intrusive_compare_key<CharT> key
         (name, std::char_traits<CharT>::length(name));
      index_it it = index.find(key);

      //If not found, return false
      if(it == index.end()){
         //This name is not present in the index, wrong pointer or name!
         //assert(0);
         return false;
      }

      block_header_t *ctrl_data = it->get_block_header();
      intrusive_value_type *iv = intrusive_value_type::get_intrusive_value_type(ctrl_data);
      void *memory = iv;
      void *values = ctrl_data->value();
      std::size_t num = ctrl_data->m_value_bytes/table.size;
      
      //Sanity check
      assert((ctrl_data->m_value_bytes % table.size) == 0);
      assert(sizeof(CharT) == ctrl_data->sizeof_char());

      //Erase node from index
      index.erase(it);

      //Destroy the headers
      ctrl_data->~block_header_t();
      iv->~intrusive_value_type();

      //Call destructors and free memory
      std::size_t destroyed;
      table.destroy_n(values, num, destroyed);
      this->deallocate(memory);
      return true;
   }

   template <class CharT>
   bool priv_generic_named_destroy(const CharT *name, 
                                   IndexType<detail::index_config<CharT, MemoryAlgorithm> > &index,
                                   detail::in_place_interface &table,
                                   detail::false_ is_intrusive_index)
   {
      (void)is_intrusive_index;
      typedef IndexType<detail::index_config<CharT, MemoryAlgorithm> >            index_type;
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
      return this->priv_generic_named_destroy_impl<CharT>(it, index, table);
   }

   template <class CharT>
   bool priv_generic_named_destroy_impl
      (const typename IndexType<detail::index_config<CharT, MemoryAlgorithm> >::iterator &it,
      IndexType<detail::index_config<CharT, MemoryAlgorithm> > &index,
      detail::in_place_interface &table)
   {
      typedef IndexType<detail::index_config<CharT, MemoryAlgorithm> >      index_type;
      typedef typename index_type::iterator        index_it;

      //Get allocation parameters
      block_header_t *ctrl_data = reinterpret_cast<block_header_t*>
                                 (detail::get_pointer(it->second.m_ptr));
      char *stored_name       = detail::char_ptr_cast(it->first.name());
      (void)stored_name;

      //Check if the distance between the name pointer and the memory pointer 
      //is correct (this can detect incorrect type in destruction)
      std::size_t num = ctrl_data->m_value_bytes/table.size;
      void *values = ctrl_data->value();

      //Sanity check
      assert((ctrl_data->m_value_bytes % table.size) == 0);
      assert((void*)stored_name == ctrl_data->template name<CharT>());
      assert(sizeof(CharT) == ctrl_data->sizeof_char());

      //Erase node from index
      index.erase(it);

      //Destroy the header
      ctrl_data->~block_header_t();

      void *memory;
      if(is_node_index_t::value){
         index_it *ihdr = block_header_t::
            to_first_header<index_it>(ctrl_data);
         ihdr->~index_it();
         memory = ihdr;
      }
      else{
         memory = ctrl_data;
      }

      //Call destructors and free memory
      std::size_t destroyed;
      table.destroy_n(values, num, destroyed);
      this->deallocate(memory);
      return true;
   }

   template<class CharT>
   void * priv_generic_named_construct(std::size_t type,
                               const CharT *name,
                               std::size_t num, 
                               bool try2find, 
                               bool dothrow,
                               detail::in_place_interface &table,
                               IndexType<detail::index_config<CharT, MemoryAlgorithm> > &index,
                               detail::true_ is_intrusive)
   {
      (void)is_intrusive;
      std::size_t namelen  = std::char_traits<CharT>::length(name);

      block_header_t block_info ( table.size*num
                                 , table.alignment
                                 , type
                                 , sizeof(CharT)
                                 , namelen);

      typedef IndexType<detail::index_config<CharT, MemoryAlgorithm> >            index_type;
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

      typename index_type::insert_commit_data   commit_data;
      typedef typename index_type::value_type   intrusive_value_type;

      BOOST_TRY{
         detail::intrusive_compare_key<CharT> key(name, namelen);
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
            return it->get_block_header()->value();
         }
         return 0;
      }

      //Allocates buffer for name + data, this can throw (it hurts)
      void *buffer_ptr; 

      //Check if there is enough memory
      if(dothrow){
         buffer_ptr = this->allocate
            (block_info.total_size_with_header<intrusive_value_type>());
      }
      else{
         buffer_ptr = this->allocate
            (block_info.total_size_with_header<intrusive_value_type>(), std::nothrow_t());
         if(!buffer_ptr)
            return 0; 
      }

      //Now construct the intrusive hook plus the header
      intrusive_value_type * intrusive_hdr = new(buffer_ptr) intrusive_value_type();
      block_header_t * hdr = new(intrusive_hdr->get_block_header())block_header_t(block_info);
      void *ptr = hdr->value();

      //Copy name to memory segment and insert data
      CharT *name_ptr = static_cast<CharT *>(hdr->template name<CharT>());
      std::char_traits<CharT>::copy(name_ptr, name, namelen+1);

      BOOST_TRY{
         //Now commit the insertion using previous context data
         it = index.insert_commit(*intrusive_hdr, commit_data);
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
      //Build scoped ptr to avoid leaks with constructor exception
      detail::mem_algo_deallocator<MemoryAlgorithm> mem
         (buffer_ptr, this->memory_algorithm());

      //Construct array, this can throw
      detail::array_construct(ptr, num, table);

      //All constructors successful, we don't want to release memory
      mem.release();
      //Release node value_eraser since construction was successful
      value_eraser.release();
      return ptr;
   }

   //!Generic named new function for
   //!named functions
   template<class CharT>
   void * priv_generic_named_construct(std::size_t type,  
                               const CharT *name,
                               std::size_t num, 
                               bool try2find, 
                               bool dothrow,
                               detail::in_place_interface &table,
                               IndexType<detail::index_config<CharT, MemoryAlgorithm> > &index,
                               detail::false_ is_intrusive)
   {
      (void)is_intrusive;
      std::size_t namelen  = std::char_traits<CharT>::length(name);

      block_header_t block_info ( table.size*num
                                 , table.alignment
                                 , type
                                 , sizeof(CharT)
                                 , namelen);

      typedef IndexType<detail::index_config<CharT, MemoryAlgorithm> >            index_type;
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
            return hdr->value();
         }
         return 0;
      }
      //Initialize the node value_eraser to erase inserted node
      //if something goes wrong
      detail::value_eraser<index_type> value_eraser(index, it);

      //Allocates buffer for name + data, this can throw (it hurts)
      void *buffer_ptr; 
      block_header_t * hdr;

      //Allocate and construct the headers
      if(is_node_index_t::value){
         std::size_t total_size = block_info.total_size_with_header<index_it>();
         if(dothrow){
            buffer_ptr = this->allocate(total_size);
         }
         else{
            buffer_ptr = this->allocate(total_size, std::nothrow_t());
            if(!buffer_ptr)
               return 0; 
         }
         index_it *idr = new(buffer_ptr) index_it(it);
         hdr = block_header_t::from_first_header<index_it>(idr);
      }
      else{
         if(dothrow){
            buffer_ptr = this->allocate(block_info.total_size());
         }
         else{
            buffer_ptr = this->allocate(block_info.total_size(), std::nothrow_t());
            if(!buffer_ptr)
               return 0; 
         }
         hdr = static_cast<block_header_t*>(buffer_ptr);
      }

      hdr = new(hdr)block_header_t(block_info);
      void *ptr = hdr->value();

      //Copy name to memory segment and insert data
      CharT *name_ptr = static_cast<CharT *>(hdr->template name<CharT>());
      std::char_traits<CharT>::copy(name_ptr, name, namelen+1);

      //Do the ugly cast, please mama, forgive me!
      //This new key points to an identical string, so it must have the 
      //same position than the overwritten key according to the predicate
      const_cast<key_type &>(it->first).name(name_ptr);
      it->second.m_ptr  = hdr;

      //Build scoped ptr to avoid leaks with constructor exception
      detail::mem_algo_deallocator<MemoryAlgorithm> mem
         (buffer_ptr, this->memory_algorithm());

      //Construct array, this can throw
      detail::array_construct(ptr, num, table);

      //All constructors successful, we don't want to release memory
      mem.release();

      //Release node value_eraser since construction was successful
      value_eraser.release();
      return ptr;
   }

   private:
   //!Returns the this pointer
   segment_manager *get_this_pointer()
   {  return this;  }

   typedef typename MemoryAlgorithm::mutex_family::recursive_mutex_type   rmutex;

   //!This struct includes needed data and derives from
   //!rmutex to allow EBO when using null interprocess_mutex
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

#endif //#ifndef BOOST_INTERPROCESS_SEGMENT_MANAGER_HPP

