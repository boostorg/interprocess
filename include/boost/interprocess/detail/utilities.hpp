//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2007.
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
#include <boost/get_pointer.hpp>
#include <boost/detail/no_exceptions_support.hpp>
#include <boost/interprocess/detail/move.hpp>
#include <boost/type_traits/has_nothrow_destructor.hpp>
#include <boost/type_traits/has_trivial_destructor.hpp>
#include <boost/call_traits.hpp>
#include <algorithm>
#include <functional>
#include <utility>
#include <stdexcept>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/intrusive/slist.hpp>

namespace boost { namespace interprocess { 

template<class T>
class offset_ptr;

namespace detail {

/*!Overload for smart pointers to avoid ADL problems with get_pointer*/
template<class Ptr>
inline typename Ptr::value_type *get_pointer(const Ptr &ptr)
{  using boost::get_pointer;  return get_pointer(ptr);   }

/*!Overload for raw pointers to avoid ADL problems with get_pointer*/
template<class T>
inline T *get_pointer(T *ptr)
{  return ptr; }

/*!To avoid ADL problems with swap*/
template <class T>
inline void do_swap(T& x, T& y)
{
   using std::swap;
   swap(x, y);
}

/*!A deleter for scoped_ptr that deallocates the memory
   allocated for an object using a STL allocator.*/
template <class Allocator>
struct scoped_deallocator
{
   typedef typename Allocator::pointer pointer;

   Allocator& m_alloc;

   scoped_deallocator(Allocator& a)
         : m_alloc(a) {}

   void operator()(pointer ptr)
      {  if (ptr) m_alloc.deallocate(ptr, 1);  }

};

/*!A deleter for scoped_ptr that deallocates the memory
   allocated for an array of objects using a STL allocator.*/
template <class Allocator>
struct scoped_array_deallocator
{
   typedef typename Allocator::pointer pointer;

   scoped_array_deallocator(Allocator& a, std::size_t length)
         : m_alloc(a), m_length(length) {}

   void operator()(pointer &ptr)
      {  m_alloc.deallocate(ptr, m_length);  }

   private:
   Allocator&  m_alloc;
   std::size_t m_length;
};

/*!A deleter for scoped_ptr that destroys
   an object using a STL allocator.*/
template <class Allocator>
struct scoped_destructor
{
   typedef typename Allocator::pointer pointer;

   Allocator& m_alloc;

   scoped_destructor(Allocator& a)
         : m_alloc(a){}

   void operator()(pointer &ptr)
      {  m_alloc.destroy(ptr);  }
};

/*!A deleter for scoped_ptr that destroys
   an object using a STL allocator.*/
template <class Allocator>
struct scoped_destructor_n
{
   typedef typename Allocator::pointer pointer;
   typedef typename Allocator::size_type size_type;

   Allocator& m_alloc;
   size_type  m_n;

   scoped_destructor_n(Allocator& a, size_type n)
         : m_alloc(a), m_n(n){}

   void operator()(pointer ptr)
   {
      for(size_type i = 0; i < m_n; ++i, ++ptr)
         m_alloc.destroy(ptr);
   }
};

template <class A>
class allocator_destroyer
{
   private:
   A & a_;

   public:
   allocator_destroyer(A &a)
      :  a_(a)
   {}

