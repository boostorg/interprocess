/*
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Hewlett-Packard Company makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 *
 * Copyright (c) 1996
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 */
//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
//
// This file comes from SGI's stl_set/stl_multiset files. Modified by Ion Gaztañaga 2004.
// Renaming, isolating and porting to generic algorithms. Pointer typedef 
// set to allocator::pointer to allow placing it in shared memory.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_SET_HPP
#define BOOST_INTERPROCESS_SET_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <utility>
#include <functional>
#include <memory>
#include <boost/interprocess/containers/tree.hpp>


namespace boost {   namespace interprocess {

// Forward declarations of operators < and ==, needed for friend declaration.

template <class T, class Pred, class Alloc>
inline bool operator==(const set<T,Pred,Alloc>& x, 
                       const set<T,Pred,Alloc>& y);

template <class T, class Pred, class Alloc>
inline bool operator<(const set<T,Pred,Alloc>& x, 
                      const set<T,Pred,Alloc>& y);


template <class T, class Pred, class Alloc>
class set 
{
   private:
   typedef detail::rb_tree<T, T, 
                     detail::identity<T>, Pred, Alloc> tree_t;
   tree_t m_tree;  // red-black tree representing set

   public:
   // typedefs:
   typedef typename tree_t::key_type               key_type;
   typedef typename tree_t::value_type             value_type;
   typedef typename tree_t::pointer                pointer;
   typedef typename tree_t::const_pointer          const_pointer;
   typedef typename tree_t::reference              reference;
   typedef typename tree_t::const_reference        const_reference;
   typedef Pred                                    key_compare;
   typedef Pred                                    value_compare;
   typedef typename tree_t::iterator               iterator;
   typedef typename tree_t::const_iterator         const_iterator;
   typedef typename tree_t::reverse_iterator       reverse_iterator;
   typedef typename tree_t::const_reverse_iterator const_reverse_iterator;
   typedef typename tree_t::size_type              size_type;
   typedef typename tree_t::difference_type        difference_type;
   typedef typename tree_t::allocator_type         allocator_type;

   // allocation/deallocation
   explicit set(const Pred& comp = Pred(),
                const allocator_type& a = allocator_type())
      : m_tree(comp, a) {}

   template <class InputIterator>
   set(InputIterator first, InputIterator last)
      : m_tree(Pred(), allocator_type())
      { m_tree.insert_unique(first, last); }

   template <class InputIterator>
   set(InputIterator first, InputIterator last, const Pred& comp,
         const allocator_type& a = allocator_type())
      : m_tree(comp, a) 
      { m_tree.insert_unique(first, last); }

   set(const set<T,Pred,Alloc>& x) 
      : m_tree(x.m_tree) {}

   set<T,Pred,Alloc>& operator=(const set<T, Pred, Alloc>& x)
      {  m_tree = x.m_tree;   return *this;  }

   // accessors:

   key_compare key_comp() const 
      { return m_tree.key_comp(); }

   value_compare value_comp() const 
      { return m_tree.key_comp(); }

   allocator_type get_allocator() const 
      { return m_tree.get_allocator(); }

   const_iterator begin() const 
      { return m_tree.begin(); }

   const_iterator end() const 
      { return m_tree.end(); }

   const_reverse_iterator rbegin() const 
      { return m_tree.rbegin(); } 

   const_reverse_iterator rend() const 
      { return m_tree.rend(); }

   iterator begin() 
      { return m_tree.begin(); }

   iterator end() 
      { return m_tree.end(); }

   reverse_iterator rbegin() 
      { return m_tree.rbegin(); } 

   reverse_iterator rend() 
      { return m_tree.rend(); }

   bool empty() const 
      { return m_tree.empty(); }

   size_type size() const 
      { return m_tree.size(); }

   size_type max_size() const 
      { return m_tree.max_size(); }

   void swap(set<T,Pred,Alloc>& x) 
      { m_tree.swap(x.m_tree); }

   // insert/erase
   std::pair<iterator,bool> insert(const value_type& x) 
      {  return m_tree.insert_unique(x);  }

   iterator insert(iterator position, const value_type& x) 
      {  return m_tree.insert_unique(position, x); }

   template <class InputIterator>
   void insert(InputIterator first, InputIterator last) 
      {  m_tree.insert_unique(first, last);  }

   iterator erase(const_iterator position) 
      {  return m_tree.erase(position); }

   size_type erase(const key_type& x) 
      {  return m_tree.erase(x); }

   iterator erase(const_iterator first, const_iterator last) 
      {  return m_tree.erase(first, last);  }

   void clear() 
      { m_tree.clear(); }

// set operations:

   const_iterator find(const key_type& x) const 
      { return m_tree.find(x); }

