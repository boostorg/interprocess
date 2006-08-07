//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
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
#include <boost/interprocess/detail/multi_segment_services.hpp>
#include <boost/interprocess/offset_ptr.hpp>
#include <boost/interprocess/indexes/flat_map_index.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/smart_ptr/scoped_ptr.hpp>
#include <boost/aligned_storage.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include <stddef.h>

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

/*!An integer that describes the type of the instance constructed in memory*/
enum InstanceType {   anonymous_type = 0, named_type = 1, unique_type = 2   };

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

//These pointers are the ones the user will use to 
//indicate previous allocation types
static const detail::anonymous_instance_t   *anonymous_instance;
static const detail::unique_instance_t      *unique_instance;

}}   //namespace boost{  namespace interprocess{

namespace boost { namespace interprocess { namespace detail {

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
      detail::get_pointer(mp_str);
      return (m_len < right.m_len) || 
               (m_len == right.m_len && 
               std::char_traits<char_type>::compare 
                  (detail::get_pointer(mp_str),
                   detail::get_pointer(right.mp_str), m_len) < 0);
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


template<class T>
struct alloc_info_t
{
   public:
   std::size_t          m_sizeof_type;
   std::size_t          m_num;
   std::size_t          m_allocation_type;

   static std::size_t get_offset()
   {  return ct_rounded_size<sizeof(alloc_info_t<T>), sizeof(T)>::value;  }

   static T *get_data_from_info(const void *info)
   {  
      return reinterpret_cast<T*>
               (detail::char_ptr_cast(info) + get_offset());
   }

   static alloc_info_t *get_info_from_data(const void *data)
   {  
      return reinterpret_cast<alloc_info_t*>
               (detail::char_ptr_cast(data) - get_offset());
   }

   inline T* get_data() const
   {
      return reinterpret_cast<T*>
               (detail::char_ptr_cast(this) + get_offset());   
   }
};

template <class CharType, class It, bool NodeIndex>
struct alloc_name_t
{
   It       m_it;
   CharType m_name;

   static std::size_t get_name_offset()
   {
      //gcc does not like 0 to be used to this
      return char_ptr_cast(&((alloc_name_t*)sizeof(alloc_name_t))->m_name) -
             char_ptr_cast(&((alloc_name_t*)sizeof(alloc_name_t))->m_it);
   }
   CharType *get_name()
   {  return &m_name; }
   It &get_it()
   {  return m_it; }
};

template <class CharType, class It>
struct alloc_name_t<CharType, It, false> 
{
   CharType m_name;

   static std::size_t get_name_offset()
   {  return 0;   }
   CharType *get_name()
   {  return &m_name; }
   It &get_it()
   {  static It m_it;   return m_it; }
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
   static const OutPtr *get(const anonymous_instance_t *name) 
   {  return reinterpret_cast<const OutPtr *>(0);   }
};

template <class OutPtr>
struct get_construct_name<unique_instance_t, OutPtr>
{
   static const OutPtr *get(const unique_instance_t *name)
   {  return reinterpret_cast<const OutPtr *>(-1);   }
};

}  //namespace detail {

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
class segment_manager : private MemoryAlgorithm
{ 
   //Non-copyable
   segment_manager();
   segment_manager(const segment_manager &);
   segment_manager &operator=(const segment_manager &);

   public:
   typedef typename MemoryAlgorithm::void_pointer void_pointer;

   private:
   template<class CharT>
   struct index_config
   {
      typedef detail::index_key<CharT, void_pointer>  key_type;
      typedef detail::index_data<void_pointer>        mapped_type;
      typedef segment_manager
         <CharType, MemoryAlgorithm, IndexType>       segment_manager;
   };

   template <class CharT>
   struct index_traits
   {
      typedef typename segment_manager::
              template index_config<CharT>      index_config_t;
      typedef IndexType<index_config_t>         index_type;
      typedef typename index_type::iterator     index_it;
      typedef std::pair <index_it, bool>        index_ib;
      typedef typename index_type::key_type     key_type;
      typedef typename index_type::mapped_type  mapped_type;
      typedef typename index_type::value_type   value_type;
   };

   typedef  IndexType<index_config<CharType> >  named_index_t;
   typedef  IndexType<index_config<char> >      unique_index_t;

