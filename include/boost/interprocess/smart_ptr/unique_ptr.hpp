//////////////////////////////////////////////////////////////////////////////
// I, Howard Hinnant, hereby place this code in the public domain.
//////////////////////////////////////////////////////////////////////////////
//
// This file is the adaptation for Interprocess of
// Howard Hinnant's unique_ptr emulation
//
// (C) Copyright Ion Gaztanaga 2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_UNIQUE_PTR_HPP_INCLUDED
#define BOOST_INTERPROCESS_UNIQUE_PTR_HPP_INCLUDED

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/assert.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/move.hpp>
#include <boost/compressed_pair.hpp>
#include <boost/static_assert.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits.hpp>
#include <boost/utility/enable_if.hpp>
#include <cstddef>

namespace boost{
namespace interprocess{

template <class T>
struct default_delete
{
    default_delete() {}
    template <class U> default_delete(const default_delete<U>&) {}
    void operator() (T* ptr) const
    {
        BOOST_STATIC_ASSERT(sizeof(T) > 0);
        delete ptr;
    }
};

template <class T>
struct default_delete<T[]>
{
    void operator() (T* ptr) const
    {
        BOOST_STATIC_ASSERT(sizeof(T) > 0);
        delete [] ptr;
    }
};

template <class T, std::size_t N>
struct default_delete<T[N]>
{
    void operator() (T* ptr, std::size_t) const
    {
        BOOST_STATIC_ASSERT(sizeof(T) > 0);
        delete [] ptr;
    }
};

/// @cond
template <class T, class D> class unique_ptr;

namespace detail {

template <class T> struct unique_ptr_error;

template <class T, class D>
struct unique_ptr_error<const unique_ptr<T, D> >
{
    typedef unique_ptr<T, D> type;
};

}  //namespace detail {
/// @endcond

template <class T, class D = default_delete<T> >
class unique_ptr
{
   /// @cond
   struct nat {int for_bool_;};
   typedef typename boost::add_reference<D>::type deleter_reference;
   typedef typename boost::add_reference<const D>::type deleter_const_reference;
   /// @endcond

   public:
   typedef T element_type;
   typedef D deleter_type;
   typedef typename detail::pointer_type<T, D>::type pointer;

   // constructors
   unique_ptr()
      :  ptr_(pointer())
   {}

   explicit unique_ptr(pointer p)
      :  ptr_(p)
   {}

   unique_ptr(pointer p
             ,typename boost::mpl::if_<boost::is_reference<D>
                  ,D
                  ,typename boost::add_reference<const D>::type>::type d)
      : ptr_(p, d)
   {}
/*
   unique_ptr(const unique_ptr& u)
      : ptr_(const_cast<unique_ptr&>(u).release(), u.get_deleter())
   {}
*/
   unique_ptr(detail::moved_object<unique_ptr> u)
      : ptr_(u.get().release(), u.get().get_deleter())
   {}
/*
   template <class U, class E>
   unique_ptr(const unique_ptr<U, E>& u,
      typename boost::enable_if_c<
            boost::is_convertible<typename unique_ptr<U, E>::pointer, pointer>::value &&
            boost::is_convertible<E, D>::value &&
            (
               !boost::is_reference<D>::value ||
               boost::is_same<D, E>::value
            )
            ,
            nat
            >::type = nat())
      : ptr_(const_cast<unique_ptr<U,E>&>(u).release(), u.get_deleter())
   {}
*/
   template <class U, class E>
   unique_ptr(const detail::moved_object<unique_ptr<U, E> >& u,
      typename boost::enable_if_c<
            boost::is_convertible<typename unique_ptr<U, E>::pointer, pointer>::value &&
            boost::is_convertible<E, D>::value &&
            (
               !boost::is_reference<D>::value ||
               boost::is_same<D, E>::value
            )
            ,
            nat
            >::type = nat())
      : ptr_(const_cast<unique_ptr<U,E>&>(u.get()).release(), u.get().get_deleter())
   {}

   // destructor
   ~unique_ptr()
   {  reset(); }

   // assignment
   unique_ptr& operator=(const unique_ptr& cu)
   {
      unique_ptr& u = const_cast<unique_ptr&>(cu);
      reset(u.release());
      ptr_.second() = u.get_deleter();
      return *this;
   }