   void operator()(typename A::pointer p)
   {  
      a_.destroy(p);
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

template <class Pair>
struct select1st 
   : public std::unary_function<Pair, typename Pair::first_type> 
{
   const typename Pair::first_type& operator()(const Pair& x) const 
   {  return x.first;   }
};

// identity is an extension: it is not part of the standard.
template <class T>
struct identity 
   : public std::unary_function<T,T> 
{
   const T& operator()(const T& x) const 
   { return x; }
};

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

//Anti-exception node eraser
template<class Cont>
class value_eraser
{
   public:
   value_eraser(Cont & cont, typename Cont::iterator it) 
      : m_map(cont), m_index_it(it), m_erase(true){}
   ~value_eraser()  
   {
      if(boost::has_nothrow_destructor<typename Cont::value_type>::value){
         if(m_erase) m_map.erase(m_index_it);
      }
      else{
         //value_eraser is used in exceptions, so we
         //disable double-exception danger
         BOOST_TRY{  if(m_erase) m_map.erase(m_index_it);  }
         BOOST_CATCH(...){}
         BOOST_CATCH_END
      }
   }
   void release() {  m_erase = false;  }

   private:
   Cont                    &m_map;
   typename Cont::iterator  m_index_it;
   bool                    m_erase;
};

}  //namespace detail {

/*!Trait class to detect if an allocator has
   a templatized construct function. If this is the case
   in node containers we only need just one allocator
   instead of three*/
template <class Allocator>
struct has_convertible_construct
{
   enum {   value = false };
};

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

template <class T, class Difference = std::ptrdiff_t>
class constant_iterator
   :  public boost::iterator_facade
         < constant_iterator<T, Difference>
         , T
         , boost::random_access_traversal_tag
         , const T &
         , Difference>
{
   typedef boost::iterator_facade
         < constant_iterator<T, Difference>
         , T
         , boost::random_access_traversal_tag
         , const T &
         , Difference>  super_t;

   typedef  constant_iterator<T, Difference> this_type;
   //Give access to private core functions
   friend class boost::iterator_core_access;

   public:
   explicit constant_iterator(const T &ref, Difference range_size)
      :  m_ptr(&ref), m_num(range_size){}

   //Constructors
   constant_iterator()
      :  m_ptr(0), m_num(0){}

   private:
   const T *         m_ptr;
   Difference  m_num;

   void increment()
   { --m_num; }

   void decrement()
   { ++m_num; }

   bool equal(const this_type &other) const
   {  return m_num == other.m_num;   }

   const T & dereference() const
   { return *m_ptr; }

   void advance(Difference n)
   {  m_num -= n; }

   Difference distance_to(const this_type &other)const
   {  return m_num - other.m_num;   }
};

template <class T, class Difference = std::ptrdiff_t>
class repeat_iterator
   :  public boost::iterator_facade
         < repeat_iterator<T, Difference>
         , T
         , boost::random_access_traversal_tag
         , T &
         , Difference>
{
   typedef boost::iterator_facade
         < repeat_iterator<T, Difference>
         , T
         , boost::random_access_traversal_tag
         , T &
         , Difference>  super_t;

   typedef  repeat_iterator<T, Difference> this_type;
   //Give access to private core functions
   friend class boost::iterator_core_access;

   public:
   explicit repeat_iterator(T &ref, Difference range_size)
      :  m_ptr(&ref), m_num(range_size){}

   //Constructors
   repeat_iterator()
      :  m_ptr(0), m_num(0){}

   private:
   T *         m_ptr;
   Difference  m_num;

   void increment()
   { --m_num; }

   void decrement()
   { ++m_num; }

   bool equal(const this_type &other) const
   {  return m_num == other.m_num;   }

   T & dereference() const
   { return *m_ptr; }

   void advance(Difference n)
   {  m_num -= n; }

   Difference distance_to(const this_type &other)const
   {  return m_num - other.m_num;   }
};

template<class FwdIt, class Count, class T, class Alloc> inline
void uninitialized_fill_n(FwdIt first,  Count count, 
                          const T& val, Alloc& al)
{
   //Save initial position
   FwdIt init = first;

   BOOST_TRY{
      //Construct objects
      for (; count--; ++first){
         al.construct(first, val);
      }
   }
   BOOST_CATCH(...){
      //Call destructors
      for (; init != first; ++init){
         al.destroy(init);
      }
      BOOST_RETHROW;
   }
   BOOST_CATCH_END
}

template<class InIt, class OutIt>
InIt copy_n(InIt first, typename std::iterator_traits<InIt>::difference_type length, OutIt dest)
{   
   for (; length--; ++dest, ++first)
      *dest = *first;
   return first;
}


template<class InIt, class FwdIt, class Alloc> inline
typename std::iterator_traits<InIt>::difference_type
   n_uninitialized_copy(InIt first,   InIt last, 
                        FwdIt dest,    Alloc& al)
{
   //Save initial destination position
   FwdIt dest_init = dest;
   typename std::iterator_traits<InIt>::difference_type constructed = 0;
   BOOST_TRY{
      //Try to build objects
      for (; first != last; ++dest, ++first, ++constructed){
         al.construct(dest, *first);
      }
   }
   BOOST_CATCH(...){
      //Call destructors
      for (; dest_init != dest; ++dest_init){
         al.destroy(dest_init);
      }
      BOOST_RETHROW;
   }
   BOOST_CATCH_END
   return (constructed);
}

template<class InIt, class FwdIt, class Alloc> inline
FwdIt uninitialized_copy(InIt first,   InIt last, 
                         FwdIt dest,    Alloc& al)
{
   //Save initial destination position
   FwdIt dest_init = dest;
   typename std::iterator_traits<InIt>::difference_type constructed = 0;
   BOOST_TRY{
      //Try to build objects
      for (; first != last; ++dest, ++first, ++constructed){
         al.construct(dest, *first);
      }
   }
   BOOST_CATCH(...){
      //Call destructors
      for (; dest_init != dest; ++dest_init){
         al.destroy(dest_init);
      }
      BOOST_RETHROW;
   }
   BOOST_CATCH_END
   return (dest);
}

template<class InIt, class FwdIt, class Alloc> inline
InIt n_uninitialized_copy_n
   (InIt first, 
    typename std::iterator_traits<InIt>::difference_type count,
    FwdIt dest,    Alloc& al)
{
   //Save initial destination position
   FwdIt dest_init = dest;
   typename std::iterator_traits<InIt>::difference_type new_count = count+1;

   BOOST_TRY{
      //Try to build objects
      for (; --new_count; ++dest, ++first){
         al.construct(dest, *first);
      }
   }
   BOOST_CATCH(...){
      //Call destructors
      new_count = count - new_count;
      for (; new_count--; ++dest_init){
         al.destroy(dest_init);
      }
      BOOST_RETHROW;
   }
   BOOST_CATCH_END
   return first;
}

// uninitialized_copy_copy
// Copies [first1, last1) into [result, result + (last1 - first1)), and
// copies [first2, last2) into
// [result + (last1 - first1), result + (last1 - first1) + (last2 - first2)).
template <class InpIt1, class InpIt2, class FwdIt, class Alloc>
FwdIt uninitialized_copy_copy(InpIt1 first1, InpIt1 last1,
                                 InpIt2 first2, InpIt2 last2,
                                 FwdIt result, Alloc &alloc)
{
   FwdIt mid = uninitialized_copy(first1, last1, result, alloc);
   BOOST_TRY {
      return uninitialized_copy(first2, last2, mid, alloc);
   }
   BOOST_CATCH(...){
      for(;result != mid; ++result){
         alloc.destroy(&*result); 
      }
      BOOST_RETHROW
   }
   BOOST_CATCH_END
}

// uninitialized_copy_n_copy_n
// Copies [first1, first1 + n1) into [result, result + n1), and
// copies [first2, first2 + n2) into
// [result + n1, result + n1 + n2).
template <class InpIt1, class InpIt2, class FwdIt, class Alloc>
InpIt2 uninitialized_copy_n_copy_n
  (InpIt1 first1,
   typename std::iterator_traits<InpIt1>::difference_type n1,
   InpIt2 first2, 
   typename std::iterator_traits<InpIt2>::difference_type n2,
   FwdIt result,
   Alloc &alloc)
{
   typename std::iterator_traits<InpIt1>::difference_type c1 = n1+1;
   typename std::iterator_traits<InpIt2>::difference_type c2 = n2+1;
   FwdIt dest_init = result;

   BOOST_TRY{
      //Try to build objects
      for (; --c1; ++result, ++first1){
         alloc.construct(result, *first1);
      }
      for (; --c2; ++result, ++first2){
         alloc.construct(result, *first2);
      }
   }
   BOOST_CATCH(...){
      //Call destructors
   typename std::iterator_traits<FwdIt>::
      difference_type c = (n1 - c1) + (n2 - c2);
      for (; c--; ++dest_init){
         alloc.destroy(dest_init);
      }
      BOOST_RETHROW;
   }
   BOOST_CATCH_END
   return first2;
}

template<class T>
const T &max_value(const T &a, const T &b)
{  return a > b ? a : b;   }

template<class T>
const T &min_value(const T &a, const T &b)
{  return a < b ? a : b;   }

template <class SizeType>
SizeType
   get_next_capacity(const SizeType max_size
                    ,const SizeType capacity
                    ,const SizeType n)
{/*
   if (n > max_size - capacity)
      throw std::length_error("get_next_capacity");
*/
   const SizeType m3 = max_size/3;

   if (capacity < m3)
      return capacity + max_value(3*(capacity+1)/5, n);

   if (capacity < m3*2)
      return capacity + max_value((capacity+1)/2, n);

   return max_size;
}

namespace detail {

struct two {char _[2];};

namespace pointer_type_imp {

template <class U> static two  test(...);
template <class U> static char test(typename U::pointer* = 0);

}  //namespace pointer_type_imp {

template <class T>
struct has_pointer_type
{
    static const bool value = sizeof(pointer_type_imp::test<T>(0)) == 1;
};

namespace pointer_type_imp {

template <class T, class D, bool = has_pointer_type<D>::value>
struct pointer_type
{
    typedef typename D::pointer type;
};

template <class T, class D>
struct pointer_type<T, D, false>
{
    typedef T* type;
};

}  //namespace pointer_type_imp {

template <class T, class D>
struct pointer_type
{
    typedef typename pointer_type_imp::pointer_type<T,
        typename boost::remove_reference<D>::type>::type type;
};

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