   struct raw_deleter_t
   {
      typedef void * pointer;

      raw_deleter_t(segment_manager &mngr)
         : m_mngr(mngr){}
      void operator()(pointer p)
         {  m_mngr.deallocate(p);      }
      private:
      segment_manager &m_mngr;
   };

 public:
   class char_ptr_holder_t
   {
     public:
      char_ptr_holder_t(const CharType *name) 
      : m_name(detail::get_construct_name<CharType, CharType>::get(name)){}

      char_ptr_holder_t(const detail::anonymous_instance_t *name) 
      : m_name(detail::get_construct_name<detail::anonymous_instance_t, CharType>::get(name)){}

      char_ptr_holder_t(const detail::unique_instance_t *name) 
      : m_name(detail::get_construct_name<detail::unique_instance_t, CharType>::get(name)){}

      operator const CharType *()
         {  return m_name;  }
     private:
      const CharType *m_name;
   };

   typedef typename MemoryAlgorithm::mutex_family        mutex_family;

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
   segment_manager(std::size_t size )
      :  MemoryAlgorithm(size, get_reserved_bytes()),
         m_header(get_this_pointer())
   {  (void) anonymous_instance;   (void) unique_instance;   }

   /*!Returns the size of the memory segment*/
   std::size_t get_size() const
   {  return MemoryAlgorithm::get_size();  }

   /*!Obtains the minimum size needed by the segment manager*/
   static std::size_t get_min_size ()
   {  return MemoryAlgorithm::get_min_size(sizeof(segment_manager)-sizeof(MemoryAlgorithm));  }

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
      construct(char_ptr_holder_t name, std::nothrow_t nothrow)
   {  return typename construct_proxy<T, false>::type (name, this);  }

   /*!Returns no throwing "search or construct" proxy object*/
   template <class T>
   typename find_construct_proxy<T, false>::type   
      find_or_construct(char_ptr_holder_t name, std::nothrow_t nothrow)
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
      construct_it(char_ptr_holder_t name, std::nothrow_t nothrow)
   {  return typename construct_iter_proxy<T, false>::type (name, this);  }

   /*!Returns no throwing "search or construct from iterators" proxy object*/
   template <class T>
   typename find_construct_iter_proxy<T, false>::type 
      find_or_construct_it(char_ptr_holder_t name, std::nothrow_t nothrow)
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

   /*!Allocates nbytes bytes. Throws boost::interprocess::bad_alloc when
      it can allocate*/
   void*  allocate(std::size_t nbytes)
   {  
      void * ret = this->allocate(nbytes, std::nothrow_t());
      if(!ret)
         throw bad_alloc();
      return ret;
   }

   /*!Allocates nbytes bytes. This function is only used in 
      single-segment management. Never throws*/
   void*  allocate (std::size_t nbytes, std::nothrow_t)
   {  return MemoryAlgorithm::allocate(nbytes);   }

   /*!It will try to allocate a buffer of preferred_size or more.
      It it can't do that, it will try to allocate at least
      min_size bytes. Throws boost::interprocess::bad_alloc when
      it can't allocate*/
/*
   std::pair<void *, bool> allocate_at_least
      (std::size_t min_size,
       std::size_t preferred_size, std::size_t &received_size, void *reuse = 0)
   {
      std::pair<void *, bool> ret = this->allocate_at_least
         (min_size, preferred_size, received_size, std::nothrow_t(), reuse);
      if(!ret.first)
         throw bad_alloc();
      return ret;
   }
*/
   std::pair<void *, bool>
      allocation_command  (allocation_type command,   std::size_t limit_size,
                           std::size_t preferred_size,std::size_t &received_size,
                           void *reuse_ptr = 0)
   {
      std::pair<void *, bool> ret = MemoryAlgorithm::allocation_command
         ( command | nothrow_allocation, limit_size, preferred_size, received_size
         , reuse_ptr);
      if(!(command & nothrow_allocation) && !ret.first)
         throw bad_alloc();
      return ret;
   }
/*
   std::pair<void *, bool>
      allocation_command  (allocation_type command,   std::size_t min_size,
                           std::size_t preferred_size,std::size_t &received_size,
                           std::nothrow_t, void *reuse_ptr = 0)
   {
      return MemoryAlgorithm::allocation_command
         (command, min_size, preferred_size, received_size, reuse_ptr);   
   }
*/