   template <class U, class E>
   unique_ptr& operator=(const unique_ptr<U, E>& cu)
   {
      unique_ptr<U, E>& u = const_cast<unique_ptr<U, E>&>(cu);
      reset(u.release());
      ptr_.second() = u.get_deleter();
      return *this;
   }

   unique_ptr& operator=(int nat::*)
   {
      reset();
      return *this;
   }

   // observers
   typename boost::add_reference<T>::type operator*()  const
   {  return *ptr_.first();   }

   pointer operator->() const
   {  return ptr_.first(); }

   pointer get()        const
   {  return ptr_.first(); }

   deleter_reference       get_deleter()       
   {  return ptr_.second();   }

   deleter_const_reference get_deleter() const 
   {  return ptr_.second();   }

   operator int nat::*() const 
   {  return ptr_.first() ? &nat::for_bool_ : 0;   }

   // modifiers
   pointer release()
   {
      pointer tmp = ptr_.first();
      ptr_.first() = pointer();
      return tmp;
   }

   void reset(pointer p = pointer())
   {
      if (ptr_.first() != p){
         if (ptr_.first())
            ptr_.second()(ptr_.first());
         ptr_.first() = p;
      }
   }

   void swap(unique_ptr& u)
   {  ptr_.swap(u.ptr_);   }
   
   void swap(detail::moved_object<unique_ptr> mu)
   {  ptr_.swap(mu.get().ptr_);  }

   /// @cond
   private:
   boost::compressed_pair<pointer, D> ptr_;