   template <class D, class S>
   pair(const detail::moved_object<std::pair<D, S> >& p)
      : first(move(p.get().first)), second(move(p.get().second))
   {}

   template <class D, class S>
   pair(const detail::moved_object<pair<D, S> >& p)
      : first(move(p.get().first)), second(move(p.get().second))
   {}

   template <class U, class V>
   pair(const detail::moved_object<U> &x, const detail::moved_object<V> &y)
      : first(move(x.get())), second(move(y.get()))
   {}

   pair(const detail::moved_object<pair> &p)
      : first(move(p.get().first)), second(move(p.get().second))
   {}

   pair& operator=(const detail::moved_object<pair> &p)
   {
      first  = move(p.get().first);
      second = move(p.get().second);
      return *this;
   }

   pair& operator=(const pair &p)
   {
      first  = p.first;
      second = p.second;
      return *this;
   }

   template <class D, class S>
   pair& operator=(const detail::moved_object<std::pair<D, S> > &p)
   {
      first  = move(p.get().first);
      second = move(p.get().second);
      return *this;
   }

   void swap(const detail::moved_object<pair> &p)
   {  std::swap(*this, p.get()); }

   void swap(pair& p)
   {  std::swap(*this, p); }
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
{
   enum{ value = false  };
};

enum create_enum_t
{  DoCreate, DoOpen, DoCreateOrOpen   };

}  //namespace interprocess { 
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_DETAIL_UTILITIES_HPP

