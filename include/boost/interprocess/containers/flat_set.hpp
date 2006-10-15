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

#ifndef BOOST_INTERPROCESS_SHMEM_FLAT_SET_HPP
#define BOOST_INTERPROCESS_SHMEM_FLAT_SET_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <utility>
#include <functional>
#include <memory>
#include <boost/interprocess/containers/detail/flat_tree.hpp>
#include <boost/interprocess/detail/move.hpp>


namespace boost {   namespace interprocess {

// Forward declarations of operators < and ==, needed for friend declaration.

template <class T, class Pred, class Alloc>
class flat_set;

template <class T, class Pred, class Alloc>
inline bool operator==(const flat_set<T,Pred,Alloc>& x, 
                       const flat_set<T,Pred,Alloc>& y);

template <class T, class Pred, class Alloc>
inline bool operator<(const flat_set<T,Pred,Alloc>& x, 
                      const flat_set<T,Pred,Alloc>& y);


template <class T, class Pred, class Alloc>
class flat_set 
{
 private:
   typedef detail::flat_tree<T, T, 
                     detail::identity<T>, Pred, Alloc> tree_t;
   tree_t m_flat_tree;  // flat tree representing flat_set

 public:
   // typedefs:
   typedef typename tree_t::key_type               key_type;
   typedef typename tree_t::value_type             value_type;
   typedef typename tree_t::pointer                pointer;
   typedef typename tree_t::const_pointer          const_pointer;
   typedef typename tree_t::reference              reference;
   typedef typename tree_t::const_reference        const_reference;
   typedef typename tree_t::key_compare            key_compare;
   typedef typename tree_t::value_compare          value_compare;
   typedef typename tree_t::iterator               iterator;
   typedef typename tree_t::const_iterator         const_iterator;
   typedef typename tree_t::reverse_iterator       reverse_iterator;
   typedef typename tree_t::const_reverse_iterator const_reverse_iterator;
   typedef typename tree_t::size_type              size_type;
   typedef typename tree_t::difference_type        difference_type;
   typedef typename tree_t::allocator_type         allocator_type;

   // allocation/deallocation
   explicit flat_set(const Pred& comp = Pred(),
                     const allocator_type& a = allocator_type())
      : m_flat_tree(comp, a) {}

   template <class InputIterator>
   flat_set(InputIterator first, InputIterator last, 
            const Pred& comp = Pred(),
            const allocator_type& a = allocator_type())
      : m_flat_tree(comp, a) 
      { m_flat_tree.insert_unique(first, last); }

   flat_set(const flat_set<T,Pred,Alloc>& x) 
      : m_flat_tree(x.m_flat_tree) {}

   flat_set(const detail::moved_object<flat_set<T,Pred,Alloc> >& mx) 
      : m_flat_tree(move(mx.get().m_flat_tree)) {}

   flat_set<T,Pred,Alloc>& operator=(const flat_set<T, Pred, Alloc>& x)
      {  m_flat_tree = x.m_flat_tree;   return *this;  }

   // accessors:

   key_compare key_comp() const 
      { return m_flat_tree.key_comp(); }

   value_compare value_comp() const 
      { return m_flat_tree.key_comp(); }

   allocator_type get_allocator() const 
      { return m_flat_tree.get_allocator(); }

   const_iterator begin() const 
      { return m_flat_tree.begin(); }

   const_iterator end() const 
      { return m_flat_tree.end(); }

   const_reverse_iterator rbegin() const 
      { return m_flat_tree.rbegin(); } 

   const_reverse_iterator rend() const 
      { return m_flat_tree.rend(); }

   iterator begin() 
      { return m_flat_tree.begin(); }

   iterator end() 
      { return m_flat_tree.end(); }

   reverse_iterator rbegin() 
      { return m_flat_tree.rbegin(); } 

   reverse_iterator rend() 
      { return m_flat_tree.rend(); }

   bool empty() const 
      { return m_flat_tree.empty(); }

   size_type size() const 
      { return m_flat_tree.size(); }

   size_type max_size() const 
      { return m_flat_tree.max_size(); }

   void swap(flat_set<T,Pred,Alloc>& x) 
      { m_flat_tree.swap(x.m_flat_tree); }

   void swap(const detail::moved_object <flat_set<T,Pred,Alloc> >& mx) 
      { this->swap(mx.get()); }

   // insert/erase
   std::pair<iterator,bool> insert(const value_type& x) 
      {  return m_flat_tree.insert_unique(x);  }