   /*!It will try to allocate a buffer of preferred_size or more.
      It it can't do that, it will try to allocate at least
      min_size bytes. Return 0 when it can't allocate*/
/*
   std::pair<void *, bool> allocate_at_least(std::size_t min_size, std::size_t preferred_size,
                           std::size_t &received_size, std::nothrow_t, void *reuse = 0)
   {  return MemoryAlgorithm::allocate_at_least(min_size, preferred_size, received_size, reuse);   }
*/
   /*!Returns true if the buffer pointed by ptr can be expanded
      to preferred_size or more. If not it will try to expand it
      at least min_size. Otherwise, returns false*/
/*
   bool expand(void *ptr,
               std::size_t min_size, std::size_t preferred_size,
               std::size_t &received_size)
   {  return MemoryAlgorithm::expand(ptr, min_size, preferred_size, received_size);   }
*/
   /*!Returns true if the buffer pointed by ptr can be shrunk
      to preferred_size. If not it will try to shrink it
      at least between max_size and preferred_size. Otherwise, returns false*/
/*
   bool shrink(void *ptr,
               std::size_t max_size, std::size_t preferred_size,
               std::size_t &received_size)
   {  return MemoryAlgorithm::shrink(ptr, max_size, preferred_size, received_size);   }
*/
   /*!Deallocates the bytes allocated with allocate/allocate_at_least()
      pointed by addr*/
   void   deallocate          (void *addr)
   {  MemoryAlgorithm::deallocate(addr);   }

   /*!Increases managed memory in extra_size bytes more. This only works
      with single-segment management*/
   void grow(std::size_t extra_size)
   {  MemoryAlgorithm::grow(extra_size);   }

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
      typedef detail::alloc_info_t<T>  ctrl_data_t;
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
   const char *get_name(const T *ptr)
   {
      typedef typename index_traits<CharType>::index_type   index_t;
      typedef typename index_traits<CharType>::key_type     key_type;
      typedef typename index_traits<CharType>::index_it     index_it;
      typedef detail::alloc_info_t<T>                       ctrl_data_t;
      const bool NodeIndex = is_node_index<index_t>::value;   //change this
      typedef detail::alloc_name_t
         <CharType, index_it, NodeIndex>                    alloc_name_t;
      
      ctrl_data_t * ctrl_data = ctrl_data_t::get_info_from_data(ptr);

      //Get total size of data port and total allocation size
      std::size_t datasize  = detail::get_rounded_size
               (ctrl_data_t::get_offset() + sizeof(T)*ctrl_data->m_num,
                                  boost::alignment_of<alloc_name_t>::value);
      alloc_name_t *alloc_name = reinterpret_cast<alloc_name_t*>
                                    (char_ptr_cast(ctrl_data)+datasize);
      return alloc_name->get_name();
   }

