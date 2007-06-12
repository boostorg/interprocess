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

#ifndef BOOST_INTERPROCESS_DETAIL_ITERATORS_HPP
#define BOOST_INTERPROCESS_DETAIL_ITERATORS_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/iterator/iterator_facade.hpp>

namespace boost {
namespace interprocess { 

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

}  //namespace interprocess { 
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_DETAIL_ITERATORS_HPP

