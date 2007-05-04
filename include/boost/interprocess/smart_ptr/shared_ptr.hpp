//////////////////////////////////////////////////////////////////////////////
//
// This file is the adaptation for Interprocess of boost/shared_ptr.hpp
//
// (C) Copyright Greg Colvin and Beman Dawes 1998, 1999.
// (C) Copyright Peter Dimov 2001, 2002, 2003
// (C) Copyright Ion Gaztañaga 2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_SHARED_PTR_HPP_INCLUDED
#define BOOST_INTERPROCESS_SHARED_PTR_HPP_INCLUDED

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/assert.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/generic_cast.hpp>
#include <boost/interprocess/detail/cast_tags.hpp>
#include <boost/assert.hpp>
#include <boost/throw_exception.hpp>
#include <boost/interprocess/smart_ptr/detail/shared_count.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_pointer.hpp>

#include <algorithm>            // for std::swap
#include <functional>           // for std::less
#include <typeinfo>             // for std::bad_cast
#include <iosfwd>               // for std::basic_ostream

namespace boost{
namespace interprocess{

template<class T, class VA, class D> class weak_ptr;
template<class T, class VA, class D> class enable_shared_from_this;

namespace detail{

template<class T, class VA, class D>
inline void sp_enable_shared_from_this
  (shared_count<T, VA, D> const & pn,
   const typename pointer_to_other <typename shared_count<T, VA, D>::pointer, 
                                    enable_shared_from_this<T, VA, D> >::type &pe,
   const typename shared_count<T, VA, D>::pointer &px)
{
    if(pe != 0)
      pe->_internal_weak_this._internal_assign(pn);
}
/*
template<class T, class VA, class D>
inline void sp_enable_shared_from_this(shared_count<T, VA, D> const &, ...)
{}
*/
} // namespace detail

/*!shared_ptr stores a pointer to a dynamically allocated object. 
   The object pointed to is guaranteed to be deleted when the last shared_ptr pointing to 
   it is destroyed or reset. shared_ptr is parameterized on 
   T (the type of the object pointed to), VA (the void allocator to be used
   to allocate the auxiliary data) and D (the deleter whose 
   operator() will be used to delete the object.
   The internal pointer will be of the same pointer type as typename 
   VA::pointer type (that is, if typename VA::pointer is 
   offset_ptr<void>, the internal pointer will be offset_ptr<T>).*/
template<class T, class VA, class D>
class shared_ptr
{
   /// @cond
   private:
   typedef shared_ptr<T, VA, D> this_type;
   /// @endcond

   public:
   /*!Provides the type of the stored pointer.*/
   typedef T                                                   element_type;
   /*!Provides the type of the stored pointer.*/
   typedef T                                                   value_type;
   /*!Provides the type of the internal stored pointer.*/
   typedef typename detail::pointer_to_other
      <typename VA::pointer, T>::type                          pointer;

   typedef typename workaround::random_it<T>::reference        reference;
   typedef typename workaround::random_it<T>::const_reference  const_reference;
   typedef typename detail::pointer_to_other
            <typename VA::pointer, const D>::type     const_deleter_pointer;
   typedef typename detail::pointer_to_other
            <typename VA::pointer, const VA>::type    const_allocator_pointer;

   /*!Constructs an empty shared_ptr. Use_count() == 0 && get() == 0.*/
   shared_ptr()
      :  m_pn() // never throws
   {}

   /*!Constructs a shared_ptr that owns the pointer p. Auxiliary data will be allocated
      with a copy of a and the object will be deleted with a copy of d.
      Requirements: D and A's copy constructor must not throw.*/
   explicit shared_ptr(const pointer&p, const VA &a = VA(), const D &d = D())
      :  m_pn(p, a, d)
   {  
      typedef pointer Pointer;
      //Check that the pointer passed is of the same type that
      //the pointer the allocator defines or it's a raw pointer
      typedef typename detail::pointer_to_other<Pointer, T>::type ParameterPointer;
      BOOST_STATIC_ASSERT((boost::is_same<pointer, ParameterPointer>::value)||
                          (boost::is_pointer<Pointer>::value));
      //detail::sp_enable_shared_from_this( m_pn, p, p ); 
   }

   /*!If r is empty, constructs an empty shared_ptr. Otherwise, constructs 
      a shared_ptr that shares ownership with r. Never throws.*/
   template<class Y>
   shared_ptr(shared_ptr<Y, VA, D> const & r)
      :  m_pn(r.m_pn) // never throws
   {}

   /*!Constructs a shared_ptr that shares ownership with r and stores 
      a copy of the pointer stored in r.*/
   template<class Y>
   explicit shared_ptr(weak_ptr<Y, VA, D> const & r)
      :  m_pn(r.m_pn) // may throw
   {}

   template<class Y>
   shared_ptr(shared_ptr<Y, VA, D> const & r, detail::static_cast_tag)
      :  m_pn(cast_to<typename detail::pointer_to_other<pointer, T>::type>::
            using_static_cast(r.m_pn.get_pointer()),
              r.m_pn) 
   {}

   template<class Y>
   shared_ptr(shared_ptr<Y, VA, D> const & r, detail::const_cast_tag)
      :  m_pn(cast_to<typename detail::pointer_to_other<pointer, T>::type>::
            using_const_cast(r.m_pn.get_pointer()),
              r.m_pn) 
   {}

   template<class Y>
   shared_ptr(shared_ptr<Y, VA, D> const & r, detail::dynamic_cast_tag)
      :  m_pn(cast_to<typename detail::pointer_to_other<pointer, T>::type>::
            using_dynamic_cast(r.m_pn.get_pointer()),
              r.m_pn) 
   {
      if(!m_pn.get_pointer()){ // need to allocate new counter -- the cast failed
         m_pn = detail::shared_count<T, VA, D>();
      }
   }

   /*!Equivalent to shared_ptr(r).swap(*this). Never throws*/
   template<class Y>
   shared_ptr & operator=(shared_ptr<Y, VA, D> const & r)
   {
      m_pn = r.m_pn; // shared_count::op= doesn't throw
      return *this;
   }

   void reset()
   {  
      this_type().swap(*this);   
   }

   template<class Pointer>
   void reset(const Pointer &p, const VA &a = VA(), const D &d = D())
   {  
      //Check that the pointer passed is of the same type that
      //the pointer the allocator defines or it's a raw pointer
      typedef typename detail::pointer_to_other<Pointer, T>::type ParameterPointer;
      BOOST_STATIC_ASSERT((boost::is_same<pointer, ParameterPointer>::value)||
                          (boost::is_pointer<Pointer>::value));
      this_type(p, a, d).swap(*this);  
   }

   reference operator* () const // never throws
   {  BOOST_ASSERT(m_pn.get_pointer() != 0);  return *m_pn.get_pointer(); }

   pointer operator-> () const // never throws
   {  BOOST_ASSERT(m_pn.get_pointer() != 0);  return m_pn.get_pointer();  }

   pointer &operator-> ()      // never throws
   {  BOOST_ASSERT(m_pn.get_pointer() != 0);  return m_pn.get_pointer();  }
   
   const pointer &get() const // never throws
   {  return m_pn.get_pointer();  }

   pointer &get()             // never throws
   {  return m_pn.get_pointer();  }

   // implicit conversion to "bool"
   void unspecified_bool_type_func() const {}
   typedef void (this_type::*unspecified_bool_type)() const;

   operator unspecified_bool_type() const // never throws
   {  return !m_pn.get_pointer() ? 0 : &this_type::unspecified_bool_type_func;  }

   bool operator! () const // never throws
   {  return !m_pn.get_pointer();   }

   bool unique() const // never throws
   {  return m_pn.unique();  }

   long use_count() const // never throws
   {  return m_pn.use_count();  }

   void swap(shared_ptr<T, VA, D> & other) // never throws
   {  m_pn.swap(other.m_pn);   }

   template<class T2, class A2, class D2> 
   bool _internal_less(shared_ptr<T2, A2, D2> const & rhs) const
   {  return m_pn < rhs.m_pn;  }

   const_deleter_pointer get_deleter() const
   {  return m_pn.get_deleter(); }

   const_allocator_pointer get_allocator() const
   {  return m_pn.get_allocator(); }

   /// @cond
   private:

   template<class T2, class A2, class D2> friend class shared_ptr;
   template<class T2, class A2, class D2> friend class weak_ptr;