   iterator find(const key_type& x) 
      { return m_tree.find(x); }

   size_type count(const key_type& x) const 
      {  return m_tree.find(x) == m_tree.end() ? 0 : 1;  }

   const_iterator lower_bound(const key_type& x) const 
      {  return m_tree.lower_bound(x); }

   iterator lower_bound(const key_type& x) 
      {  return m_tree.lower_bound(x); }

   const_iterator upper_bound(const key_type& x) const 
      {  return m_tree.upper_bound(x);    }

   iterator upper_bound(const key_type& x)
      {  return m_tree.upper_bound(x);    }

   std::pair<const_iterator, const_iterator> 
      equal_range(const key_type& x) const 
      {  return m_tree.equal_range(x); }

   std::pair<iterator,iterator> 
      equal_range(const key_type& x) 
      {  return m_tree.equal_range(x); }

   template <class K1, class C1, class A1>
   friend bool operator== (const set<K1,C1,A1>&, const set<K1,C1,A1>&);

   template <class K1, class C1, class A1>
   friend bool operator< (const set<K1,C1,A1>&, const set<K1,C1,A1>&);
};

template <class T, class Pred, class Alloc>
inline bool operator==(const set<T,Pred,Alloc>& x, 
                       const set<T,Pred,Alloc>& y) 
{  return x.m_tree == y.m_tree;  }

template <class T, class Pred, class Alloc>
inline bool operator<(const set<T,Pred,Alloc>& x, 
                      const set<T,Pred,Alloc>& y) 
{  return x.m_tree < y.m_tree;   }

template <class T, class Pred, class Alloc>
inline bool operator!=(const set<T,Pred,Alloc>& x, 
                       const set<T,Pred,Alloc>& y) 
{  return !(x == y);   }

template <class T, class Pred, class Alloc>
inline bool operator>(const set<T,Pred,Alloc>& x, 
                      const set<T,Pred,Alloc>& y) 
{  return y < x; }

template <class T, class Pred, class Alloc>
inline bool operator<=(const set<T,Pred,Alloc>& x, 
                       const set<T,Pred,Alloc>& y) 
{  return !(y < x); }

template <class T, class Pred, class Alloc>
inline bool operator>=(const set<T,Pred,Alloc>& x, 
                       const set<T,Pred,Alloc>& y) 
{  return !(x < y);  }

template <class T, class Pred, class Alloc>
inline void swap(set<T,Pred,Alloc>& x, 
                 set<T,Pred,Alloc>& y) 
{  x.swap(y);  }

// Forward declaration of operators < and ==, needed for friend declaration.

template <class T, class Pred, class Alloc>
inline bool operator==(const multiset<T,Pred,Alloc>& x, 
                       const multiset<T,Pred,Alloc>& y);

template <class T, class Pred, class Alloc>
inline bool operator<(const multiset<T,Pred,Alloc>& x, 
                      const multiset<T,Pred,Alloc>& y);

template <class T, class Pred, class Alloc>
class multiset 
{
 private:
   typedef detail::rb_tree<T, T, 
                     detail::identity<T>, Pred, Alloc> tree_t;
   tree_t m_tree;  // red-black tree representing multiset

 public:
   // typedefs:
   typedef typename tree_t::key_type               key_type;
   typedef typename tree_t::value_type             value_type;
   typedef typename tree_t::pointer                pointer;
   typedef typename tree_t::const_pointer          const_pointer;
   typedef typename tree_t::reference              reference;
   typedef typename tree_t::const_reference        const_reference;
   typedef Pred                                    key_compare;
   typedef Pred                                    value_compare;
   typedef typename tree_t::iterator               iterator;
   typedef typename tree_t::const_iterator         const_iterator;
   typedef typename tree_t::reverse_iterator       reverse_iterator;
   typedef typename tree_t::const_reverse_iterator const_reverse_iterator;
   typedef typename tree_t::size_type              size_type;
   typedef typename tree_t::difference_type        difference_type;
   typedef typename tree_t::allocator_type         allocator_type;

   // allocation/deallocation
   explicit multiset(const Pred& comp = Pred(),
                     const allocator_type& a = allocator_type())
      : m_tree(comp, a) {}

   template <class InputIterator>
   multiset(InputIterator first, InputIterator last)
      : m_tree(Pred(), allocator_type())
      { m_tree.insert_equal(first, last); }

   template <class InputIterator>
   multiset(InputIterator first, InputIterator last,
            const Pred& comp,
            const allocator_type& a = allocator_type())
      : m_tree(comp, a) 
      { m_tree.insert_equal(first, last); }

   multiset(const multiset<T,Pred,Alloc>& x) 
      : m_tree(x.m_tree) {}

   multiset<T,Pred,Alloc>& operator=(const multiset<T,Pred,Alloc>& x) 
      {  m_tree = x.m_tree;   return *this;  }

   // accessors:

   key_compare key_comp() const 
      { return m_tree.key_comp(); }

   value_compare value_comp() const 
      { return m_tree.key_comp(); }

   allocator_type get_allocator() const 
      { return m_tree.get_allocator(); }

   const_iterator begin() const 
      { return m_tree.begin(); }

   const_iterator end() const 
      { return m_tree.end(); }

   const_reverse_iterator rbegin() const 
      { return m_tree.rbegin(); } 

   const_reverse_iterator rend() const 
      { return m_tree.rend(); }

   iterator begin() 
      { return m_tree.begin(); }

   iterator end() 
      { return m_tree.end(); }

   reverse_iterator rbegin() 
      { return m_tree.rbegin(); } 

   reverse_iterator rend() 
      { return m_tree.rend(); }

   bool empty() const 
      { return m_tree.empty(); }

   size_type size() const 
      { return m_tree.size(); }

   size_type max_size() const 
      { return m_tree.max_size(); }

   void swap(multiset<T,Pred,Alloc>& x) 
      { m_tree.swap(x.m_tree); }

   // insert/erase
   iterator insert(const value_type& x) 
      {  return m_tree.insert_equal(x);   }

   iterator insert(iterator position, const value_type& x) 
      {  return m_tree.insert_equal(position, x);  }

   template <class InputIterator>
   void insert(InputIterator first, InputIterator last) 
      {  m_tree.insert_equal(first, last);  }

   iterator erase(const_iterator position) 
      {  return m_tree.erase(position); }

   size_type erase(const key_type& x) 
      {  return m_tree.erase(x); }

   iterator erase(const_iterator first, const_iterator last)
      {  return m_tree.erase(first, last); }

   void clear() 
      { m_tree.clear(); }

   // multiset operations:
   const_iterator find(const key_type& x) const 
      { return m_tree.find(x); }

   iterator find(const key_type& x) 
      { return m_tree.find(x); }

   size_type count(const key_type& x) const 
      {  return m_tree.count(x);  }

   const_iterator lower_bound(const key_type& x) const 
      {  return m_tree.lower_bound(x); }

   iterator lower_bound(const key_type& x) 
      {  return m_tree.lower_bound(x); }

   const_iterator upper_bound(const key_type& x) const 
      {  return m_tree.upper_bound(x);    }

   iterator upper_bound(const key_type& x)
      {  return m_tree.upper_bound(x);    }

   std::pair<const_iterator, const_iterator> 
      equal_range(const key_type& x) const 
      {  return m_tree.equal_range(x); }

   std::pair<iterator,iterator> 
      equal_range(const key_type& x) 
      {  return m_tree.equal_range(x); }

   template <class K1, class C1, class A1>
   friend bool operator== (const multiset<K1,C1,A1>&,
                           const multiset<K1,C1,A1>&);
   template <class K1, class C1, class A1>
   friend bool operator< (const multiset<K1,C1,A1>&,
                          const multiset<K1,C1,A1>&);
};

template <class T, class Pred, class Alloc>
inline bool operator==(const multiset<T,Pred,Alloc>& x, 
                       const multiset<T,Pred,Alloc>& y) 
{  return x.m_tree == y.m_tree;  }

template <class T, class Pred, class Alloc>
inline bool operator<(const multiset<T,Pred,Alloc>& x, 
                      const multiset<T,Pred,Alloc>& y) 
{  return x.m_tree < y.m_tree;   }

template <class T, class Pred, class Alloc>
inline bool operator!=(const multiset<T,Pred,Alloc>& x, 
                       const multiset<T,Pred,Alloc>& y) 
{  return !(x == y);  }

template <class T, class Pred, class Alloc>
inline bool operator>(const multiset<T,Pred,Alloc>& x, 
                      const multiset<T,Pred,Alloc>& y) 
{  return y < x;  }

template <class T, class Pred, class Alloc>
inline bool operator<=(const multiset<T,Pred,Alloc>& x, 
                       const multiset<T,Pred,Alloc>& y) 
{  return !(y < x);  }

template <class T, class Pred, class Alloc>
inline bool operator>=(const multiset<T,Pred,Alloc>& x, 
                       const multiset<T,Pred,Alloc>& y) 
{  return !(x < y);  }

template <class T, class Pred, class Alloc>
inline void swap(multiset<T,Pred,Alloc>& x, 
                 multiset<T,Pred,Alloc>& y) 
{  x.swap(y);  }

}} //namespace boost {   namespace interprocess {

#include <boost/interprocess/detail/config_end.hpp>

#endif /* BOOST_INTERPROCESS_SET_HPP */

// Local Variables:
// mode:C++
// End:
