//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2004-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
//
// This file comes from SGI's stl_slist.h file. Modified by Ion Gaztañaga 2004-2007
// Renaming, isolating and porting to generic algorithms. Pointer typedef 
// set to allocator::pointer to allow placing it in shared memory.
//
///////////////////////////////////////////////////////////////////////////////
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

#ifndef BOOST_INTERPROCESS_SLIST_HPP
#define BOOST_INTERPROCESS_SLIST_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/iterator/reverse_iterator.hpp>
#include <boost/iterator.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/detail/allocator_utilities.hpp>
#include <boost/interprocess/detail/move.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/detail/no_exceptions_support.hpp>
#include <boost/intrusive/slist.hpp>
#include <boost/type_traits/has_trivial_destructor.hpp>

#include <iterator>
#include <utility>
#include <memory>
#include <functional>
#include <algorithm>

namespace boost{  namespace interprocess{

namespace detail {
/// @cond
template <class T, class VoidPointer>
struct slist_node
   :  public boost::intrusive::slist_base_hook
         < boost::intrusive::tag
         , boost::intrusive::safe_link
         , VoidPointer>
{
   typedef boost::intrusive::slist_base_hook
         < boost::intrusive::tag
         , boost::intrusive::safe_link
         , VoidPointer>                IslistData;

   template<class Convertible>
   slist_node(const Convertible &value)
      : m_data(value){}

   T m_data;
};

template<class A>
struct slist_alloc
   :  public boost::detail::allocator::
         rebind_to<A, slist_node
            <typename A::value_type, typename boost::detail::allocator::
               rebind_to<A, void>::type::pointer>
            >::type
{
   typedef slist_alloc<A>                             self_t;
   typedef typename A::value_type                     value_type;
   typedef slist_node
      <value_type, typename boost::detail::allocator::
         rebind_to<A, void>::type::pointer>           Node;
   typedef typename boost::detail::allocator::
      rebind_to<A, Node>::type                        NodeAlloc;
   typedef A                                          ValAlloc;
   typedef typename A::size_type                      SizeType;
   typedef typename NodeAlloc::pointer                NodePtr;
   typedef detail::scoped_deallocator<NodeAlloc>      Deallocator;

   enum {   
      node_has_trivial_destructor =  
         boost::has_trivial_destructor<NodePtr>::value &&
         boost::has_trivial_destructor<value_type>::value 
   };

   slist_alloc(const ValAlloc &a) 
      : NodeAlloc(a)
   {}

   slist_alloc(const detail::moved_object<ValAlloc> &a) 
      : NodeAlloc(a.get())
   {}

   slist_alloc(const slist_alloc &other)
      : NodeAlloc(other)
   {}

   slist_alloc(const detail::moved_object<slist_alloc> &other)
      : NodeAlloc(move((NodeAlloc&)other.get()))
   {  this->swap(other.get());  }

   ~slist_alloc()
   {}

   typename NodeAlloc::size_type max_size() const
   {  return NodeAlloc::max_size();  }

   template<class Convertible>
   static void construct(const NodePtr &ptr, const Convertible &value)
   {  new(detail::get_pointer(ptr)) Node(value);  }

   static void destroy(const NodePtr &ptr)
   {  detail::get_pointer(ptr)->~Node();  }

   template<class Convertible>
   NodePtr create_node(const Convertible& x)
   {
      NodePtr p = NodeAlloc::allocate(1);
      scoped_ptr<Node, Deallocator>node_deallocator(p, *this);
      self_t::construct(p, x);
      node_deallocator.release();
      return (p);
   }

   void destroy_node(NodePtr node)
   {
      if(!node_has_trivial_destructor){
         self_t::destroy(node);
      }
      NodeAlloc::deallocate(node, 1);
   }

   void swap(slist_alloc &x)
   {
      NodeAlloc& this_alloc   = static_cast<NodeAlloc&>(*this);
      NodeAlloc& other_alloc  = static_cast<NodeAlloc&>(x);

      if (this_alloc != other_alloc){
         detail::do_swap(this_alloc, other_alloc);
      }

      this->m_islist.swap(x.m_islist);
   }

   protected:
   //The intrusive slist
   typedef typename boost::intrusive::slist
      < typename Node::IslistData::template value_traits<Node>
      , true
      , typename A::size_type> Islist;
   Islist m_islist;

   Islist &non_const_islist() const
   {  return const_cast<Islist&>(this->m_islist);   }
};
/// @endcond
}  //namespace detail {

//! An slist is a singly linked list: a list where each element is linked to the next 
//! element, but not to the previous element. That is, it is a Sequence that 
//! supports forward but not backward traversal, and (amortized) constant time 
//! insertion and removal of elements. Slists, like lists, have the important 
//! property that insertion and splicing do not invalidate iterators to list elements, 
//! and that even removal invalidates only the iterators that point to the elements 
//! that are removed. The ordering of iterators may be changed (that is, 
//! slist<T>::iterator might have a different predecessor or successor after a list 
//! operation than it did before), but the iterators themselves will not be invalidated 
//! or made to point to different elements unless that invalidation or mutation is explicit.
//!
//! The main difference between slist and list is that list's iterators are bidirectional 
//! iterators, while slist's iterators are forward iterators. This means that slist is 
//! less versatile than list; frequently, however, bidirectional iterators are 
//! unnecessary. You should usually use slist unless you actually need the extra 
//! functionality of list, because singly linked lists are smaller and faster than double 
//! linked lists. 
//! 
//! Important performance note: like every other Sequence, slist defines the member 
//! functions insert and erase. Using these member functions carelessly, however, can 
//! result in disastrously slow programs. The problem is that insert's first argument is 
//! an iterator p, and that it inserts the new element(s) before p. This means that 
//! insert must find the iterator just before p; this is a constant-time operation 
//! for list, since list has bidirectional iterators, but for slist it must find that 
//! iterator by traversing the list from the beginning up to p. In other words: 
//! insert and erase are slow operations anywhere but near the beginning of the slist.
//! 
//! Slist provides the member functions insert_after and erase_after, which are constant 
//! time operations: you should always use insert_after and erase_after whenever 
//! possible. If you find that insert_after and erase_after aren't adequate for your 
//! needs, and that you often need to use insert and erase in the middle of the list, 
//! then you should probably use list instead of slist.
template <class T, class A>
class slist 
   : protected detail::slist_alloc<A>
{
   /// @cond
   typedef detail::slist_alloc<A>                     AllocHolder;
   typedef typename AllocHolder::NodePtr              NodePtr;
   typedef slist <T, A>                               ThisType;
   typedef typename AllocHolder::NodeAlloc            NodeAlloc;
   typedef typename AllocHolder::ValAlloc             ValAlloc;
   typedef typename AllocHolder::Node                 Node;
   typedef typename AllocHolder::Islist               Islist;
   typedef detail::allocator_destroyer<NodeAlloc>     Destroyer;

   class equal_to_value
   {
      typedef typename AllocHolder::value_type value_type;
      const value_type &t_;

      public:
      equal_to_value(const value_type &t)
         :  t_(t)
      {}

      bool operator()(const value_type &t)const
      {  return t_ == t;   }
   };

   template<class Pred>
   struct ValueCompareToNodeCompare
      :  Pred
   {
      ValueCompareToNodeCompare(Pred pred)
         :  Pred(pred)
      {}

      bool operator()(const Node &a, const Node &b) const
      {  return static_cast<const Pred&>(*this)(a.m_data, b.m_data);  }

      bool operator()(const Node &a) const
      {  return static_cast<const Pred&>(*this)(a.m_data);  }
   };
   /// @endcond
   public:
   //! The type of object, T, stored in the list
   typedef T                                       value_type;
   //! Pointer to T
   typedef typename A::pointer                     pointer;
   //! Const pointer to T
   typedef typename A::const_pointer               const_pointer;
   //! Reference to T
   typedef typename A::reference                   reference;
   //! Const reference to T
   typedef typename A::const_reference             const_reference;
   //! An unsigned integral type
   typedef typename A::size_type                   size_type;
   //! A signed integral type
   typedef typename A::difference_type             difference_type;
   //! The allocator type
   typedef A                                       allocator_type;

   /// @cond
   private:
   typedef difference_type                         list_difference_type;
   typedef pointer                                 list_pointer;
   typedef const_pointer                           list_const_pointer;
   typedef reference                               list_reference;
   typedef const_reference                         list_const_reference;
   /// @endcond

   public:
   //! Const iterator used to iterate through a list. 
   class const_iterator
      : public boost::iterator<std::forward_iterator_tag, 
                                 value_type,         list_difference_type, 
                                 list_const_pointer, list_const_reference>
   {
      /// @cond
      protected:
      typename Islist::iterator m_it;
      explicit const_iterator(typename Islist::iterator it)  : m_it(it){}
      void prot_incr(){ ++m_it; }

      private:
      typename Islist::iterator get()
      {  return this->m_it;   }
      /// @endcond

      public:
      friend class slist<T, A>;
      typedef list_difference_type        difference_type;

      //Constructors
      const_iterator()
         :  m_it()
      {}

      //Pointer like operators
      const_reference operator*() const 
      { return m_it->m_data;  }

      const_pointer   operator->() const 
      { return  const_pointer(&m_it->m_data); }

      //Increment / Decrement
      const_iterator& operator++()       
      { prot_incr();  return *this; }

      const_iterator operator++(int)      
      { typename Islist::iterator tmp = m_it; ++*this; return const_iterator(tmp);  }

      //Comparison operators
      bool operator==   (const const_iterator& r)  const
      {  return m_it == r.m_it;  }

      bool operator!=   (const const_iterator& r)  const
      {  return m_it != r.m_it;  }
   };

   //! Iterator used to iterate through a list
   class iterator : public const_iterator
   {
      /// @cond
      private:
      explicit iterator(typename Islist::iterator it)
         :  const_iterator(it)
      {}
   
      typename Islist::iterator get()
      {  return this->m_it;   }
      /// @endcond

      public:
      friend class slist<T, A>;
      typedef list_pointer       pointer;
      typedef list_reference     reference;

      //Constructors
      iterator(){}

      //Pointer like operators
      reference operator*()  const {  return  this->m_it->m_data;  }
      pointer   operator->() const {  return  pointer(&this->m_it->m_data);  }

      //Increment / Decrement
      iterator& operator++()  
         { this->prot_incr(); return *this;  }

      iterator operator++(int)
         { typename Islist::iterator tmp = this->m_it; ++*this; return iterator(tmp); }
   };

   public:
   //! <b>Effects</b>: Constructs a list taking the allocator as parameter.
   //! 
   //! <b>Throws</b>: If allocator_type's copy constructor throws.
   //! 
   //! <b>Complexity</b>: Constant.
   explicit slist(const allocator_type& a = allocator_type())
      :  AllocHolder(a)
   {}

//   explicit slist(size_type n)
//      :  AllocHolder(move(allocator_type()))
//   { this->resize(n); }

   //! <b>Effects</b>: Constructs a list that will use a copy of allocator a
   //!   and inserts n copies of value.
   //!
   //! <b>Throws</b>: If allocator_type's default constructor or copy constructor
   //!   throws or T's default or copy constructor throws.
   //! 
   //! <b>Complexity</b>: Linear to n.
   explicit slist(size_type n, const value_type& x = value_type(),
                  const allocator_type& a =  allocator_type())
      :  AllocHolder(a)
   { this->insert_after(this->before_begin(), n, x); }

   //! <b>Effects</b>: Constructs a list that will use a copy of allocator a
   //!   and inserts a copy of the range [first, last) in the list.
   //!
   //! <b>Throws</b>: If allocator_type's default constructor or copy constructor
   //!   throws or T's constructor taking an dereferenced InIt throws.
   //!
   //! <b>Complexity</b>: Linear to the range [first, last).
   template <class InpIt>
   slist(InpIt first, InpIt last,
         const allocator_type& a =  allocator_type()) 
      : AllocHolder(a)
   { this->insert_after_range(this->end_node(), first, last); }

   //! <b>Effects</b>: Copy constructs a list.
   //!
   //! <b>Postcondition</b>: x == *this.
   //! 
   //! <b>Throws</b>: If allocator_type's default constructor or copy constructor throws.
   //! 
   //! <b>Complexity</b>: Linear to the elements x contains.
   slist(const slist& x) 
      : AllocHolder(static_cast<const NodeAlloc&>(x))
   { this->insert_after_range(this->before_begin(), x.begin(), x.end()); }

   //! <b>Effects</b>: Move constructor. Moves mx's resources to *this.
   //!
   //! <b>Throws</b>: If allocator_type's default constructor throws.
   //! 
   //! <b>Complexity</b>: Constant.
   slist(const detail::moved_object<slist> &x)
      : AllocHolder(move((AllocHolder&)x.get()))
   {}

   //! <b>Effects</b>: Makes *this contain the same elements as x.
   //!
   //! <b>Postcondition</b>: this->size() == x.size(). *this contains a copy 
   //! of each of x's elements. 
   //!
   //! <b>Throws</b>: If memory allocation throws or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to the number of elements in x.
   slist& operator= (const slist& x)
   {
      if (&x != this){
         this->assign(x.begin(), x.end());
      }
      return *this;
   }

   //! <b>Effects</b>: Makes *this contain the same elements as x.
   //!
   //! <b>Postcondition</b>: this->size() == x.size(). *this contains a copy 
   //! of each of x's elements. 
   //!
   //! <b>Throws</b>: If memory allocation throws or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to the number of elements in x.
   slist& operator= (const detail::moved_object<slist>& mx)
   {
      if (&mx.get() != this){
         this->clear();
         this->swap(mx.get());
      }
      return *this;
   }

   //! <b>Effects</b>: Destroys the list. All stored values are destroyed
   //!   and used memory is deallocated.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of elements.
   ~slist() 
   {  this->clear(); }

   //! <b>Effects</b>: Returns a copy of the internal allocator.
   //! 
   //! <b>Throws</b>: If allocator's copy constructor throws.
   //! 
   //! <b>Complexity</b>: Constant.
   allocator_type get_allocator() const
   {  return allocator_type(*this); }

   public:

   //! <b>Effects</b>: Assigns the n copies of val to *this.
   //!
   //! <b>Throws</b>: If memory allocation throws or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to n.
   void assign(size_type n, const T& val)
   { this->fill_assign(n, val); }

   //! <b>Effects</b>: Assigns the range [first, last) to *this.
   //!
   //! <b>Throws</b>: If memory allocation throws or
   //!   T's constructor from dereferencing InpIt throws.
   //!
   //! <b>Complexity</b>: Linear to n.
   template <class InpIt>
   void assign(InpIt first, InpIt last) 
   {
      const bool aux_boolean = boost::is_integral<InpIt>::value;
      typedef boost::mpl::bool_<aux_boolean> Result;
      this->assign_dispatch(first, last, Result());
   }

   //! <b>Effects</b>: Returns an iterator to the first element contained in the list.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   iterator begin() 
   { return iterator(this->m_islist.begin()); }

   //! <b>Effects</b>: Returns a const_iterator to the first element contained in the list.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_iterator begin() const 
   {  return const_iterator(this->non_const_islist().begin());   }

   //! <b>Effects</b>: Returns an iterator to the end of the list.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   iterator end()
   { return iterator(this->m_islist.end()); }

   //! <b>Effects</b>: Returns a const_iterator to the end of the list.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_iterator end() const
   {  return const_iterator(this->non_const_islist().end());   }

   //! <b>Effects</b>: Returns a non-dereferenceable iterator that,
   //! when incremented, yields begin().  This iterator may be used
   //! as the argument toinsert_after, erase_after, etc.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   iterator before_begin() 
   {  return iterator(end());  }

   //! <b>Effects</b>: Returns a non-dereferenceable const_iterator 
   //! that, when incremented, yields begin().  This iterator may be used
   //! as the argument toinsert_after, erase_after, etc.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_iterator before_begin() const
   {  return const_iterator(end());  }

   //! <b>Effects</b>: Returns the number of the elements contained in the list.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   size_type size() const 
   {  return this->m_islist.size(); }

   //! <b>Effects</b>: Returns the largest possible size of the list.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   size_type max_size() const 
   {  return AllocHolder::max_size();  }

   //! <b>Effects</b>: Returns true if the list contains no elements.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   bool empty() const 
   {  return !this->size();   }

   //! <b>Effects</b>: Swaps the contents of *this and x.
   //!   If this->allocator_type() != x.allocator_type()
   //!   allocators are also swapped.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of elements on *this and x.
   void swap(slist& x)
   {  AllocHolder::swap(x);   }

   //! <b>Requires</b>: !empty()
   //!
   //! <b>Effects</b>: Returns a reference to the first element 
   //!   from the beginning of the container.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   reference front() 
   {  return *this->begin();  }

   //! <b>Requires</b>: !empty()
   //!
   //! <b>Effects</b>: Returns a const reference to the first element 
   //!   from the beginning of the container.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_reference front() const 
   {  return *this->begin();  }

   //! <b>Effects</b>: Inserts a copy of t in the beginning of the list.
   //!
   //! <b>Throws</b>: If memory allocation throws or
   //!   T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   void push_front(const value_type& x)
   {  this->m_islist.push_front(*this->create_node(x));  }

   //! <b>Effects</b>: Constructs a new element in the beginning of the list
   //!   and moves the resources of t to this new element.
   //!
   //! <b>Throws</b>: If memory allocation throws.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   void push_front(const detail::moved_object<T>& x)
   {  this->m_islist.push_front(*this->create_node(x));  }

   //! <b>Effects</b>: Removes the first element from the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   void pop_front()
   {  this->m_islist.pop_front_and_destroy(Destroyer(*this));      }

   //! <b>Returns</b>: The iterator to the element before i in the sequence. 
   //!   Returns the end-iterator, if either i is the begin-iterator or the 
   //!   sequence is empty. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Linear to the number of elements before i. 
   iterator previous(iterator p) 
   {  return iterator(this->m_islist.previous(p.get())); }

   //! <b>Returns</b>: The const_iterator to the element before i in the sequence. 
   //!   Returns the end-const_iterator, if either i is the begin-const_iterator or 
   //!   the sequence is empty. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Linear to the number of elements before i. 
   const_iterator previous(const_iterator p) 
   {  return const_iterator(this->m_islist.previous(p.get())); }

   //! <b>Requires</b>: p must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Inserts a copy of the value after the p pointed
   //!    by prev_p.
   //!
   //! <b>Returns</b>: An iterator to the inserted element.
   //! 
   //! <b>Throws</b>: If memory allocation throws or T's copy constructor throws.
   //! 
   //! <b>Complexity</b>: Amortized constant time.
   //!
   //! <b>Note</b>: Does not affect the validity of iterators and references of
   //!   previous values.
   iterator insert_after(iterator prev_pos, const value_type& x) 
   {  return iterator(this->m_islist.insert_after(prev_pos.get(), *this->create_node(x))); }

   //! <b>Requires</b>: prev_pos must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Inserts a move constructed copy object from the value after the
   //!    p pointed by prev_pos.
   //!
   //! <b>Returns</b>: An iterator to the inserted element.
   //! 
   //! <b>Throws</b>: If memory allocation throws.
   //! 
   //! <b>Complexity</b>: Amortized constant time.
   //!
   //! <b>Note</b>: Does not affect the validity of iterators and references of
   //!   previous values.
   iterator insert_after(iterator prev_pos, const detail::moved_object<value_type>& x) 
   {  return iterator(this->m_islist.insert_after(prev_pos.get(), *this->create_node(x))); }

   //! <b>Requires</b>: prev_pos must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Inserts n copies of x after prev_pos.
   //!
   //! <b>Throws</b>: If memory allocation throws or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to n.
   //!
   //! <b>Note</b>: Does not affect the validity of iterators and references of
   //!   previous values.
   void insert_after(iterator prev_pos, size_type n, const value_type& x)
   {  this->priv_insert_after_fill(prev_pos, n, x); }

   //! <b>Requires</b>: prev_pos must be a valid iterator of *this.
   //! 
   //! <b>Effects</b>: Inserts the range pointed by [first, last) 
   //!   after the p prev_pos.
   //! 
   //! <b>Throws</b>: If memory allocation throws, T's constructor from a
   //!   dereferenced InpIt throws.
   //! 
   //! <b>Complexity</b>: Linear to the number of elements inserted.
   //! 
   //! <b>Note</b>: Does not affect the validity of iterators and references of
   //!   previous values.
   template <class InIter>
   void insert_after(iterator prev_pos, InIter first, InIter last) 
   {  this->insert_after_range(prev_pos, first, last);  }

   //! <b>Requires</b>: p must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Insert a copy of x before p.
   //!
   //! <b>Throws</b>: If memory allocation throws or x's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to the elements before p.
   iterator insert(iterator p, const value_type& x) 
   {  return insert_after(previous(p), x); }

   //! <b>Requires</b>: p must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Insert a new element before p with mx's resources.
   //!
   //! <b>Throws</b>: If memory allocation throws.
   //!
   //! <b>Complexity</b>: Linear to the elements before p.
   iterator insert(iterator p, const detail::moved_object<value_type>& x) 
   {  return insert_after(previous(p), x); }

   //! <b>Requires</b>: p must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Inserts n copies of x before p.
   //!
   //! <b>Throws</b>: If memory allocation throws or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to n plus linear to the elements before p.
   void insert(iterator p, size_type n, const value_type& x) 
   {  return insert_after(previous(p), n, x); }
      
   //! <b>Requires</b>: p must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Insert a copy of the [first, last) range before p.
   //!
   //! <b>Throws</b>: If memory allocation throws, T's constructor from a
   //!   dereferenced InpIt throws.
   //!
   //! <b>Complexity</b>: Linear to std::distance [first, last) plus
   //!    linear to the elements before p.
   template <class InIter>
   void insert(iterator p, InIter first, InIter last) 
   {  return insert_after(previous(p), first, last); }

   //! <b>Effects</b>: Erases the element after the element pointed by prev_pos
   //!    of the list.
   //!
   //! <b>Returns</b>: the first element remaining beyond the removed elements,
   //!   or end() if no such element exists.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Note</b>: Does not invalidate iterators or references to non erased elements.
   iterator erase_after(iterator prev_pos)
   {
      return iterator(this->m_islist.erase_after_and_destroy(prev_pos.get(), Destroyer(*this)));
   }

   //! <b>Effects</b>: Erases the range (before_first, last) from
   //!   the list. 
   //!
   //! <b>Returns</b>: the first element remaining beyond the removed elements,
   //!   or end() if no such element exists.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Linear to the number of erased elements.
   //! 
   //! <b>Note</b>: Does not invalidate iterators or references to non erased elements.
   iterator erase_after(iterator before_first, iterator last) 
   {
      return iterator(this->m_islist.erase_after_and_destroy(before_first.get(), last.get(), Destroyer(*this)));
   }

   //! <b>Requires</b>: p must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Erases the element at p p.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of elements before p.
   iterator erase(iterator p) 
   {  return iterator(this->erase_after(previous(p))); }

   //! <b>Requires</b>: first and last must be valid iterator to elements in *this.
   //!
   //! <b>Effects</b>: Erases the elements pointed by [first, last).
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the distance between first and last plus
   //!   linear to the elements before first.
   iterator erase(iterator first, iterator last)
   {  return iterator(this->erase_after(previous(first), last)); }

   //! <b>Effects</b>: Inserts or erases elements at the end such that
   //!   the size becomes n. New elements are copy constructed from x.
   //!
   //! <b>Throws</b>: If memory allocation throws, or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to the difference between size() and new_size.
   void resize(size_type newsize, const T& x)
   {
      typename Islist::iterator end_n(this->m_islist.end()), cur(end_n), cur_next;
      while (++(cur_next = cur) != end_n && newsize > 0){
         --newsize;
         cur = cur_next;
      }
      if (cur_next != end_n) 
         this->erase_after(iterator(cur), iterator(end_n));
      else
         this->insert_after(iterator(cur), newsize, x);
   }

   //! <b>Effects</b>: Inserts or erases elements at the end such that
   //!   the size becomes n. New elements are default constructed.
   //!
   //! <b>Throws</b>: If memory allocation throws, or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to the difference between size() and new_size.
   void resize(size_type newsize)
   {
      typename Islist::iterator end_n(this->m_islist.end()), cur(end_n), cur_next;
      size_type len = this->size();
      size_type left = newsize;
      
      while (++(cur_next = cur) != end_n && left > 0){
         --left;
         cur = cur_next;
      }
      if (cur_next != end_n){
         this->erase_after(iterator(cur), iterator(end_n));
      }
      else{
         size_type n = newsize - len;
         iterator c(cur);
         while(n--){
            T default_constructed;
            if(boost::is_scalar<T>::value){
               //Value initialization hack. Fix me!
               new(&default_constructed)T();
            }
            c = this->insert_after(c, move(default_constructed));
         }
      }
   }

   //! <b>Effects</b>: Erases all the elements of the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of elements in the list.
   void clear() 
   {  this->m_islist.clear_and_destroy(Destroyer(*this));  }

   // Removes all of the elements from the slist x to *this, inserting
   // them immediately after p.  x must not be *this.  Complexity:
   // linear in x.size().

   //! <b>Requires</b>: p must point to an element contained
   //!   by the list. x != *this
   //!
   //! <b>Effects</b>: Transfers all the elements of list x to this list, after the
   //!   the element pointed by p. No destructors or copy constructors are called.
   //!
   //! <b>Throws</b>: std::runtime_error if this' allocator and x's allocator
   //!   are not equal.
   //!
   //! <b>Complexity</b>: Linear to the elements in x.
   //! 
   //! <b>Note</b>: Iterators of values obtained from list x now point to elements of
   //!    this list. Iterators of this list and all the references are not invalidated.
   void splice_after(iterator prev_pos, slist& x)
   {
      if((NodeAlloc&)*this == (NodeAlloc&)x){
         this->m_islist.splice_after(prev_pos.get(), x.m_islist);
      }
      else{
         throw std::runtime_error("slist::splice called with unequal allocators");
      }
   }

   //void splice_after(iterator prev_pos, const detail::moved_object<slist>& x)
   //{  this->splice_after(prev_pos, x.get()); }

   // Moves the element that follows prev to *this, inserting it immediately
   //  after p.  This is constant time.

   //! <b>Requires</b>: prev_pos must be a valid iterator of this.
   //!   i must point to an element contained in list x.
   //! 
   //! <b>Effects</b>: Transfers the value pointed by i, from list x to this list, 
   //!   after the element pointed by prev_pos.
   //!   If prev_pos == prev or prev_pos == ++prev, this function is a null operation. 
   //! 
   //! <b>Throws</b>: std::runtime_error if this' allocator and x's allocator
   //!   are not equal.
   //! 
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Note</b>: Iterators of values obtained from list x now point to elements of this
   //!   list. Iterators of this list and all the references are not invalidated.
   void splice_after(iterator prev_pos, slist& x, iterator prev)
   {
      if((NodeAlloc&)*this == (NodeAlloc&)x){
         this->m_islist.splice_after(prev_pos.get(), x.m_islist, prev.get());
      }
      else{
         throw std::runtime_error("slist::splice called with unequal allocators");
      }
   }

   //void splice_after(iterator prev_pos, const detail::moved_object<slist>& x, iterator prev)
   //{  return splice_after(prev_pos, x.get(), prev);   }

   // Moves the range [before_first + 1, before_last + 1) to *this,
   //  inserting it immediately after p.  This is constant time.


   //! <b>Requires</b>: prev_pos must be a valid iterator of this.
   //!   before_first and before_last must be valid iterators of x.
   //!   prev_pos must not be contained in [before_first, before_last) range.
   //! 
   //! <b>Effects</b>: Transfers the range [before_first + 1, before_last + 1)
   //!   from list x to this list, after the element pointed by prev_pos.
   //! 
   //! <b>Throws</b>: std::runtime_error if this' allocator and x's allocator
   //!   are not equal.
   //! 
   //! <b>Complexity</b>: Linear to the number of transferred elements.
   //! 
   //! <b>Note</b>: Iterators of values obtained from list x now point to elements of this
   //!   list. Iterators of this list and all the references are not invalidated.
   void splice_after(iterator prev_pos,      slist& x, 
                     iterator before_first,  iterator before_last)
   {
      if((NodeAlloc&)*this == (NodeAlloc&)x){
         this->m_islist.splice_after
            (prev_pos.get(), x.m_islist, before_first.get(), before_last.get());
      }
      else{
         throw std::runtime_error("slist::splice called with unequal allocators");
      }
   }

   //void splice_after(iterator prev_pos,      const detail::moved_object<slist>& x, 
   //                  iterator before_first,  iterator before_last)
   //{  this->splice_after(prev_pos, x.get(), before_first, before_last); }

   //! <b>Requires</b>: prev_pos must be a valid iterator of this.
   //!   before_first and before_last must be valid iterators of x.
   //!   prev_pos must not be contained in [before_first, before_last) range.
   //!   n == std::distance(before_first, before_last)
   //! 
   //! <b>Effects</b>: Transfers the range [before_first + 1, before_last + 1)
   //!   from list x to this list, after the element pointed by prev_pos.
   //! 
   //! <b>Throws</b>: std::runtime_error if this' allocator and x's allocator
   //!   are not equal.
   //! 
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Note</b>: Iterators of values obtained from list x now point to elements of this
   //!   list. Iterators of this list and all the references are not invalidated.
   void splice_after(iterator prev_pos,      slist& x, 
                     iterator before_first,  iterator before_last,
                     size_type n)
   {
      if((NodeAlloc&)*this == (NodeAlloc&)x){
         this->m_islist.splice_after
            (prev_pos.get(), x.m_islist, before_first.get(), before_last.get(), n);
      }
      else{
         throw std::runtime_error("slist::splice called with unequal allocators");
      }
   }

   //void splice_after(iterator prev_pos,      const detail::moved_object<slist>& x, 
   //                  iterator before_first,  iterator before_last, size_type n)
   //{  this->splice_after(prev_pos, x.get(), before_first, before_last, n); }

   //! <b>Requires</b>: p must point to an element contained
   //!   by the list. x != *this
   //!
   //! <b>Effects</b>: Transfers all the elements of list x to this list, before the
   //!   the element pointed by p. No destructors or copy constructors are called.
   //!
   //! <b>Throws</b>: std::runtime_error if this' allocator and x's allocator
   //!   are not equal.
   //!
   //! <b>Complexity</b>: Linear in distance(begin(), p), and linear in x.size().
   //! 
   //! <b>Note</b>: Iterators of values obtained from list x now point to elements of
   //!    this list. Iterators of this list and all the references are not invalidated.
   void splice(iterator p, slist& x) 
   {  this->splice_after(this->previous(p), x);  }

   //void splice(iterator p, const detail::moved_object<slist>& x) 
   //{  return this->splice(p, x.get());  }

   //! <b>Requires</b>: p must point to an element contained
   //!   by this list. i must point to an element contained in list x.
   //! 
   //! <b>Effects</b>: Transfers the value pointed by i, from list x to this list, 
   //!   before the the element pointed by p. No destructors or copy constructors are called.
   //!   If p == i or p == ++i, this function is a null operation. 
   //! 
   //! <b>Throws</b>: std::runtime_error if this' allocator and x's allocator
   //!   are not equal.
   //! 
   //! <b>Complexity</b>: Linear in distance(begin(), p), and in distance(x.begin(), i).
   //! 
   //! <b>Note</b>: Iterators of values obtained from list x now point to elements of this
   //!   list. Iterators of this list and all the references are not invalidated.
   void splice(iterator p, slist& x, iterator i) 
   {  this->splice_after(previous(p), x, i);  }

   //void splice(iterator p, const detail::moved_object<slist>& x, iterator i)
   //{  this->splice(p, x.get(), i);   }

   //! <b>Requires</b>: p must point to an element contained
   //!   by this list. first and last must point to elements contained in list x.
   //! 
   //! <b>Effects</b>: Transfers the range pointed by first and last from list x to this list, 
   //!   before the the element pointed by p. No destructors or copy constructors are called.
   //! 
   //! <b>Throws</b>: std::runtime_error if this' allocator and x's allocator
   //!   are not equal.
   //! 
   //! <b>Complexity</b>: Linear in distance(begin(), p), in distance(x.begin(), first),
   //!   and in distance(first, last).
   //! 
   //! <b>Note</b>: Iterators of values obtained from list x now point to elements of this
   //!   list. Iterators of this list and all the references are not invalidated.
   void splice(iterator p, slist& x, iterator first, iterator last)
   {  this->splice_after(previous(p), x, previous(first), previous(last));  }

   //void splice(iterator p, const detail::moved_object<slist>& x, iterator first, iterator last)
   //{  this->splice(p, x.get(), first, last);  }

   //! <b>Effects</b>: Reverses the order of elements in the list. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: This function is linear time.
   //! 
   //! <b>Note</b>: Iterators and references are not invalidated
   void reverse() 
   {  this->m_islist.reverse();  }

   //! <b>Effects</b>: Removes all the elements that compare equal to value.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Linear time. It performs exactly size() comparisons for equality.
   //! 
   //! <b>Note</b>: The relative order of elements that are not removed is unchanged,
   //!   and iterators to elements that are not removed remain valid.
   void remove(const T& value)
   {  remove_if(equal_to_value(value));  }

   //! <b>Effects</b>: Removes all the elements for which a specified
   //!   predicate is satisfied.
   //! 
   //! <b>Throws</b>: If pred throws.
   //! 
   //! <b>Complexity</b>: Linear time. It performs exactly size() calls to the predicate.
   //! 
   //! <b>Note</b>: The relative order of elements that are not removed is unchanged,
   //!   and iterators to elements that are not removed remain valid.
   template <class Pred> 
   void remove_if(Pred pred)
   {
      typedef ValueCompareToNodeCompare<Pred> Predicate;
      this->m_islist.remove_and_destroy_if(Predicate(pred), Destroyer(*this));
   }

   //! <b>Effects</b>: Removes adjacent duplicate elements or adjacent 
   //!   elements that are equal from the list.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Linear time (size()-1 comparisons calls to pred()).
   //! 
   //! <b>Note</b>: The relative order of elements that are not removed is unchanged,
   //!   and iterators to elements that are not removed remain valid.
   void unique()
   {  this->unique(value_equal());  }

   //! <b>Effects</b>: Removes adjacent duplicate elements or adjacent 
   //!   elements that satisfy some binary predicate from the list.
   //! 
   //! <b>Throws</b>: If pred throws.
   //! 
   //! <b>Complexity</b>: Linear time (size()-1 comparisons equality comparisons).
   //! 
   //! <b>Note</b>: The relative order of elements that are not removed is unchanged,
   //!   and iterators to elements that are not removed remain valid.
   template <class Pred> 
   void unique(Pred pred)
   {
      typedef ValueCompareToNodeCompare<Pred> Predicate;
      this->m_islist.unique_and_destroy(Predicate(pred), Destroyer(*this));
   }

   //! <b>Requires</b>: The lists x and *this must be distinct. 
   //!
   //! <b>Effects</b>: This function removes all of x's elements and inserts them
   //!   in order into *this according to std::less<value_type>. The merge is stable; 
   //!   that is, if an element from *this is equivalent to one from x, then the element 
   //!   from *this will precede the one from x. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: This function is linear time: it performs at most
   //!   size() + x.size() - 1 comparisons.
   void merge(slist& x)
   {  this->merge(x, value_less()); }

   //void merge(const detail::moved_object<slist>& x)
   //{  this->merge(x.get(), value_less()); }

   //! <b>Requires</b>: p must be a comparison function that induces a strict weak
   //!   ordering and both *this and x must be sorted according to that ordering
   //!   The lists x and *this must be distinct. 
   //! 
   //! <b>Effects</b>: This function removes all of x's elements and inserts them
   //!   in order into *this. The merge is stable; that is, if an element from *this is 
   //!   equivalent to one from x, then the element from *this will precede the one from x. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: This function is linear time: it performs at most
   //!   size() + x.size() - 1 comparisons.
   //! 
   //! <b>Note</b>: Iterators and references to *this are not invalidated.
   template <class StrictWeakOrdering> 
   void merge(slist& x, StrictWeakOrdering comp)
   {
      if((NodeAlloc&)*this == (NodeAlloc&)x){
         this->m_islist.merge(x.m_islist,
            ValueCompareToNodeCompare<StrictWeakOrdering>(comp));
      }
      else{
         throw std::runtime_error("list::merge called with unequal allocators");
      }
   }

   //template <class StrictWeakOrdering> 
   //void merge(const detail::moved_object<slist>& x, StrictWeakOrdering comp)
   //{  this->merge(x.get(), comp);  }

   //! <b>Effects</b>: This function sorts the list *this according to std::less<value_type>. 
   //!   The sort is stable, that is, the relative order of equivalent elements is preserved.
   //! 
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Notes</b>: Iterators and references are not invalidated.
   //! 
   //! <b>Complexity</b>: The number of comparisons is approximately N log N, where N
   //!   is the list's size.
   void sort()
   {  this->sort(value_less());  }

   //! <b>Effects</b>: This function sorts the list *this according to std::less<value_type>. 
   //!   The sort is stable, that is, the relative order of equivalent elements is preserved.
   //! 
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Notes</b>: Iterators and references are not invalidated.
   //! 
   //! <b>Complexity</b>: The number of comparisons is approximately N log N, where N
   //!   is the list's size.
   template <class StrictWeakOrdering> 
   void sort(StrictWeakOrdering comp)
   {
      // nothing if the slist has length 0 or 1.
      if (this->size() < 2)
         return;
      this->m_islist.sort(ValueCompareToNodeCompare<StrictWeakOrdering>(comp));
   }

   /// @cond
   private:
   void fill_assign(size_type n, const T& val)
   {
      iterator end_n(end());
      iterator prev(before_begin());
      iterator node(begin());
      for ( ; node != end_n && n > 0 ; --n){
         *node = val;
         prev = node;
         ++node;
      }
      if (n > 0)
         this->insert_after(prev, n, val);
      else
         this->erase_after(prev, end_n);
   }

   template <class Int>
   void assign_dispatch(Int n, Int val, boost::mpl::true_)
   {  this->fill_assign((size_type) n, (T)val); }

   template <class InpIt>
   void assign_dispatch(InpIt first, InpIt last,
                           boost::mpl::false_)
   {
      iterator end_n(end());
      iterator prev(before_begin());
      iterator node(begin());
      while (node != end_n && first != last){
         *node = *first;
         prev = node;
         ++node;
         ++first;
      }
      if (first != last)
         this->insert_after_range(prev, first, last);
      else
         this->erase_after(prev, end_n);
   }

   void priv_insert_after_fill(iterator prev_pos, size_type n, const value_type& x) 
   {
      typename Islist::iterator intrusive_it(prev_pos.get());
      for (size_type i = 0; i < n; ++i){
         intrusive_it = this->m_islist.insert_after(intrusive_it, *this->create_node(x));
      }
   }

   // Check whether it's an integral type.  If so, it's not an iterator.
   template <class InIter>
   void insert_after_range(iterator prev_pos, InIter first, InIter last) 
   {
      const bool aux_boolean = boost::is_integral<InIter>::value;
      typedef boost::mpl::bool_<aux_boolean> Result;
      this->insert_after_range(prev_pos, first, last, Result());
   }

   template <class Int>
   void insert_after_range(iterator prev_pos, Int n, Int x, boost::mpl::true_) 
   {  this->priv_insert_after_fill(prev_pos, n, x);  }

   template <class InIter>
   void insert_after_range(iterator prev_pos, InIter first, InIter last, boost::mpl::false_) 
   {
      typename Islist::iterator intrusive_it(prev_pos.get());
      while (first != last){
         intrusive_it = this->m_islist.insert_after(intrusive_it, *this->create_node(*first));
         ++first;
      }
   }

   //Functors for member algorithm defaults
   struct value_less
   {
      bool operator()(const value_type &a, const value_type &b) const
         {  return a < b;  }
   };

   struct value_equal
   {
      bool operator()(const value_type &a, const value_type &b) const
         {  return a == b;  }
   };

   struct value_equal_to_this
   {
      explicit value_equal_to_this(const value_type &ref)
         : m_ref(ref){}

      bool operator()(const value_type &val) const
         {  return m_ref == val;  }

      const value_type &m_ref;
   };
   /// @endcond
};

template <class T, class A>
inline bool 
operator==(const slist<T,A>& x, const slist<T,A>& y)
{
   if(x.size() != y.size()){
      return false;
   }
   typedef typename slist<T,A>::const_iterator const_iterator;
   const_iterator end1 = x.end();

   const_iterator i1 = x.begin();
   const_iterator i2 = y.begin();
   while (i1 != end1 && *i1 == *i2){
      ++i1;
      ++i2;
   }
   return i1 == end1;
}

template <class T, class A>
inline bool
operator<(const slist<T,A>& sL1, const slist<T,A>& sL2)
{
   return std::lexicographical_compare
      (sL1.begin(), sL1.end(), sL2.begin(), sL2.end());
}

template <class T, class A>
inline bool 
operator!=(const slist<T,A>& sL1, const slist<T,A>& sL2) 
   {  return !(sL1 == sL2);   }

template <class T, class A>
inline bool 
operator>(const slist<T,A>& sL1, const slist<T,A>& sL2) 
   {  return sL2 < sL1; }

template <class T, class A>
inline bool 
operator<=(const slist<T,A>& sL1, const slist<T,A>& sL2)
   {  return !(sL2 < sL1); }

template <class T, class A>
inline bool 
operator>=(const slist<T,A>& sL1, const slist<T,A>& sL2)
   {  return !(sL1 < sL2); }

template <class T, class A>
inline void swap(slist<T,A>& x, slist<T,A>& y) 
   {  x.swap(y);  }

template <class T, class A>
inline void swap(const detail::moved_object<slist<T,A> >& x, slist<T,A>& y) 
   {  x.get().swap(y);  }

template <class T, class A>
inline void swap(slist<T,A>& x, const detail::moved_object<slist<T,A> >& y) 
   {  x.swap(y.get());  }

/// @cond
/*!This class is movable*/
template <class T, class A>
struct is_movable<slist<T, A> >
{
   enum {   value = true };
};

/*!This class is movable*/
template <class A, class VoidPointer>
struct is_movable<detail::slist_node<A, VoidPointer> >
{
   enum {   value = true };
};

/*!This class is movable*/
template <class A>
struct is_movable<detail::slist_alloc<A> >
{
   enum {   value = true };
};

//!has_trivial_destructor_after_move<> == true_type
//!specialization for optimizations
template <class T, class A>
struct has_trivial_destructor_after_move<slist<T, A> >
{
   enum {   value = has_trivial_destructor<A>::value  };
};
/// @endcond

}} //namespace boost{  namespace interprocess{

// Specialization of insert_iterator so that insertions will be constant
// time rather than linear time.

//iG
//Ummm, I don't like to define things in namespace std, but 
//there is no other way
namespace std {

template <class T, class A>
class insert_iterator<boost::interprocess::slist<T, A> > 
{
 protected:
   typedef boost::interprocess::slist<T, A> Container;
   Container* container;
   typename Container::iterator iter;
   public:
   typedef Container           container_type;
   typedef output_iterator_tag iterator_category;
   typedef void                value_type;
   typedef void                difference_type;
   typedef void                pointer;
   typedef void                reference;

   insert_iterator(Container& x, 
                   typename Container::iterator i, 
                   bool is_previous = false) 
      : container(&x), iter(is_previous ? i : x.previous(i)){ }

   insert_iterator<Container>& 
      operator=(const typename Container::value_type& value) 
   { 
      iter = container->insert_after(iter, value);
      return *this;
   }
   insert_iterator<Container>& operator*(){ return *this; }
   insert_iterator<Container>& operator++(){ return *this; }
   insert_iterator<Container>& operator++(int){ return *this; }
};

}  //namespace std;



#include <boost/interprocess/detail/config_end.hpp>

#endif /* BOOST_INTERPROCESS_SLIST_HPP */