   std::pair<iterator,bool> insert(const detail::moved_object<value_type>& x) 
      {  return m_flat_tree.insert_unique(x);  }

   iterator insert(iterator position, const value_type& x) 
      {  return m_flat_tree.insert_unique(position, x); }

   iterator insert(iterator position, const detail::moved_object<value_type>& x) 
      {  return m_flat_tree.insert_unique(position, x); }

   template <class InputIterator>
   void insert(InputIterator first, InputIterator last) 
      {  m_flat_tree.insert_unique(first, last);  }

   iterator erase(const_iterator position) 
      {  return m_flat_tree.erase(position); }

   size_type erase(const key_type& x) 
      {  return m_flat_tree.erase(x); }

   iterator erase(const_iterator first, const_iterator last) 
      {  return m_flat_tree.erase(first, last);  }

   void clear() 
      { m_flat_tree.clear(); }

   // flat_set operations:

   const_iterator find(const key_type& x) const 
      { return m_flat_tree.find(x); }

   iterator find(const key_type& x) 
      { return m_flat_tree.find(x); }

   size_type count(const key_type& x) const 
      {  return m_flat_tree.find(x) == m_flat_tree.end() ? 0 : 1;  }

   const_iterator lower_bound(const key_type& x) const 
      {  return m_flat_tree.lower_bound(x); }

   iterator lower_bound(const key_type& x) 
      {  return m_flat_tree.lower_bound(x); }

   const_iterator upper_bound(const key_type& x) const 
      {  return m_flat_tree.upper_bound(x);    }

   iterator upper_bound(const key_type& x)
      {  return m_flat_tree.upper_bound(x);    }

   std::pair<const_iterator, const_iterator> 
      equal_range(const key_type& x) const 
      {  return m_flat_tree.equal_range(x); }

   std::pair<iterator,iterator> 
      equal_range(const key_type& x) 
      {  return m_flat_tree.equal_range(x); }

   size_type capacity() const           
      { return m_flat_tree.capacity(); }

   void reserve(size_type count)       
      { m_flat_tree.reserve(count);   }

   template <class K1, class C1, class A1>
   friend bool operator== (const flat_set<K1,C1,A1>&, const flat_set<K1,C1,A1>&);

   template <class K1, class C1, class A1>
   friend bool operator< (const flat_set<K1,C1,A1>&, const flat_set<K1,C1,A1>&);
};

template <class T, class Pred, class Alloc>
inline bool operator==(const flat_set<T,Pred,Alloc>& x, 
                       const flat_set<T,Pred,Alloc>& y) 
   {  return x.m_flat_tree == y.m_flat_tree;  }

template <class T, class Pred, class Alloc>
inline bool operator<(const flat_set<T,Pred,Alloc>& x, 
                      const flat_set<T,Pred,Alloc>& y) 
   {  return x.m_flat_tree < y.m_flat_tree;   }

template <class T, class Pred, class Alloc>
inline bool operator!=(const flat_set<T,Pred,Alloc>& x, 
                       const flat_set<T,Pred,Alloc>& y) 
   {  return !(x == y);   }

template <class T, class Pred, class Alloc>
inline bool operator>(const flat_set<T,Pred,Alloc>& x, 
                      const flat_set<T,Pred,Alloc>& y) 
   {  return y < x; }

template <class T, class Pred, class Alloc>
inline bool operator<=(const flat_set<T,Pred,Alloc>& x, 
                       const flat_set<T,Pred,Alloc>& y) 
   {  return !(y < x); }

template <class T, class Pred, class Alloc>
inline bool operator>=(const flat_set<T,Pred,Alloc>& x, 
                       const flat_set<T,Pred,Alloc>& y) 
   {  return !(x < y);  }

template <class T, class Pred, class Alloc>
inline void swap(flat_set<T,Pred,Alloc>& x, 
                 flat_set<T,Pred,Alloc>& y) 
   {  x.swap(y);  }

template <class T, class Pred, class Alloc>
inline void swap(const detail::moved_object<flat_set<T,Pred,Alloc> >& x, 
                 flat_set<T,Pred,Alloc>& y) 
   {  x.get().swap(y);  }

template <class T, class Pred, class Alloc>
inline void swap(flat_set<T,Pred,Alloc>& x, 
                 const detail::moved_object<flat_set<T,Pred,Alloc> >& y) 
   {  x.swap(y.get());  }

/*!This class is movable*/
template <class T, class P, class A>
struct is_movable<flat_set<T, P, A> >
{
   enum {   value = true };
};

// Forward declaration of operators < and ==, needed for friend declaration.

template <class T, class Pred, class Alloc>
class flat_multiset;

template <class T, class Pred, class Alloc>
inline bool operator==(const flat_multiset<T,Pred,Alloc>& x, 
                       const flat_multiset<T,Pred,Alloc>& y);

template <class T, class Pred, class Alloc>
inline bool operator<(const flat_multiset<T,Pred,Alloc>& x, 
                      const flat_multiset<T,Pred,Alloc>& y);

template <class T, class Pred, class Alloc>
class flat_multiset 
{
 private:
   typedef detail::flat_tree<T, T, 
                     detail::identity<T>, Pred, Alloc> tree_t;
   tree_t m_flat_tree;  // flat tree representing flat_multiset

