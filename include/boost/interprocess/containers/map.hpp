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
// This file comes from SGI's stl_map/stl_multimap files. Modified by Ion Gaztañaga 2004.
// Renaming, isolating and porting to generic algorithms. Pointer typedef 
// set to allocator::pointer to allow placing it in shared memory.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_SHMEM_MAP_HPP
#define BOOST_INTERPROCESS_SHMEM_MAP_HPP

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
#include <boost/interprocess/detail/move.hpp>

namespace boost { namespace interprocess {

// Forward declarations of operators == and <, needed for friend declarations.
template <class Key, class T, class Pred, class Alloc>
inline bool operator==(const map<Key,T,Pred,Alloc>& x, 
                       const map<Key,T,Pred,Alloc>& y);

template <class Key, class T, class Pred, class Alloc>
inline bool operator<(const map<Key,T,Pred,Alloc>& x, 
                      const map<Key,T,Pred,Alloc>& y);

template <class Key, class T, class Pred, class Alloc>
class map 
{
 private:
   typedef detail::rb_tree<Key, 
                           std::pair<const Key, T>, 
                           detail::select1st< std::pair<const Key, T> >, 
                           Pred, 
                           Alloc> tree_t;
   tree_t m_tree;  // red-black tree representing map

 public:
   // typedefs:
   typedef typename tree_t::key_type               key_type;
   typedef typename tree_t::value_type             value_type;
   typedef typename tree_t::pointer                pointer;
   typedef typename tree_t::const_pointer          const_pointer;
   typedef typename tree_t::reference              reference;
   typedef typename tree_t::const_reference        const_reference;
   typedef T                                       mapped_type;
   typedef Pred                                    key_compare;
   typedef typename tree_t::iterator               iterator;
   typedef typename tree_t::const_iterator         const_iterator;
   typedef typename tree_t::reverse_iterator       reverse_iterator;
   typedef typename tree_t::const_reverse_iterator const_reverse_iterator;
   typedef typename tree_t::size_type              size_type;
   typedef typename tree_t::difference_type        difference_type;
   typedef typename tree_t::allocator_type         allocator_type;

   class value_compare 
      :  public Pred,
         public std::binary_function<value_type, value_type, bool> 
   {
      friend class map<Key,T,Pred,Alloc>;
    protected :
      value_compare(const Pred &c) : Pred(c) {}
    public:
      bool operator()(const value_type& x, const value_type& y) const {
         return Pred::operator()(x.first, y.first);
      }
   };

   // allocation/deallocation

   map() : m_tree(Pred(), allocator_type()) {}

   explicit map(const Pred& comp,
                const allocator_type& a = allocator_type())
      : m_tree(comp, a) {}

   template <class InputIterator>
   map(InputIterator first, InputIterator last)
      : m_tree(Pred(), allocator_type())
      { m_tree.insert_unique(first, last); }

   template <class InputIterator>
   map(InputIterator first, InputIterator last, const Pred& comp,
         const allocator_type& a = allocator_type())
      : m_tree(comp, a) 
      { m_tree.insert_unique(first, last); }

   map(const map<Key,T,Pred,Alloc>& x) 
      : m_tree(x.m_tree) {}

   map(const detail::moved_object<map<Key,T,Pred,Alloc> >& x) 
      : m_tree(move(x.get().m_tree)) {}

   map<Key,T,Pred,Alloc>& operator=(const map<Key, T, Pred, Alloc>& x)
      {  m_tree = x.m_tree;   return *this;  }

   map<Key,T,Pred,Alloc>& operator=(const detail::moved_object<map<Key,T,Pred,Alloc> >& x)
      {  m_tree.swap(x.get().m_tree);   return *this;  }

   // accessors:

   key_compare key_comp() const 
      { return m_tree.key_comp(); }

   value_compare value_comp() const 
      { return value_compare(m_tree.key_comp()); }

   allocator_type get_allocator() const 
      { return m_tree.get_allocator(); }

   iterator begin() 
      { return m_tree.begin(); }

   const_iterator begin() const 
      { return m_tree.begin(); }

   iterator end() 
      { return m_tree.end(); }

   const_iterator end() const 
      { return m_tree.end(); }

   reverse_iterator rbegin() 
      { return m_tree.rbegin(); }

   const_reverse_iterator rbegin() const 
      { return m_tree.rbegin(); }

   reverse_iterator rend() 
      { return m_tree.rend(); }

   const_reverse_iterator rend() const 
      { return m_tree.rend(); }

   bool empty() const 
      { return m_tree.empty(); }

   size_type size() const 
      { return m_tree.size(); }

   size_type max_size() const 
      { return m_tree.max_size(); }

   T& operator[](const key_type& k) 
   {
      //we can optimize this
      iterator i = lower_bound(k);
      // i->first is greater than or equivalent to k.
      if (i == end() || key_comp()(k, (*i).first))
         i = insert(i, value_type(k, T()));
      return (*i).second;
   }

   T& operator[](const detail::moved_object<key_type>& mk) 
   {
      key_type &k = mk.get();
      //we can optimize this
      iterator i = lower_bound(k);
      // i->first is greater than or equivalent to k.
      if (i == end() || key_comp()(k, (*i).first))
         i = insert(i, value_type(k, move(T())));
      return (*i).second;
   }

   void swap(map<Key,T,Pred,Alloc>& x) 
      { m_tree.swap(x.m_tree); }

   void swap(const detail::moved_object<map<Key,T,Pred,Alloc> >& x) 
      { m_tree.swap(x.get().m_tree); }

   // insert/erase

   std::pair<iterator,bool> insert(const value_type& x) 
      { return m_tree.insert_unique(x); }

   std::pair<iterator,bool> insert(const std::pair<key_type, mapped_type>& x) 
      { return m_tree.insert_unique(x); }

   std::pair<iterator,bool> insert(const detail::moved_object<std::pair<key_type, mapped_type> > &x) 
      { return m_tree.insert_unique(x); }
/*
   std::pair<iterator,bool> insert(const detail::moved_object<value_type>& x) 
      { return m_tree.insert_unique(x); }
*/
/*
   std::pair<iterator,bool> insert_from(const key_type& k,
                                        const mapped_type& m)
      {  return m_tree.insert_unique(value_type(k, m));  }      
*/
   iterator insert(iterator position, const value_type& x)
      { return m_tree.insert_unique(position, x); }

   iterator insert(iterator position, const detail::moved_object<std::pair<key_type, mapped_type> > &x)
      { return m_tree.insert_unique(position, x); }

   iterator insert(iterator position, const std::pair<key_type, mapped_type>& x)
      { return m_tree.insert_unique(position, x); }
/*
   iterator insert(iterator position, const detail::moved_object<value_type>& x)
      { return m_tree.insert_unique(position, x); }
*/
/*
   iterator insert_from(iterator position,
                        const key_type& k,
                        const mapped_type& m)
      { return m_tree.insert_unique(position, value_type(k, m)); }
*/
   template <class InputIterator>
   void insert(InputIterator first, InputIterator last) 
      {  m_tree.insert_unique(first, last);  }

   iterator erase(const_iterator position) 
      { return m_tree.erase(position); }

   size_type erase(const key_type& x) 
      { return m_tree.erase(x); }

   iterator erase(const_iterator first, const_iterator last)
      { return m_tree.erase(first, last); }

   void clear() 
      { m_tree.clear(); }

   // map operations:

   iterator find(const key_type& x) 
      { return m_tree.find(x); }

   const_iterator find(const key_type& x) const 
      { return m_tree.find(x); }

   size_type count(const key_type& x) const 
      {  return m_tree.find(x) == m_tree.end() ? 0 : 1;  }

   iterator lower_bound(const key_type& x) 
      {  return m_tree.lower_bound(x); }

   const_iterator lower_bound(const key_type& x) const 
      {  return m_tree.lower_bound(x); }

   iterator upper_bound(const key_type& x) 
      {  return m_tree.upper_bound(x); }

   const_iterator upper_bound(const key_type& x) const 
      {  return m_tree.upper_bound(x); }
     
   std::pair<iterator,iterator> equal_range(const key_type& x) 
      {  return m_tree.equal_range(x); }

   std::pair<const_iterator,const_iterator> equal_range(const key_type& x) const 
      {  return m_tree.equal_range(x); }

   template <class K1, class T1, class C1, class A1>
   friend bool operator== (const map<K1, T1, C1, A1>&,
                           const map<K1, T1, C1, A1>&);
   template <class K1, class T1, class C1, class A1>
   friend bool operator< (const map<K1, T1, C1, A1>&,
                           const map<K1, T1, C1, A1>&);
};

template <class Key, class T, class Pred, class Alloc>
inline bool operator==(const map<Key,T,Pred,Alloc>& x, 
                       const map<Key,T,Pred,Alloc>& y) 
   {  return x.m_tree == y.m_tree;  }

template <class Key, class T, class Pred, class Alloc>
inline bool operator<(const map<Key,T,Pred,Alloc>& x, 
                      const map<Key,T,Pred,Alloc>& y) 
   {  return x.m_tree < y.m_tree;   }

template <class Key, class T, class Pred, class Alloc>
inline bool operator!=(const map<Key,T,Pred,Alloc>& x, 
                       const map<Key,T,Pred,Alloc>& y) 
   {  return !(x == y); }

template <class Key, class T, class Pred, class Alloc>
inline bool operator>(const map<Key,T,Pred,Alloc>& x, 
                      const map<Key,T,Pred,Alloc>& y) 
   {  return y < x;  }

template <class Key, class T, class Pred, class Alloc>
inline bool operator<=(const map<Key,T,Pred,Alloc>& x, 
                       const map<Key,T,Pred,Alloc>& y) 
   {  return !(y < x);  }

template <class Key, class T, class Pred, class Alloc>
inline bool operator>=(const map<Key,T,Pred,Alloc>& x, 
                       const map<Key,T,Pred,Alloc>& y) 
   {  return !(x < y);  }

template <class Key, class T, class Pred, class Alloc>
inline void swap(map<Key,T,Pred,Alloc>& x, 
                 map<Key,T,Pred,Alloc>& y) 
   {  x.swap(y);  }

template <class Key, class T, class Pred, class Alloc>
inline void swap(const detail::moved_object<map<Key,T,Pred,Alloc> >& x, 
                 map<Key,T,Pred,Alloc>& y) 
   {  x.get().swap(y);  }

template <class Key, class T, class Pred, class Alloc>
inline void swap(map<Key,T,Pred,Alloc>& x, 
                 const detail::moved_object<map<Key,T,Pred,Alloc> >& y) 
   {  x.swap(y.get());  }


/*!This class is movable*/
template <class T, class P, class A>
struct is_movable<map<T, P, A> >
{
   enum {   value = true };
};

// Forward declaration of operators < and ==, needed for friend declaration.

template <class Key, class T, class Pred, class Alloc>
inline bool operator==(const multimap<Key,T,Pred,Alloc>& x, 
                       const multimap<Key,T,Pred,Alloc>& y);

template <class Key, class T, class Pred, class Alloc>
inline bool operator<(const multimap<Key,T,Pred,Alloc>& x, 
                      const multimap<Key,T,Pred,Alloc>& y);

template <class Key, class T, class Pred, class Alloc>
class multimap 
{
 private:
   typedef detail::rb_tree<Key, 
                           std::pair<const Key, T>, 
                           detail::select1st< std::pair<const Key, T> >, 
                           Pred, 
                           Alloc> tree_t;
   tree_t m_tree;  // red-black tree representing map

 public:
   // typedefs:
   typedef typename tree_t::key_type               key_type;
   typedef typename tree_t::value_type             value_type;
   typedef typename tree_t::pointer                pointer;
   typedef typename tree_t::const_pointer          const_pointer;
   typedef typename tree_t::reference              reference;
   typedef typename tree_t::const_reference        const_reference;
   typedef T                                       mapped_type;
   typedef Pred                                    key_compare;
   typedef typename tree_t::iterator               iterator;
   typedef typename tree_t::const_iterator         const_iterator;
   typedef typename tree_t::reverse_iterator       reverse_iterator;
   typedef typename tree_t::const_reverse_iterator const_reverse_iterator;
   typedef typename tree_t::size_type              size_type;
   typedef typename tree_t::difference_type        difference_type;
   typedef typename tree_t::allocator_type         allocator_type;

   class value_compare 
      :  public Pred,
         public std::binary_function<value_type, value_type, bool> 
   {
      friend class multimap<Key,T,Pred,Alloc>;
    protected :
      value_compare(const Pred &c) : Pred(c) {}
    public:
      bool operator()(const value_type& x, const value_type& y) const {
         return Pred::operator()(x.first, y.first);
      }
   };

   // allocation/deallocation
   explicit multimap(const Pred& comp = Pred(),
                     const allocator_type& a = allocator_type())
      : m_tree(comp, a) { }

   template <class InputIterator>
   multimap(InputIterator first, InputIterator last)
      : m_tree(Pred(), allocator_type())
      { m_tree.insert_equal(first, last); }

   template <class InputIterator>
   multimap(InputIterator first, InputIterator last,
            const Pred& comp,
            const allocator_type& a = allocator_type())
      : m_tree(comp, a) 
      { m_tree.insert_equal(first, last); }

   multimap(const multimap<Key,T,Pred,Alloc>& x) 
      : m_tree(x.m_tree) { }

   multimap(const detail::moved_object<multimap<Key,T,Pred,Alloc> >& x) 
      : m_tree(move(x.get().m_tree)) { }

   multimap<Key,T,Pred,Alloc>&
   operator=(const multimap<Key,T,Pred,Alloc>& x) 
      {  m_tree = x.m_tree;   return *this;  }

   multimap<Key,T,Pred,Alloc>&
   operator=(const detail::moved_object<multimap<Key,T,Pred,Alloc> >& x) 
      {  m_tree.swap(move(x.get().m_tree));   return *this;  }

   // accessors:

   key_compare key_comp() const 
      { return m_tree.key_comp(); }

   value_compare value_comp() const 
      { return value_compare(m_tree.key_comp()); }

   allocator_type get_allocator() const 
      { return m_tree.get_allocator(); }

   iterator begin() 
      { return m_tree.begin(); }

   const_iterator begin() const 
      { return m_tree.begin(); }

   iterator end() 
      { return m_tree.end(); }

   const_iterator end() const 
      { return m_tree.end(); }

   reverse_iterator rbegin() 
      { return m_tree.rbegin(); }

   const_reverse_iterator rbegin() const 
      { return m_tree.rbegin(); }

   reverse_iterator rend() 
      { return m_tree.rend(); }

   const_reverse_iterator rend() const 
      { return m_tree.rend(); }

   bool empty() const 
      { return m_tree.empty(); }

   size_type size() const 
      { return m_tree.size(); }

   size_type max_size() const 
      { return m_tree.max_size(); }

   void swap(multimap<Key,T,Pred,Alloc>& x) 
      { m_tree.swap(x.m_tree); }

   void swap(const detail::moved_object<multimap<Key,T,Pred,Alloc> >& x) 
      { m_tree.swap(x.get().m_tree); }

   // insert/erase

   iterator insert(const value_type& x) 
      { return m_tree.insert_equal(x); }

   iterator insert(const std::pair<key_type, mapped_type>& x) 
      { return m_tree.insert_equal(x); }

   iterator insert(const detail::moved_object<std::pair<key_type, mapped_type> >& x) 
      { return m_tree.insert_equal(x); }
/*
   iterator insert_from(const key_type    &k,
                        const mapped_type &m)
      {  return m_tree.insert_equal(value_type(k, m));  }
*/

   iterator insert(iterator position, const value_type& x)
      { return m_tree.insert_equal(position, x); }

   iterator insert(iterator position, const std::pair<key_type, mapped_type>& x)
      { return m_tree.insert_equal(position, x); }

   iterator insert(iterator position, const detail::moved_object<std::pair<key_type, mapped_type> >& x)
      { return m_tree.insert_equal(position, x); }

/*
   iterator insert_from(iterator position,
                        const key_type    &k,
                        const mapped_type &m)
      {  return m_tree.insert_equal(position, value_type(k, m));  }
*/

   template <class InputIterator>
   void insert(InputIterator first, InputIterator last) 
      {  m_tree.insert_equal(first, last); }

   iterator erase(const_iterator position) 
      { return m_tree.erase(position); }

   size_type erase(const key_type& x) 
      { return m_tree.erase(x); }

   iterator erase(const_iterator first, const_iterator last)
      { return m_tree.erase(first, last); }

   void clear() 
      { m_tree.clear(); }

   // multimap operations:

   iterator find(const key_type& x) 
      { return m_tree.find(x); }

   const_iterator find(const key_type& x) const 
      { return m_tree.find(x); }

   size_type count(const key_type& x) const 
      { return m_tree.count(x); }

   iterator lower_bound(const key_type& x) 
      {return m_tree.lower_bound(x); }

   const_iterator lower_bound(const key_type& x) const 
      {  return m_tree.lower_bound(x);  }

   iterator upper_bound(const key_type& x) 
      {return m_tree.upper_bound(x); }

   const_iterator upper_bound(const key_type& x) const 
      {  return m_tree.upper_bound(x); }

   std::pair<iterator,iterator> equal_range(const key_type& x) 
      {  return m_tree.equal_range(x);   }

   std::pair<const_iterator,const_iterator> 
      equal_range(const key_type& x) const 
      {  return m_tree.equal_range(x);   }

   template <class K1, class T1, class C1, class A1>
   friend bool operator== (const multimap<K1, T1, C1, A1>& x,
                           const multimap<K1, T1, C1, A1>& y);

   template <class K1, class T1, class C1, class A1>
   friend bool operator< (const multimap<K1, T1, C1, A1>& x,
                          const multimap<K1, T1, C1, A1>& y);
};

template <class Key, class T, class Pred, class Alloc>
inline bool operator==(const multimap<Key,T,Pred,Alloc>& x, 
                       const multimap<Key,T,Pred,Alloc>& y) 
   {  return x.m_tree == y.m_tree;  }

template <class Key, class T, class Pred, class Alloc>
inline bool operator<(const multimap<Key,T,Pred,Alloc>& x, 
                      const multimap<Key,T,Pred,Alloc>& y) 
   {  return x.m_tree < y.m_tree;   }

template <class Key, class T, class Pred, class Alloc>
inline bool operator!=(const multimap<Key,T,Pred,Alloc>& x, 
                       const multimap<Key,T,Pred,Alloc>& y) 
   {  return !(x == y);  }

template <class Key, class T, class Pred, class Alloc>
inline bool operator>(const multimap<Key,T,Pred,Alloc>& x, 
                      const multimap<Key,T,Pred,Alloc>& y) 
   {  return y < x;  }

template <class Key, class T, class Pred, class Alloc>
inline bool operator<=(const multimap<Key,T,Pred,Alloc>& x, 
                       const multimap<Key,T,Pred,Alloc>& y) 
   {  return !(y < x);  }

template <class Key, class T, class Pred, class Alloc>
inline bool operator>=(const multimap<Key,T,Pred,Alloc>& x, 
                       const multimap<Key,T,Pred,Alloc>& y) 
   {  return !(x < y);  }

template <class Key, class T, class Pred, class Alloc>
inline void swap(multimap<Key,T,Pred,Alloc>& x, 
                 multimap<Key,T,Pred,Alloc>& y) 
   {  x.swap(y);  }

template <class Key, class T, class Pred, class Alloc>
inline void swap(const detail::moved_object<multimap<Key,T,Pred,Alloc> >& x, 
                 multimap<Key,T,Pred,Alloc>& y) 
   {  x.get().swap(y);  }

template <class Key, class T, class Pred, class Alloc>
inline void swap(multimap<Key,T,Pred,Alloc>& x, 
                 const detail::moved_object<multimap<Key,T,Pred,Alloc> >& y) 
   {  x.swap(y.get());  }

template <class T, class P, class A>
struct is_movable<multimap<T, P, A> >
{
   enum {   value = true };
};

}} //namespace boost { namespace interprocess {

#include <boost/interprocess/detail/config_end.hpp>

#endif /* BOOST_INTERPROCESS_SHMEM_MAP_HPP */