   /*!Returns is the the name of an object created with construct/find_or_construct
      functions. Does not throw*/
   template<class T>
   InstanceType get_type(const T *ptr)
   {
      typedef detail::alloc_info_t<T>  ctrl_data_t;
            ctrl_data_t * ctrl_data = ctrl_data_t::get_info_from_data(ptr);
      return static_cast<InstanceType>(ctrl_data->m_allocation_type);
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

   /*!Returns the result of "all_memory_deallocated()" function
      of the used memory algorithm*/
   bool all_memory_deallocated()
   {   return MemoryAlgorithm::all_memory_deallocated(); }

   /*!Returns the result of "check_sanity()" function
      of the used memory algorithm*/
   bool check_sanity()
   {   return MemoryAlgorithm::check_sanity(); }

   private:

   template <class T>
   bool priv_destroy_ptr(const T *ptr)
   {
      typedef detail::alloc_info_t<T>  ctrl_data_t;
      
      ctrl_data_t * ctrl_data = ctrl_data_t::get_info_from_data(ptr);
      
      switch(ctrl_data->m_allocation_type){
         case anonymous_type:
            return this->priv_generic_anonymous_destroy<T>(ptr);
         break;

         case named_type:
            return this->priv_generic_named_destroy<T, CharType>
               (ctrl_data, m_header.m_named_index);
         break;

         case unique_type:
            return this->priv_generic_named_destroy<T, char>
               (ctrl_data, m_header.m_unique_index);
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
       typename index_traits<CharT>::index_type &index)
   {
      typedef typename index_traits<CharT>::key_type  key_type;
      typedef typename index_traits<CharT>::index_it  index_it;
      typedef detail::alloc_info_t<T>  ctrl_data_t;

      //-------------------------------
      boost::interprocess::scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      //Find name in index
      index_it it = index.find(key_type(name, std::char_traits<CharT>::length(name)));

      //Initialize returned values
      T *ret_ptr           = 0;
      std::size_t ret_num  = 0;

      //If found, assign values
      if(it != index.end()){
         ctrl_data_t *ctrl_data = reinterpret_cast<ctrl_data_t*>
                                    (detail::get_pointer(it->second.m_ptr));
         //Check if sizeof is correct!
         if(sizeof(T) != ctrl_data->m_sizeof_type){
            //The template parameter T is not correct!
            assert(0);
            ret_ptr  = 0;
            ret_num  = 0;
         }
         else{
            ret_ptr  = ctrl_data->get_data();
            ret_num  = ctrl_data->m_num;
         }
      }
      return std::make_pair(ret_ptr, ret_num);
   }

   /*!Calls the destructor and makes an anonymous deallocate*/
   template <class T>
   bool priv_generic_anonymous_destroy(const void *object)
   {
      typedef detail::alloc_info_t<T> ctrl_data_t;
      //-------------------------------
      boost::interprocess::scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      if(!object)
         return false;

      //Get control data from associated with this object    
      ctrl_data_t * ctrl_data = ctrl_data_t::get_info_from_data(object);
      if(ctrl_data->m_allocation_type != anonymous_type){
         //This is not an anonymous object, the pointer is wrong!
         assert(0);
         return false;
      }
   
      if(ctrl_data->m_sizeof_type != sizeof(T)){
         //Wrong template parameter T in deallocation
         assert(0);
         return false;
      }

      //Call destructors
      if(!boost::has_trivial_destructor<T>::value){
         this->priv_array_destroy
            (ctrl_data, ctrl_data_t::get_data_from_info(ctrl_data), ctrl_data->m_num);
      }

      return true;
   }
   template <class T, class CharT>
   bool priv_generic_named_destroy(const CharT *name, 
                                   typename index_traits<CharT>::index_type &index)
   {
      typedef typename index_traits<CharT>::key_type  key_type;
      typedef typename index_traits<CharT>::index_it  index_it;
      typedef detail::alloc_info_t<T>                 ctrl_data_t; 
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
   bool priv_generic_named_destroy(detail::alloc_info_t<T> *data, 
                                   typename index_traits<CharT>::index_type &index)
   {
      typedef typename index_traits<CharT>::index_type   index_t;
      typedef typename index_traits<CharT>::key_type     key_type;
      typedef typename index_traits<CharT>::index_it     index_it;
      typedef detail::alloc_info_t<T>                    ctrl_data_t;
      //-------------------------------
      boost::interprocess::scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      const bool NodeIndex = is_node_index<index_t>::value;   //change this
      typedef detail::alloc_name_t<CharT, index_it, NodeIndex>  alloc_name_t;

      //Get total size of data port and total allocation size
      std::size_t datasize  = detail::get_rounded_size
                                 (ctrl_data_t::get_offset() + sizeof(T)*data->m_num,
                                  boost::alignment_of<alloc_name_t>::value);
      alloc_name_t *alloc_name = reinterpret_cast<alloc_name_t*>
                                    (char_ptr_cast(data)+datasize);
      if(NodeIndex){
         //If we are using node indexes, the iterator to the index
         //entry is stored there, so we can erase faster
         return this->priv_generic_named_destroy_impl<T, CharT>
                  (alloc_name->get_it(), index);
      }
      else{
         //If not using node indexes, we must use the name to find
         return this->priv_generic_named_destroy<T, CharT>
                  (alloc_name->get_name(), index);
      }
   }

   template <class T, class CharT>
   bool priv_generic_named_destroy(const typename index_traits<CharT>::index_it &it,
                                   typename index_traits<CharT>::index_type &index)
   {
      //-------------------------------
      boost::interprocess::scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      return this->priv_generic_named_destroy_impl<T, CharT>(it, index);
   }

   template <class T, class CharT>
   bool priv_generic_named_destroy_impl(const typename index_traits<CharT>::index_it &it,
                                        typename index_traits<CharT>::index_type &index)
   {
      typedef typename index_traits<CharT>::index_type   index_t;
      typedef typename index_traits<CharT>::key_type     key_type;
      typedef typename index_traits<CharT>::index_it     index_it;
      typedef detail::alloc_info_t<T>                    ctrl_data_t; 

      //Get allocation parameters
      void *memory            = detail::get_pointer(it->second.m_ptr);
      char *stored_name       = detail::char_ptr_cast
                                 (detail::get_pointer(it->first.mp_str));
      //Check if the distance between the name pointer and the memory pointer 
      //is correct (this can detect incorrect T type in destruction)
      ctrl_data_t *ctrl_data  = reinterpret_cast<ctrl_data_t *>(memory);
      std::size_t num         = ctrl_data->m_num;

      //Check if the sizeof(T) is equal to the size of the created type
      if(sizeof(T) != ctrl_data->m_sizeof_type){
         //Wrong template T parameter in deallocation!
         assert(0);
         return false;
      }

      const bool NodeIndex = is_node_index<index_t>::value;   //change this
      typedef detail::alloc_name_t<CharT, index_it, NodeIndex>  alloc_name_t;

      //Get total size of data port and total allocation size
      std::size_t datasize  = detail::get_rounded_size
                                 (ctrl_data_t::get_offset() + sizeof(T)*num, 
                                  boost::alignment_of<alloc_name_t>::value);

      //Check if the distance between the name pointer and the memory pointer 
      //is correct (this can detect incorrect T type in destruction)
      if(std::size_t(stored_name - detail::char_ptr_cast(memory)) != 
         (datasize + alloc_name_t::get_name_offset())){
         assert(0);
         //Something ugly here, name pointer is not in the right position!
         return false;
      }

      //If this is a node index, destroy the iterator (usually iterators are
      //POD , but you never know)
      if(NodeIndex){
         alloc_name_t *alloc_name = reinterpret_cast<alloc_name_t*>
                                       (detail::char_ptr_cast(memory)+datasize);
         alloc_name->get_it().~index_it();
      }
      //Erase node from index
      index.erase(it);

      //Call destructors
      if(!boost::has_trivial_destructor<T>::value){
         priv_array_destroy(memory, ctrl_data_t::get_data_from_info(memory), num);
      }

      return true;
   }

   template <class T>
   void priv_array_destroy(const void * memory, T *objects, std::size_t count)
   {
      //Build scoped ptr to avoid leaks with destructor exception
      boost::interprocess::scoped_ptr<void, raw_deleter_t> mem((void*)memory, *this);

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
                               typename index_traits<CharT>::index_type &index)
   {
      typedef typename index_traits<CharT>::index_type   index_t;
      typedef typename index_traits<CharT>::key_type     key_type;
      typedef typename index_traits<CharT>::mapped_type  mapped_type;
      typedef typename index_traits<CharT>::value_type   value_type;
      typedef typename index_traits<CharT>::index_it     index_it;
      typedef typename index_traits<CharT>::index_type   index_type;
      typedef typename index_traits<CharT>::index_ib     index_ib;
      typedef detail::alloc_info_t<T>                    ctrl_data_t;

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
      std::size_t namelen  = std::char_traits<CharT>::length(name);      
      index_ib insert_ret = index.insert(value_type(key_type (name, namelen), 
                                                    mapped_type(0)));
      index_it it = insert_ret.first;

      //If found and this is find or construct, return data
      //else return null
      if(!insert_ret.second){
         return try2find ? 
                  ctrl_data_t::get_data_from_info
                     (detail::get_pointer(it->second.m_ptr)): 
                  0;
      }
      //Initialize the node value_eraser to erase inserted node
      //if something goes wrong
      detail::value_eraser<index_type> value_eraser(index, insert_ret.first);

      const bool NodeIndex = is_node_index<index_t>::value;   //change this
      typedef detail::alloc_name_t<CharT, index_it, NodeIndex>  alloc_name_t;

      //Get total size of data port and total allocation size
      std::size_t datasize  = detail::get_rounded_size
                                 (ctrl_data_t::get_offset() + sizeof(T)*num, 
                                  boost::alignment_of<alloc_name_t>::value);
      std::size_t allocsize =  datasize + alloc_name_t::get_name_offset() + (namelen+1)*sizeof(CharT);

      //Allocates buffer for name + data, this can throw (it hurts)
      void *buffer_ptr; 

      //Check if there is enough memory
      if(dothrow){
         buffer_ptr = this->allocate(allocsize);
      }
      else{
         buffer_ptr = this->allocate(allocsize, std::nothrow_t());
         if(!buffer_ptr)   return 0; 
      }

      //Set pointer and control data
      insert_ret.first->second.m_ptr   = buffer_ptr;
      ctrl_data_t *ctrl_data           = static_cast<ctrl_data_t*>(buffer_ptr);
      ctrl_data->m_allocation_type     = type;
      ctrl_data->m_num                 = num;
      ctrl_data->m_sizeof_type         = sizeof(T);
      alloc_name_t *alloc_name         = reinterpret_cast<alloc_name_t *>
                                          (char_ptr_cast(ctrl_data) + datasize);

      //Copy name to memory segment and insert data
      CharT *name_ptr    = alloc_name->get_name();
      std::char_traits<CharT>::copy(name_ptr, name, namelen+1);

      //If this is a node container, store also the iterator
      if(NodeIndex){
         new(&alloc_name->get_it()) index_it(insert_ret.first);
      }

      //Do the ugly cast, please mama, forgive me!
      //This new key points to an identical string, so it must have the 
      //same position than the overwritten key according to the predicate
      const_cast<key_type &>(insert_ret.first->first).mp_str = name_ptr;

      //Avoid constructions if constructor is trivial
      if(!CtorFunc::is_trivial){
         //Build scoped ptr to avoid leaks with constructor exception
         boost::interprocess::scoped_ptr<void, raw_deleter_t> mem(buffer_ptr, *this);

         //Construct array, this can throw
         this->priv_array_construct<CtorFunc>
            (ctrl_data_t::get_data_from_info(buffer_ptr), num, ctor);

         //All constructors successful, we don't want to release memory
         mem.release();
      }
      //Release node value_eraser since construction was successful
      value_eraser.release();
      return ctrl_data_t::get_data_from_info(buffer_ptr);
   }

   /*!Generic anonymous "new" function.*/
   template<class T, class CtorFunc>
   T * priv_generic_anonymous_construct(std::size_t num, 
                                        bool dothrow,
                                        CtorFunc &ctor)
   {
      typedef detail::alloc_info_t<T> ctrl_data_t;
      //-------------------------------
      boost::interprocess::scoped_lock<rmutex> guard(m_header);
      //-------------------------------
      std::size_t allocsize = ctrl_data_t::get_offset() + sizeof(T)*num;      

      //Allocate memory
      void *ptr_struct = this->allocate(allocsize, std::nothrow_t());

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
      T* ptr = ctrl_data_t::get_data_from_info(ptr_struct);
      ctrl_data_t *ctrl_data = reinterpret_cast<ctrl_data_t*>(ptr_struct);

      //Set control data
      ctrl_data->m_num               = num;
      ctrl_data->m_allocation_type   = anonymous_type;
      ctrl_data->m_sizeof_type       = sizeof(T);

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

   std::size_t get_reserved_bytes()
   {
      //Get the number of bytes until the end of (*this)
      //beginning in the end of the MemoryAlgorithm base.
      return   (detail::char_ptr_cast(this) + sizeof(*this))   -
               (detail::char_ptr_cast(static_cast<MemoryAlgorithm*>(this)) +
                  sizeof(MemoryAlgorithm));
   }

   typedef typename MemoryAlgorithm::mutex_family::recursive_mutex_type   rmutex;

   /*!This struct includes needed data and derives from
      rmutex to allow EBO when using null interprocess_mutex*/
   struct header_t : public rmutex
   {
      named_index_t           m_named_index;
      unique_index_t          m_unique_index;
   
      header_t(segment_manager *segment_mngr)
      :  m_named_index (segment_mngr),
         m_unique_index(segment_mngr)
         {}
   }  m_header;
};


}} //namespace boost { namespace interprocess

#include <boost/interprocess/detail/config_end.hpp>

#endif //#ifndef BOOST_INTERPROCESS_NAMED_OBJECT_ALGO_HPP