 public:
   // typedefs:
   typedef typename tree_t::key_type               key_type;
   typedef typename tree_t::value_type             value_type;
   typedef typename tree_t::pointer                pointer;
   typedef typename tree_t::const_pointer          const_pointer;
   typedef typename tree_t::reference              reference;
   typedef typename tree_t::const_reference        const_reference;
   typedef typename tree_t::key_compare            key_compare;
   typedef typename tree_t::value_compare          value_compare;
   typedef typename tree_t::iterator               iterator;
   typedef typename tree_t::const_iterator         const_iterator;
   typedef typename tree_t::reverse_iterator       reverse_iterator;
   typedef typename tree_t::const_reverse_iterator const_reverse_iterator;
   typedef typename tree_t::size_type              size_type;
   typedef typename tree_t::difference_type        difference_type;
   typedef typename tree_t::allocator_type         allocator_type;

   // allocation/deallocation
   explicit flat_multiset(const Pred& comp = Pred(),
                          const allocator_type& a = allocator_type())
      : m_flat_tree(comp, a) {}

   template <class InputIterator>
   flat_multiset(InputIterator first, InputIterator last,
                 const Pred& comp        = Pred(),
                 const allocator_type& a = allocator_type())
      : m_flat_tree(comp, a) 
      { m_flat_tree.insert_equal(first, last); }

   flat_multiset(const flat_multiset<T,Pred,Alloc>& x) 
      : m_flat_tree(x.m_flat_tree) {}

   flat_multiset(const detail::moved_object<flat_multiset<T,Pred,Alloc> >& x) 
      : m_flat_tree(move(x.get().m_flat_tree)) {}

   flat_multiset<T,Pred,Alloc>& operator=(const flat_multiset<T,Pred,Alloc>& x) 
      {  m_flat_tree = x.m_flat_tree;   return *this;  }

   flat_multiset<T,Pred,Alloc>& operator=(const detail::moved_object<flat_multiset<T,Pred,Alloc> >& x) 
      {  m_flat_tree.swap(x.get().m_flat_tree);   return *this;  }

   // accessors:

   key_compare key_comp() const 
      { return m_flat_tree.key_comp(); }

   value_compare value_comp() const 
      { return m_flat_tree.key_comp(); }

   allocator_type get_allocator() const 
      { return m_flat_tree.get_allocator(); }

   const_iterator begin() const 
      { return m_flat_tree.begin(); }

   const_iterator end() const 
      { return m_flat_tree.end(); }

   const_reverse_iterator rbegin() const 
      { return m_flat_tree.rbegin(); } 

   const_reverse_iterator rend() const 
      { return m_flat_tree.rend(); }

   iterator begin() 
      { return m_flat_tree.begin(); }

   iterator end() 
      { return m_flat_tree.end(); }

   reverse_iterator rbegin() 
      { return m_flat_tree.rbegin(); } 

   reverse_iterator rend() 
      { return m_flat_tree.rend(); }

   bool empty() const 
      { return m_flat_tree.empty(); }

   size_type size() const 
      { return m_flat_tree.size(); }

   size_type max_size() const 
      { return m_flat_tree.max_size(); }

   void swap(flat_multiset<T,Pred,Alloc>& x) 
      { m_flat_tree.swap(x.m_flat_tree); }

   void swap(const detail::moved_object<flat_multiset<T,Pred,Alloc> >& mx) 
      { this->swap(mx.get()); }

   // insert/erase
   iterator insert(const value_type& x) 
      {  return m_flat_tree.insert_equal(x);   }

   iterator insert(const detail::moved_object<value_type>& x) 
      {  return m_flat_tree.insert_equal(x);   }

   iterator insert(iterator position, const value_type& x) 
      {  return m_flat_tree.insert_equal(position, x);  }

