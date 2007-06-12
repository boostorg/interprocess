//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2007.
// (C) Copyright Gennaro Prota 2003 - 2004.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_UTILITIES_HPP
#define BOOST_INTERPROCESS_DETAIL_UTILITIES_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/detail/move.hpp>
#include <boost/type_traits/has_trivial_destructor.hpp>
#include <boost/interprocess/detail/min_max.hpp>
//#include <functional>
#include <utility>
#include <stdexcept>

namespace boost {
namespace interprocess { 
namespace detail {

template<class SmartPtr>
struct smart_ptr_type
{
   typedef typename SmartPtr::value_type value_type;
   typedef value_type *pointer;
   static pointer get (const SmartPtr &smartptr)
   {  return smartptr.get();}
};

template<class T>
struct smart_ptr_type<T*>
{
   typedef T value_type;
   typedef value_type *pointer;
   static pointer get (pointer ptr)
   {  return ptr;}
};

//!Overload for smart pointers to avoid ADL problems with get_pointer
template<class Ptr>
inline typename smart_ptr_type<Ptr>::pointer
get_pointer(const Ptr &ptr)
{  return smart_ptr_type<Ptr>::get(ptr);   }

//!To avoid ADL problems with swap
template <class T>
inline void do_swap(T& x, T& y)
{
   using std::swap;
   swap(x, y);
}

//!A deleter for scoped_ptr that deallocates the memory
//!allocated for an object using a STL allocator.
template <class Allocator>
struct scoped_ptr_deallocator
{
   typedef typename Allocator::pointer pointer;

   Allocator& m_alloc;

   scoped_ptr_deallocator(Allocator& a)
         : m_alloc(a) {}

   void operator()(pointer ptr)
      {  if (ptr) m_alloc.deallocate(ptr, 1);  }
};

//!A deleter for scoped_ptr that deallocates the memory
//!allocated for an array of objects using a STL allocator.
template <class Allocator>
struct scoped_ptr_array_deallocator
{
   typedef typename Allocator::pointer pointer;

   scoped_ptr_array_deallocator(Allocator& a, std::size_t length)
         : m_alloc(a), m_length(length) {}

   void operator()(pointer &ptr)
      {  m_alloc.deallocate(ptr, m_length);  }

   private:
   Allocator&  m_alloc;
   std::size_t m_length;
};

//!A deleter for scoped_ptr that deallocates the memory
//!allocated for an object using a STL allocator.
template <class Allocator>
struct scoped_deallocator
{
   typedef typename Allocator::pointer pointer;

   pointer     m_ptr;
   Allocator&  m_alloc;

   scoped_deallocator(pointer p, Allocator& a)
      : m_ptr(p), m_alloc(a) {}

   ~scoped_deallocator()
   {  if (m_ptr) m_alloc.deallocate(m_ptr, 1);  }

   void release()
   {  m_ptr = 0; }
};

//!A deleter for scoped_ptr that deallocates the memory
//!allocated for an array of objects using a STL allocator.
template <class Allocator>
struct scoped_array_deallocator
{
   typedef typename Allocator::pointer pointer;

   scoped_array_deallocator(pointer p, Allocator& a, std::size_t length)
      : m_ptr(p), m_alloc(a), m_length(length) {}

   ~scoped_array_deallocator()
   {  if (m_ptr) m_alloc.deallocate(m_ptr, m_length);  }

   void release()
   {  m_ptr = 0; }

   private:
   pointer     m_ptr;
   Allocator&  m_alloc;
   std::size_t m_length;
};

//!A deleter for scoped_ptr that destroys
//!an object using a STL allocator.
template <class Pointer>
struct scoped_destructor_n
{
   typedef Pointer pointer;

   pointer     m_p;
   std::size_t m_n;

   scoped_destructor_n(pointer p, std::size_t n)
         : m_p(p), m_n(n){}

   void release()
   {  m_p = 0; }
   
   ~scoped_destructor_n()
   {
      if(!m_p) return;
      typedef typename std::iterator_traits<Pointer>::value_type value_type;
      value_type *raw_ptr = detail::get_pointer(m_p);
      for(std::size_t i = 0; i < m_n; ++i, ++raw_ptr)
         raw_ptr->~value_type();
   }
};

template <class A>
class allocator_destroyer
{
   typedef typename A::value_type value_type;
   private:
   A & a_;

   public:
   allocator_destroyer(A &a)
      :  a_(a)
   {}

   void operator()(const typename A::pointer &p)
   {  
      detail::get_pointer(p)->~value_type();
      a_.deallocate(p, 1);
   }
};

/*!Forces a cast from any pointer to char * pointer*/
template<class T>
inline char* char_ptr_cast(T *ptr)
{
   //This is nasty, but we like it a lot!
   return (char*)(ptr);
}

inline char* char_ptr_cast()
{
   //This is nasty, but we like it a lot!
   return (char*)(0);
}

//Rounds "orig_size" by excess to round_to bytes
inline std::size_t get_rounded_size(std::size_t orig_size, std::size_t round_to)
{
   return ((orig_size-1)/round_to+1)*round_to;
}

inline std::size_t get_truncated_size(std::size_t orig_size, std::size_t multiple)
{
   return orig_size/multiple*multiple;
}
 
template <std::size_t OrigSize, std::size_t RoundTo>
struct ct_rounded_size
{
   enum { value = ((OrigSize-1)/RoundTo+1)*RoundTo };
};

template <std::size_t Value1, std::size_t Value2>
struct ct_min
{
   enum { value = (Value1 < Value2)? Value1 : Value2 };
};

template <std::size_t Value1, std::size_t Value2>
struct ct_max
{
   enum { value = (Value1 > Value2)? Value1 : Value2 };
};

// Gennaro Prota wrote this. Thanks!
template <int p, int n = 4>
struct ct_max_pow2_less
{
   enum { c = 2*n < p };

   static const std::size_t value =
         c ? (ct_max_pow2_less< c*p, 2*c*n>::value) : n;
};

template <>
struct ct_max_pow2_less<0, 0>
{
   static const std::size_t value = 0;
};

/*!Obtains a generic pointer of the same type that
   can point to other pointed type: Ptr<?> -> Ptr<NewValueType>*/
template<class T, class U>
struct pointer_to_other;

template<class T, class U, 
         template<class> class Sp>
struct pointer_to_other< Sp<T>, U >
{
   typedef Sp<U> type;
};

template<class T, class T2, class U, 
         template<class, class> class Sp>
struct pointer_to_other< Sp<T, T2>, U >
{
   typedef Sp<U, T2> type;
};

template<class T, class T2, class T3, class U, 
         template<class, class, class> class Sp>
struct pointer_to_other< Sp<T, T2, T3>, U >
{
   typedef Sp<U, T2, T3> type;
};

template<class T, class U>
struct pointer_to_other< T*, U >
{
   typedef U* type;
};

}  //namespace detail {

/*!Trait class to detect if an index is a node
   index. This allows more efficient operations
   when deallocating named objects.*/
template <class Index>
struct is_node_index
{
   enum {   value = false };
};


/*!Trait class to detect if an index is an intrusive
   index. This will embed the derivation hook in each
   allocation header, to provide memory for the intrusive
   container.*/
template <class Index>
struct is_intrusive_index
{
   enum {   value = false };
};

/*!Trait class to detect if an smart pointer has 
   multi-segment addressing capabilities.*/
template <class Ptr>
struct is_multisegment_ptr
{
   enum {   value = false };
};

/*!A Interprocess shared pointer deleter that uses the segment manager's 
   destroy_ptr function to destroy the shared resource.*/
template<class T, class SegmentManager>
class deleter
{
   public:
   typedef typename detail::pointer_to_other
      <typename SegmentManager::void_pointer, T>::type   pointer;

   private:
   typedef typename detail::pointer_to_other
      <pointer, SegmentManager>::type   segment_manager_pointer;

   segment_manager_pointer mp_deleter;

   public:
   deleter(const segment_manager_pointer &pdeleter)
      :  mp_deleter(pdeleter)
   {}

   void operator()(const pointer &p)
   {  mp_deleter->destroy_ptr(detail::get_pointer(p));   }
};

template <class SizeType>
SizeType
   get_next_capacity(const SizeType max_size
                    ,const SizeType capacity
                    ,const SizeType n)
{
//   if (n > max_size - capacity)
//      throw std::length_error("get_next_capacity");

   const SizeType m3 = max_size/3;

   if (capacity < m3)
      return capacity + max_value(3*(capacity+1)/5, n);

   if (capacity < m3*2)
      return capacity + max_value((capacity+1)/2, n);

   return max_size;
}

namespace detail {

template <class T1, class T2>
struct pair
{
   typedef T1 first_type;
   typedef T2 second_type;

   T1 first;
   T2 second;

   pair()
      : first(), second()
   {}

   pair(const T1& x, const T2& y)
      : first(x), second(y)
   {}

   template <class D, class S>
   pair(const std::pair<D, S>& p)
      : first(p.first), second(p.second)
   {}

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   template <class D, class S>
   pair(const detail::moved_object<std::pair<D, S> >& p)
      : first(move(p.get().first)), second(move(p.get().second))
   {}
   #else
   template <class D, class S>
   pair(std::pair<D, S> && p)
      : first(move(p.first)), second(move(p.second))
   {}
   #endif

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   template <class D, class S>
   pair(const detail::moved_object<pair<D, S> >& p)
      : first(move(p.get().first)), second(move(p.get().second))
   {}
   #else
   template <class D, class S>
   pair(pair<D, S> && p)
      : first(move(p.first)), second(move(p.second))
   {}
   #endif

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   template <class U, class V>
   pair(const detail::moved_object<U> &x, const detail::moved_object<V> &y)
      : first(move(x.get())), second(move(y.get()))
   {}
   #else
   template <class U, class V>
   pair(U &&x, V &&y)
      : first(move(x)), second(move(y))
   {}
   #endif

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   pair(const detail::moved_object<pair> &p)
      : first(move(p.get().first)), second(move(p.get().second))
   {}
   #else
   pair(pair &&p)
      : first(move(p.first)), second(move(p.second))
   {}
   #endif

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   pair& operator=(const detail::moved_object<pair> &p)
   {
      first  = move(p.get().first);
      second = move(p.get().second);
      return *this;
   }
   #else
   pair& operator=(pair &&p)
   {
      first  = move(p.first);
      second = move(p.second);
      return *this;
   }
   #endif

   pair& operator=(const pair &p)
   {
      first  = p.first;
      second = p.second;
      return *this;
   }

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   template <class D, class S>
   pair& operator=(const detail::moved_object<std::pair<D, S> > &p)
   {
      first  = move(p.get().first);
      second = move(p.get().second);
      return *this;
   }
   #else
   template <class D, class S>
   pair& operator=(std::pair<D, S> &&p)
   {
      first  = move(p.first);
      second = move(p.second);
      return *this;
   }
   #endif

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   void swap(const detail::moved_object<pair> &p)
   {  std::swap(*this, p.get()); }

   void swap(pair& p)
   {  std::swap(*this, p); }

   #else
   void swap(pair &&p)
   {  std::swap(*this, p); }
   #endif
};

template <class T1, class T2>
inline bool operator==(const pair<T1,T2>& x, const pair<T1,T2>& y)
{  return static_cast<bool>(x.first == y.first && x.second == y.second);  }

template <class T1, class T2>
inline bool operator< (const pair<T1,T2>& x, const pair<T1,T2>& y)
{  return static_cast<bool>(x.first < y.first ||
                         (!(y.first < x.first) && x.second < y.second)); }

template <class T1, class T2>
inline bool operator!=(const pair<T1,T2>& x, const pair<T1,T2>& y)
{  return static_cast<bool>(!(x == y));  }

template <class T1, class T2>
inline bool operator> (const pair<T1,T2>& x, const pair<T1,T2>& y)
{  return y < x;  }

template <class T1, class T2>
inline bool operator>=(const pair<T1,T2>& x, const pair<T1,T2>& y)
{  return static_cast<bool>(!(x < y)); }

template <class T1, class T2>
inline bool operator<=(const pair<T1,T2>& x, const pair<T1,T2>& y)
{  return static_cast<bool>(!(y < x)); }

template <class T1, class T2>
inline pair<T1, T2> make_pair(T1 x, T2 y)
{  return pair<T1, T2>(x, y); }

#ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
template <class T1, class T2>
inline void swap(const detail::moved_object<pair<T1, T2> > &x, pair<T1, T2>& y)
{
   swap(x.get().first, y.first);
   swap(x.get().second, y.second);
}

template <class T1, class T2>
inline void swap(pair<T1, T2>& x, const detail::moved_object<pair<T1, T2> > &y)
{
   swap(x.first, y.get().first);
   swap(x.second, y.get().second);
}

template <class T1, class T2>
inline void swap(pair<T1, T2>& x, pair<T1, T2>& y)
{
   swap(x.first, y.first);
   swap(x.second, y.second);
}

#else
template <class T1, class T2>
inline void swap(pair<T1, T2>&&x, pair<T1, T2>&&y)
{
   swap(x.first, y.first);
   swap(x.second, y.second);
}
#endif

}  //namespace detail {

//!The pair is movable if any of its members is movable
template <class T1, class T2>
struct is_movable<boost::interprocess::detail::pair<T1, T2> >
{
   enum {  value = is_movable<T1>::value || is_movable<T2>::value };
};

//!The pair is movable if any of its members is movable
template <class T1, class T2>
struct is_movable<std::pair<T1, T2> >
{
   enum {  value = is_movable<T1>::value || is_movable<T2>::value };
};

///has_trivial_destructor_after_move<> == true_type
///specialization for optimizations
template <class T>
struct has_trivial_destructor_after_move
   : public boost::has_trivial_destructor<T>
{};

enum create_enum_t
{  DoCreate, DoOpen, DoCreateOrOpen   };

}  //namespace interprocess { 
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_DETAIL_UTILITIES_HPP