   detail::shared_count<T, VA, D>   m_pn;    // reference counter
   /// @endcond
};  // shared_ptr

template<class T, class VA, class D, class U, class VA2, class D2> inline 
bool operator==(shared_ptr<T, VA, D> const & a, shared_ptr<U, VA2, D2> const & b)
{  return a.get() == b.get(); }

template<class T, class VA, class D, class U, class VA2, class D2> inline 
bool operator!=(shared_ptr<T, VA, D> const & a, shared_ptr<U, VA2, D2> const & b)
{  return a.get() != b.get(); }

template<class T, class VA, class D, class U, class VA2, class D2> inline 
bool operator<(shared_ptr<T, VA, D> const & a, shared_ptr<U, VA2, D2> const & b)
{  return a._internal_less(b);   }

template<class T, class VA, class D> inline 
void swap(shared_ptr<T, VA, D> & a, shared_ptr<T, VA, D> & b)
{  a.swap(b);  }

template<class T, class VA, class D, class U> inline
shared_ptr<T, VA, D> static_pointer_cast(shared_ptr<U, VA, D> const & r)
{  return shared_ptr<T, VA, D>(r, detail::static_cast_tag());   }

template<class T, class VA, class D, class U> inline 
shared_ptr<T, VA, D> const_pointer_cast(shared_ptr<U, VA, D> const & r)
{  return shared_ptr<T, VA, D>(r, detail::const_cast_tag()); }

template<class T, class VA, class D, class U> inline 
shared_ptr<T, VA, D> dynamic_pointer_cast(shared_ptr<U, VA, D> const & r)
{  return shared_ptr<T, VA, D>(r, detail::dynamic_cast_tag());  }

// get_pointer() enables boost::mem_fn to recognize shared_ptr
template<class T, class VA, class D> inline
T * get_pointer(shared_ptr<T, VA, D> const & p)
{  return p.get();   }

// operator<<
template<class E, class T, class Y, class VA, class D> inline
std::basic_ostream<E, T> & operator<< 
   (std::basic_ostream<E, T> & os, shared_ptr<Y, VA, D> const & p)
{  os << p.get();   return os;   }
/*
// get_deleter (experimental)
template<class T, class VA, class D> 
typename detail::pointer_to_other<shared_ptr<T, VA, D>, D>::type
   get_deleter(shared_ptr<T, VA, D> const & p)
{  return static_cast<D *>(p._internal_get_deleter(typeid(D)));   }
*/
} // namespace interprocess

/// @cond
#if defined(_MSC_VER) && (_MSC_VER < 1400)
// get_pointer() enables boost::mem_fn to recognize shared_ptr
template<class T, class VA, class D> inline
T * get_pointer(boost::interprocess::shared_ptr<T, VA, D> const & p)
{  return p.get();   }
#endif
/// @endcond

} // namespace boost

/// @cond
namespace boost{
namespace interprocess{

/*!Simulation of cast operators between pointers.*/
template<class T, class VA, class D>
class cast_to< shared_ptr<T, VA, D> >
{
   public:
   template<class S>
   static shared_ptr<T, VA, D> using_static_cast(const shared_ptr<S, VA, D> &s)
   {  return shared_ptr<T, VA, D>(s, detail::static_cast_tag());   }

   template<class S>
   static shared_ptr<T, VA, D> using_reinterpret_cast(const shared_ptr<S, VA, D> &s)
   {  return shared_ptr<T, VA, D>(s, detail::reinterpret_cast_tag());   }

   template<class S>
   static shared_ptr<T, VA, D> using_const_cast(const shared_ptr<S, VA, D> &s)
   {  return shared_ptr<T, VA, D>(s, detail::const_cast_tag());   }

   template<class S>
   static shared_ptr<T, VA, D> using_dynamic_cast(const shared_ptr<S, VA, D> &s)
   {  return shared_ptr<T, VA, D>(s, detail::dynamic_cast_tag());   }
};
/// @endcond

}  //namespace interprocess{
}  //namespace boost{

#include <boost/interprocess/detail/config_end.hpp>

#endif  // #ifndef BOOST_INTERPROCESS_SHARED_PTR_HPP_INCLUDED