   iterator insert(iterator position, const detail::moved_object<value_type>& x) 
      {  return m_flat_tree.insert_equal(position, x);  }

   template <class InputIterator>
   void insert(InputIterator first, InputIterator last) 
      {  m_flat_tree.insert_equal(first, last);  }

   iterator erase(const_iterator position) 
      {  return m_flat_tree.erase(position); }

   size_type erase(const key_type& x) 
      {  return m_flat_tree.erase(x); }

   iterator erase(const_iterator first, const_iterator last)
      {  return m_flat_tree.erase(first, last); }

   void clear() 
      { m_flat_tree.clear(); }

   // flat_multiset operations:
   const_iterator find(const key_type& x) const 
      { return m_flat_tree.find(x); }

   iterator find(const key_type& x) 
      { return m_flat_tree.find(x); }

   size_type count(const key_type& x) const 
      {  return m_flat_tree.count(x);  }

   const_iterator lower_bound(const key_type& x) const 
      {  return m_flat_tree.lower_bound(x); }

   iterator lower_bound(const key_type& x) 
      {  return m_flat_tree.lower_bound(x); }

   const_iterator upper_bound(const key_type& x) const 
      {  return m_flat_tree.upper_bound(x);    }

   iterator upper_bound(const key_type& x)
      {  return m_flat_tree.upper_bound(x);    }

   std::pair<const_iterator, const_iterator> 
      equal_range(const key_type& x) const 
      {  return m_flat_tree.equal_range(x); }

   std::pair<iterator,iterator> 
      equal_range(const key_type& x) 
      {  return m_flat_tree.equal_range(x); }

   size_type capacity() const           
      { return m_flat_tree.capacity(); }

   void reserve(size_type count)       
      { m_flat_tree.reserve(count);   }

   template <class K1, class C1, class A1>
   friend bool operator== (const flat_multiset<K1,C1,A1>&,
                           const flat_multiset<K1,C1,A1>&);
   template <class K1, class C1, class A1>
   friend bool operator< (const flat_multiset<K1,C1,A1>&,
                          const flat_multiset<K1,C1,A1>&);
};

template <class T, class Pred, class Alloc>
inline bool operator==(const flat_multiset<T,Pred,Alloc>& x, 
                       const flat_multiset<T,Pred,Alloc>& y) 
   {  return x.m_flat_tree == y.m_flat_tree;  }

template <class T, class Pred, class Alloc>
inline bool operator<(const flat_multiset<T,Pred,Alloc>& x, 
                      const flat_multiset<T,Pred,Alloc>& y) 
   {  return x.m_flat_tree < y.m_flat_tree;   }

template <class T, class Pred, class Alloc>
inline bool operator!=(const flat_multiset<T,Pred,Alloc>& x, 
                       const flat_multiset<T,Pred,Alloc>& y) 
   {  return !(x == y);  }

template <class T, class Pred, class Alloc>
inline bool operator>(const flat_multiset<T,Pred,Alloc>& x, 
                      const flat_multiset<T,Pred,Alloc>& y) 
   {  return y < x;  }

template <class T, class Pred, class Alloc>
inline bool operator<=(const flat_multiset<T,Pred,Alloc>& x, 
                       const flat_multiset<T,Pred,Alloc>& y) 
   {  return !(y < x);  }

template <class T, class Pred, class Alloc>
inline bool operator>=(const flat_multiset<T,Pred,Alloc>& x, 
                       const flat_multiset<T,Pred,Alloc>& y) 
{  return !(x < y);  }

template <class T, class Pred, class Alloc>
inline void swap(flat_multiset<T,Pred,Alloc>& x, 
                 flat_multiset<T,Pred,Alloc>& y) 
   {  x.swap(y);  }

template <class T, class Pred, class Alloc>
inline void swap(const detail::moved_object<flat_multiset<T,Pred,Alloc> >& x, 
                 flat_multiset<T,Pred,Alloc>& y) 
   {  x.get().swap(y);  }

template <class T, class Pred, class Alloc>
inline void swap(flat_multiset<T,Pred,Alloc>& x, 
                 const detail::moved_object<flat_multiset<T,Pred,Alloc> >& y) 
   {  x.swap(y.get());  }

/*!This class is movable*/
template <class T, class P, class A>
struct is_movable<flat_multiset<T, P, A> >
{
   enum {   value = true };
};

}} //namespace boost {   namespace interprocess {

#include <boost/interprocess/detail/config_end.hpp>

#endif /* BOOST_INTERPROCESS_SHMEM_FLAT_SET_HPP */

// Local Variables:
// mode:C++
// End:
