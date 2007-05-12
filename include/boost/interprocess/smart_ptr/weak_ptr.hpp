//////////////////////////////////////////////////////////////////////////////
//
// This file is the adaptation for Interprocess of boost/weak_ptr.hpp
//
// (C) Copyright Peter Dimov 2001, 2002, 2003
// (C) Copyright Ion Gaztanaga 2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_WEAK_PTR_HPP_INCLUDED
#define BOOST_INTERPROCESS_WEAK_PTR_HPP_INCLUDED

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/smart_ptr/shared_ptr.hpp>

namespace boost{
namespace interprocess{

template<class T, class A, class D>
class weak_ptr
{
   /// @cond
   private:
   // Borland 5.5.1 specific workarounds
   typedef weak_ptr<T, A, D> this_type;
   typedef typename detail::pointer_to_other
      <typename A::pointer, T>::type      pointer;
   typedef typename workaround::random_it<T>::reference       reference;
   typedef typename workaround::random_it<T>::const_reference const_reference;
   /// @endcond

   public:
   typedef T element_type;
   typedef T value_type;

   weak_ptr()
   : m_pn() // never throws
   {}
   //  generated copy constructor, assignment, destructor are fine
   //
   //  The "obvious" converting constructor implementation:
   //
   //  template<class Y>
   //  weak_ptr(weak_ptr<Y> const & r): m_px(r.m_px), m_pn(r.m_pn) // never throws
   //  {
   //  }
   //
   //  has a serious problem.
   //
   //  r.m_px may already have been invalidated. The m_px(r.m_px)
   //  conversion may require access to *r.m_px (virtual inheritance).
   //
   //  It is not possible to avoid spurious access violations since
   //  in multithreaded programs r.m_px may be invalidated at any point.
   template<class Y>
   weak_ptr(weak_ptr<Y, A, D> const & r)
      : m_pn(r.m_pn) // never throws
   {  
      //Construct a temporary shared_ptr so that nobody
      //can destroy the value while constructing this
      const shared_ptr<T, A, D> &ref = r.lock();
      m_pn.set_pointer(ref.get()); 
   }

   template<class Y>
   weak_ptr(shared_ptr<Y, A, D> const & r)
      : m_pn(r.m_pn) // never throws
   {}

   template<class Y>
   weak_ptr & operator=(weak_ptr<Y, A, D> const & r) // never throws
   {  
      //Construct a temporary shared_ptr so that nobody
      //can destroy the value while constructing this
      const shared_ptr<T, A, D> &ref = r.lock();
      m_pn = r.m_pn;
      m_pn.set_pointer(ref.get());
      return *this;
   }

   template<class Y>
   weak_ptr & operator=(shared_ptr<Y, A, D> const & r) // never throws
   {  m_pn = r.m_pn;  return *this;  }

   shared_ptr<T, A, D> lock() const // never throws
   {
      // optimization: avoid throw overhead
      if(expired()){
         return shared_ptr<element_type, A, D>();
      }
      BOOST_TRY{
         return shared_ptr<element_type, A, D>(*this);
      }
      BOOST_CATCH(bad_weak_ptr const &){
         // Q: how can we get here?
         // A: another thread may have invalidated r after the use_count test above.
         return shared_ptr<element_type, A, D>();
      }
      BOOST_CATCH_END
   }

   long use_count() const // never throws
   {  return m_pn.use_count();  }

   bool expired() const // never throws
   {  return m_pn.use_count() == 0;   }

   void reset() // never throws in 1.30+
   {  this_type().swap(*this);   }

   void swap(this_type & other) // never throws
   {  detail::do_swap(m_pn, other.m_pn);   }

   template<class T2, class A2, class D2> 
   bool _internal_less(weak_ptr<T2, A2, D2> const & rhs) const
   {  return m_pn < rhs.m_pn;  }
   
   template<class Y>
   void _internal_assign(const detail::shared_count<Y, A, D> & pn2)
   {  m_pn = pn2;   }

   /// @cond
   private:

   template<class T2, class A2, class D2> friend class shared_ptr;
   template<class T2, class A2, class D2> friend class weak_ptr;

   detail::weak_count<T, A, D> m_pn;      // reference counter
   /// @endcond
};  // weak_ptr

template<class T, class A, class D, class U, class A2, class D2> inline 
bool operator<(weak_ptr<T, A, D> const & a, weak_ptr<U, A2, D2> const & b)
{  return a._internal_less(b);   }

template<class T, class A, class D> inline
void swap(weak_ptr<T, A, D> & a, weak_ptr<T, A, D> & b)
{  a.swap(b);  }

} // namespace interprocess
} // namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif  // #ifndef BOOST_INTERPROCESS_WEAK_PTR_HPP_INCLUDED
