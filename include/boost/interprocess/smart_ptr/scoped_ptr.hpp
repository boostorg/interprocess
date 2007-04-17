//////////////////////////////////////////////////////////////////////////////
//
// This file is the adaptation for Interprocess of boost/scoped_ptr.hpp
//
// (C) Copyright Greg Colvin and Beman Dawes 1998, 1999.
// (C) Copyright Peter Dimov 2001, 2002
// (C) Copyright Ion Gazta�aga 2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_SCOPED_PTR_HPP_INCLUDED
#define BOOST_INTERPROCESS_SCOPED_PTR_HPP_INCLUDED

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/assert.hpp>

namespace boost {
namespace interprocess {

/*!scoped_ptr stores a pointer to a dynamically allocated object. 
   The object pointed to is guaranteed to be deleted, either on destruction
   of the scoped_ptr, or via an explicit reset. The user can avoid this
   deletion using release().
   scoped_ptr is parameterized on T (the type of the object pointed to) and 
   Deleter (the functor to be executed to delete the internal pointer).
   The internal pointer will be of the same pointer type as typename 
   Deleter::pointer type (that is, if typename Deleter::pointer is 
   offset_ptr<void>, the internal pointer will be offset_ptr<T>).*/
template<class T, class Deleter>
class scoped_ptr
   : private Deleter
{
   scoped_ptr(scoped_ptr const &);
   scoped_ptr & operator=(scoped_ptr const &);

   typedef scoped_ptr<T, Deleter> this_type;
   typedef typename workaround::random_it<T>::reference reference;

   public:

   /*!Provides the type of the stored pointer.*/
   typedef T element_type;

   /*!Provides the type of the internal stored pointer.*/
   typedef typename detail::pointer_to_other
            <typename Deleter::pointer, T>::type pointer;

   /*!Constructs a scoped_ptr, storing a copy of p(which can be 0) and d.
      Does not throw.*/
   explicit scoped_ptr(const pointer &p = 0, const Deleter &d = Deleter())
      : Deleter(d), m_ptr(p) // throws if pointer/Deleter copy ctor throws
   {}

   /*!If the stored pointer is not 0, destroys the object pointed to by the stored pointer. 
      calling the operator() of the stored deleter. Never throws*/
   ~scoped_ptr()
   { 
      if(m_ptr){
         Deleter &del = static_cast<Deleter&>(*this);
         del(m_ptr);
      }
   }

   /*!Deletes the object pointed to by the stored pointer and then
      stores a copy of p. Never throws*/
   void reset(const pointer &p = 0) // never throws
   {  BOOST_ASSERT(p == 0 || p != m_ptr); this_type(p).swap(*this);  }

   /*!Deletes the object pointed to by the stored pointer and then
      stores a copy of p and a copy of d.*/
   void reset(const pointer &p, const Deleter &d) // never throws
   {  BOOST_ASSERT(p == 0 || p != m_ptr); this_type(p).swap(*this);  }

   /*!Assigns internal pointer as 0 and returns previous pointer. This will
      avoid deletion on destructor*/
   pointer release()
   {  pointer tmp(m_ptr);  m_ptr = 0;  return tmp; }

   /*!Returns a reference to the object pointed to by the stored pointer.
      Never throws.*/
   reference operator*() const
   {  BOOST_ASSERT(m_ptr != 0);  return *m_ptr; }

   /*!Returns the internal stored pointer.*/
   pointer &operator->() // never throws
   {  BOOST_ASSERT(m_ptr != 0);  return m_ptr;  }

   /*!Returns the internal stored pointer. Never throws.*/
   const pointer &operator->() const
   {  BOOST_ASSERT(m_ptr != 0);  return m_ptr;  }

   /*!Returns the stored pointer. Never throws.*/
   pointer & get()
   {  return m_ptr;  }

   /*!Returns the stored pointer. Never throws.*/
   const pointer & get() const
   {  return m_ptr;  }

   // implicit conversion to "bool"
   typedef pointer this_type::*unspecified_bool_type;

   operator unspecified_bool_type() const // never throws
   {  return m_ptr == 0? 0: &this_type::m_ptr;  }

   /*!Returns true if the stored pointer is 0. Never throws.*/
   bool operator! () const // never throws
   {  return m_ptr == 0;   }

   /*!Exchanges the internal pointer and deleter with other scoped_ptr
      Never throws.*/
   void swap(scoped_ptr & b) // never throws
   {  detail::do_swap<Deleter>(*this, b); detail::do_swap(m_ptr, b.m_ptr); }

   private:
   pointer m_ptr;
};

/*!Exchanges the internal pointer and deleter with other scoped_ptr
   Never throws.*/
template<class T, class D> inline
void swap(scoped_ptr<T, D> & a, scoped_ptr<T, D> & b) // never throws
{  a.swap(b); }

/*!Returns a copy of the stored pointer*/
template<class T, class D> inline
typename scoped_ptr<T, D>::pointer get_pointer(scoped_ptr<T, D> const & p)
{  return p.get();   }

} // namespace interprocess
} // namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif // #ifndef BOOST_INTERPROCESS_SCOPED_PTR_HPP_INCLUDED