   //This private constructor avoids moving from non-const lvalues
   unique_ptr(const unique_ptr&);
   template <class U, class E> unique_ptr(unique_ptr<U, E>&);
   template <class U> unique_ptr(U&, typename detail::unique_ptr_error<U>::type = 0);

   
   unique_ptr& operator=(unique_ptr&);
   template <class U, class E> unique_ptr& operator=(unique_ptr<U, E>&);
   template <class U> typename detail::unique_ptr_error<U>::type operator=(U&);
   /// @endcond
};
/*
template <class T, class D>
class unique_ptr<T[], D>
{
    struct nat {int for_bool_;};
    typedef typename boost::add_reference<D>::type deleter_reference;
    typedef typename boost::add_reference<const D>::type deleter_const_reference;
public:
    typedef T element_type;
    typedef D deleter_type;
    typedef typename detail::pointer_type<T, D>::type pointer;

    // constructors
    unique_ptr() : ptr_(pointer()) {}
    explicit unique_ptr(pointer p) : ptr_(p) {}
    unique_ptr(pointer p, typename boost::mpl::if_<
                          boost::is_reference<D>,
                          D,
                          typename boost::add_reference<const D>::type>::type d)
        : ptr_(p, d) {}
    unique_ptr(const unique_ptr& u)
        : ptr_(const_cast<unique_ptr&>(u).release(), u.get_deleter()) {}

    // destructor
    ~unique_ptr() {reset();}

    // assignment
    unique_ptr& operator=(const unique_ptr& cu)
    {
        unique_ptr& u = const_cast<unique_ptr&>(cu);
        reset(u.release());
        ptr_.second() = u.get_deleter();
        return *this;
    }
    unique_ptr& operator=(int nat::*)
    {
        reset();
        return *this;
    }

    // observers
    typename boost::add_reference<T>::type operator[](std::size_t i)  const {return ptr_.first()[i];}
    pointer get()        const {return ptr_.first();}
    deleter_reference       get_deleter()       {return ptr_.second();}
    deleter_const_reference get_deleter() const {return ptr_.second();}
    operator int nat::*() const {return ptr_.first() ? &nat::for_bool_ : 0;}

    // modifiers
    pointer release()
    {
        pointer tmp = ptr_.first();
        ptr_.first() = pointer();
        return tmp;
    }
    void reset(pointer p = pointer())
    {
        if (ptr_.first() != p)
        {
            if (ptr_.first())
                ptr_.second()(ptr_.first());
            ptr_.first() = p;
        }
    }
    void swap(unique_ptr& u) {ptr_.swap(u.ptr_);}
private:
    boost::compressed_pair<pointer, D> ptr_;

    template <class U, class E> unique_ptr(U p, E,
        typename boost::enable_if<boost::is_convertible<U, pointer> >::type* = 0);
    template <class U> explicit unique_ptr(U,
        typename boost::enable_if<boost::is_convertible<U, pointer> >::type* = 0);

    unique_ptr(unique_ptr&);
    template <class U> unique_ptr(U&, typename detail::unique_ptr_error<U>::type = 0);

    unique_ptr& operator=(unique_ptr&);
    template <class U> typename detail::unique_ptr_error<U>::type operator=(U&);
};

template <class T, class D, std::size_t N>
class unique_ptr<T[N], D>
{
    struct nat {int for_bool_;};
    typedef typename boost::add_reference<D>::type deleter_reference;
    typedef typename boost::add_reference<const D>::type deleter_const_reference;
public:
    typedef T element_type;
    typedef D deleter_type;
    typedef typename detail::pointer_type<T, D>::type pointer;
    static const std::size_t size = N;

    // constructors
    unique_ptr() : ptr_(pointer()) {}
    explicit unique_ptr(pointer p) : ptr_(p) {}
    unique_ptr(pointer p, typename boost::mpl::if_<
                         boost::is_reference<D>,
                         D,
                         typename boost::add_reference<const D>::type>::type d)
        : ptr_(p, d) {}
    unique_ptr(const unique_ptr& u)
        : ptr_(const_cast<unique_ptr&>(u).release(), u.get_deleter()) {}

    // destructor
    ~unique_ptr() {reset();}

    // assignment
    unique_ptr& operator=(const unique_ptr& cu)
    {
        unique_ptr& u = const_cast<unique_ptr&>(cu);
        reset(u.release());
        ptr_.second() = u.get_deleter();
        return *this;
    }
    unique_ptr& operator=(int nat::*)
    {
        reset();
        return *this;
    }

    // observers
    typename boost::add_reference<T>::type operator[](std::size_t i)  const {return ptr_.first()[i];}
    pointer get()        const {return ptr_.first();}
    deleter_reference       get_deleter()       {return ptr_.second();}
    deleter_const_reference get_deleter() const {return ptr_.second();}
    operator int nat::*() const {return ptr_.first() ? &nat::for_bool_ : 0;}

    // modifiers
    pointer release()
    {
        pointer tmp = ptr_.first();
        ptr_.first() = pointer();
        return tmp;
    }
    void reset(pointer p = pointer())
    {
        if (ptr_.first() != p)
        {
            if (ptr_.first())
                ptr_.second()(ptr_.first(), N);
            ptr_.first() = p;
        }
    }
    void swap(unique_ptr& u) {ptr_.swap(u.ptr_);}
private:
    boost::compressed_pair<pointer, D> ptr_;

    template <class U, class E> unique_ptr(U p, E,
        typename boost::enable_if<boost::is_convertible<U, pointer> >::type* = 0);
    template <class U> explicit unique_ptr(U,
        typename boost::enable_if<boost::is_convertible<U, pointer> >::type* = 0);

    unique_ptr(unique_ptr&);
    template <class U> unique_ptr(U&, typename detail::unique_ptr_error<U>::type = 0);

    unique_ptr& operator=(unique_ptr&);
    template <class U> typename detail::unique_ptr_error<U>::type operator=(U&);
};
*/
template <class T, class D> inline
void swap(unique_ptr<T, D>& x, unique_ptr<T, D>& y)
{  x.swap(y);  }

template <class T1, class D1, class T2, class D2> inline
bool operator==(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y)
{  return x.get() == y.get(); }

template <class T1, class D1, class T2, class D2> inline
bool operator!=(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y)
{  return x.get() != y.get(); }

template <class T1, class D1, class T2, class D2> inline
bool operator <(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y)
{  return x.get() < y.get();  }

template <class T1, class D1, class T2, class D2> inline
bool operator<=(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y)
{  return x.get() <= y.get(); }

template <class T1, class D1, class T2, class D2> inline
bool operator >(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y)
{  return x.get() > y.get();  }

template <class T1, class D1, class T2, class D2> inline
bool operator>=(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y)
{  return x.get() >= y.get(); }

/// @cond
/*!This class has move constructor*/
template <class T, class D>
struct is_movable<unique_ptr<T, D> >
{
   enum {   value = true };
};
/// @endcond

}  //namespace interprocess{
}  //namespace boost{

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_UNIQUE_PTR_HPP_INCLUDED
