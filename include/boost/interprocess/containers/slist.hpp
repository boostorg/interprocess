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
// (C) Copyright Ion Gaztañaga 2004-2005. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
//
// This file comes from SGI's stl_slist.h file. Modified by Ion Gazta�ga 2004-2005
// Renaming, isolating and porting to generic algorithms. Pointer typedef 
// set to allocator::pointer to allow placing it in shared memory.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_SLIST_HPP
#define BOOST_INTERPROCESS_SLIST_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/iterator/reverse_iterator.hpp>
#include <boost/iterator.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/detail/allocator_utilities.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/detail/no_exceptions_support.hpp>

//#include <new>
#include <utility>
#include <memory>
#include <functional>
#include <algorithm>

namespace boost{  namespace interprocess{

template<class Alloc>
struct shmem_slist_node;


template<class Alloc>
struct shmem_slist_node_base
{
   typedef typename boost::detail::allocator::
         rebind_to<Alloc, shmem_slist_node<Alloc> >::type   NodeAlloc;
   typedef typename  NodeAlloc::pointer                     NodePtr;
   typedef typename  NodeAlloc::const_pointer               NodeCPtr;

   NodePtr m_next;

   //Inserts new node after prev_node and returns new_node
   static NodePtr make_link(NodePtr prev_node, NodePtr new_node)
   {
      new_node->m_next = prev_node->m_next;
      prev_node->m_next = new_node;
      return new_node;
   }

   //Returns the previous node of node "node", starting from node head
   static NodePtr previous(NodePtr head, NodeCPtr node)
   {
      while (head->m_next != node)
         head = head->m_next;
      return head;
   }

   //Moves elements of a list (before_first, before_last] after prev_pos
   //Example from two lists A and B:  
   // A:       prev_pos -> pos
   // B:       before_first -> first -> c -> d-> before_last -> after
   //becomes
   // A:       prev_pos -> first -> c -> d-> before_last -> pos
   // B:       before_first -> after
   static void splice_after(NodePtr prev_pos, NodePtr before_first, NodePtr before_last)
   {
      if (prev_pos != before_first && prev_pos != before_last) {
         NodePtr first = before_first->m_next;
         NodePtr after = prev_pos->m_next;
         before_first->m_next = before_last->m_next;
         prev_pos->m_next = first;
         before_last->m_next = after;
      }
   }

   //Inverses elements of a list defined by [head, last)
   //The new final node points to last
   //Returns new first node
   //Example:  
   //          first -> b -> c -> d-> last 
   //becomes
   //          d -> c -> b -> first -> last
   //returns:  d
   static NodePtr reverse(NodePtr first, NodePtr last)
   {
      NodePtr result = first;
      first = first->m_next; 
      result->m_next = last;
      while(first != last) {
         NodePtr next = first->m_next;
         first->m_next = result;
         result = first;
         first = next;
      }
      return result;
   }

   //Returns the number of nodes [first, last)
   //Example:  
   //          first -> last 
   //returns:  1
   static std::size_t size(NodePtr first, NodePtr last)
   {
      std::size_t result = 0;
      for ( ; first->m_next != last; first = first->m_next)
         ++result;
      return result;
   }
};

template<class Alloc>
struct shmem_slist_node
   : public shmem_slist_node_base<Alloc>
{
   typedef shmem_slist_node_base<Alloc>                     base_t;
   typedef typename boost::detail::allocator::
         rebind_to<Alloc, shmem_slist_node<Alloc> >::type   NodeAlloc;
   typedef typename  NodeAlloc::pointer                     NodePtr;
   typedef typename  NodeAlloc::const_pointer               NodeCPtr;
   typedef typename  Alloc::value_type                      value_t;

   shmem_slist_node(const value_t & value)
      : m_data(value){}

   value_t  m_data;
};

template<class T, class A, bool convertible_construct>
struct shmem_slist_alloc
   :  public boost::detail::allocator::
               rebind_to<A, shmem_slist_node<A> >::type,
      public boost::detail::allocator::
            rebind_to<A, typename boost::detail::allocator::
               rebind_to<A, shmem_slist_node<A> >::
                  type::pointer >::type,
      public A
{
   typedef shmem_slist_node<A>                        Node;
   typedef typename boost::detail::allocator::
      rebind_to<A, shmem_slist_node<A> >::type        NodeAlloc;
   typedef typename boost::detail::allocator::
      rebind_to<A, typename NodeAlloc::pointer>::type PtrAlloc;
   typedef A                                          ValAlloc;
   typedef typename NodeAlloc::pointer                NodePtr;
   typedef detail::scoped_deallocator<NodeAlloc>      Deallocator;
   typedef detail::scoped_destructor<PtrAlloc>        PtrDestructor;

   enum {   
      node_has_trivial_destructor =  
      boost::has_trivial_destructor<NodePtr>::value |
      boost::has_trivial_destructor<T>::value 
   };

   shmem_slist_alloc(const ValAlloc &a) 
      : NodeAlloc(a), PtrAlloc(a), ValAlloc(a) 
      {  priv_init();   }

   shmem_slist_alloc(const NodeAlloc &a) 
      : NodeAlloc(a), PtrAlloc(a), ValAlloc(a) 
      {  priv_init();   }

   shmem_slist_alloc(const shmem_slist_alloc &other)
      : NodeAlloc(other), PtrAlloc(other), ValAlloc(other)
      {  priv_init();   }

   ~shmem_slist_alloc()
   {
      if(!boost::has_trivial_constructor<NodePtr>::value){
         PtrAlloc::destroy(&m_node->m_next);
      }
      NodeAlloc::deallocate(m_node, 1); 
   }

   typename NodeAlloc::size_type max_size() const
      {  return NodeAlloc::max_size();  }

   void priv_init()
   {
      m_node = NodeAlloc::allocate(1);

      if(!boost::has_trivial_constructor<NodePtr>::value){
         scoped_ptr<Node, Deallocator>node_deallocator(m_node, *this);
         //Make sure destructors are called before memory is freed
         //if an exception is thrown
         {
            typedef typename PtrAlloc::pointer NodePtrPtr;
            NodePtrPtr  pnext(PtrAlloc::address(m_node->m_next));
            PtrAlloc::construct(pnext, m_node);
            scoped_ptr<NodePtr, PtrDestructor>next_destroy(pnext, *this);
            next_destroy.release();
         }
         node_deallocator.release();
      }
      else{
         m_node->m_next = m_node;
      }
   }

   template <class Convertible>
   NodePtr create_node(const Convertible& x)
   {
      NodePtr p = NodeAlloc::allocate(1);
      scoped_ptr<Node, Deallocator>node_deallocator(p, *this);


      //Make sure destructors are called before memory is freed
      //if an exception is thrown
      {
         if(!boost::has_trivial_constructor<NodePtr>::value){
            typedef typename PtrAlloc::pointer NodePtrPtr;
            NodePtrPtr  pnext(PtrAlloc::address(p->m_next));
            PtrAlloc::construct(pnext, NodePtr(0));
            scoped_ptr<NodePtr, PtrDestructor>next_destroy(pnext, *this);
            ValAlloc::construct(ValAlloc::address(p->m_data), x);
            next_destroy.release();
         }
         else{
            p->m_next = 0;
            ValAlloc::construct(ValAlloc::address(p->m_data), x);
         }
      }
      node_deallocator.release();
      return (p);
   }
   
   void destroy_node(NodePtr node)
   {
      if(!node_has_trivial_destructor){
         NodeAlloc::destroy(node);
      }
      NodeAlloc::deallocate(node, 1);
   }
   
   void swap(shmem_slist_alloc &x)
   {
      if (static_cast<NodeAlloc&>(*this) != 
          static_cast<NodeAlloc&>(x)){
            detail::do_swap(static_cast<NodeAlloc&>(*this), 
                                  static_cast<NodeAlloc&>(x));
            detail::do_swap(static_cast<PtrAlloc&>(*this), 
                                  static_cast<PtrAlloc&>(x));
            detail::do_swap(static_cast<ValAlloc&>(*this), 
                                  static_cast<ValAlloc&>(x));
      }
      detail::do_swap(this->m_node, x.m_node);      
   }

 protected:
   typename NodeAlloc::pointer m_node;
};

template<class T, class A>
struct shmem_slist_alloc<T, A, true>
   :  public boost::detail::allocator::
         rebind_to<A, shmem_slist_node<A> >::type
{
   typedef shmem_slist_node<A>                        Node;
   typedef typename boost::detail::allocator::
      rebind_to<A, shmem_slist_node<A> >::type        NodeAlloc;
   typedef typename boost::detail::allocator::
      rebind_to<A, typename NodeAlloc::pointer>::type PtrAlloc;
   typedef A                                          ValAlloc;
   typedef typename NodeAlloc::pointer                NodePtr;
   typedef detail::scoped_deallocator<NodeAlloc>      Deallocator;
   typedef detail::scoped_destructor<PtrAlloc>        PtrDestructor;

   enum {   
      node_has_trivial_destructor =  
      boost::has_trivial_destructor<NodePtr>::value |
      boost::has_trivial_destructor<T>::value 
   };

   shmem_slist_alloc(const ValAlloc &a) 
      : NodeAlloc(a)
   {  priv_init();   }

   shmem_slist_alloc(const NodeAlloc &a) 
      : NodeAlloc(a)
   {  priv_init();   }

   shmem_slist_alloc(const shmem_slist_alloc &other)
      : NodeAlloc(other)
   {  priv_init();   }

  ~shmem_slist_alloc()
   {
      if(!boost::has_trivial_destructor<NodePtr>::value){
         PtrAlloc ptr_alloc(*this);
         ptr_alloc.destroy(ptr_alloc.address(m_node->m_next));
      }
      NodeAlloc::deallocate(m_node, 1); 
   }

   typename NodeAlloc::size_type max_size() const
      {  return NodeAlloc::max_size();  }

   template<class Convertible>
   NodePtr create_node(const Convertible& x)
   {
      NodePtr p = NodeAlloc::allocate(1);
      scoped_ptr<Node, Deallocator>node_deallocator(p, *this);
      NodeAlloc::construct(p, x);
      node_deallocator.release();
      p->m_next = NodePtr(0);
      return (p);
   }

   void destroy_node(NodePtr node)
   {
      if(!node_has_trivial_destructor){
         NodeAlloc::destroy(node);
      }
      NodeAlloc::deallocate(node, 1);
   }

   void swap(shmem_slist_alloc &x)
   {
      NodeAlloc& this_alloc  = static_cast<NodeAlloc&>(*this);
      NodeAlloc& other_alloc = static_cast<NodeAlloc&>(x);
      if (this_alloc != other_alloc){
         detail::do_swap(this_alloc, other_alloc);
      }
      detail::do_swap(this->m_node, x.m_node);     
   }

 protected:
   typename NodeAlloc::pointer m_node;

 private:

    void priv_init()
    {
       m_node = NodeAlloc::allocate(1);
       if(!boost::has_trivial_constructor<NodePtr>::value){
         typedef typename PtrAlloc::pointer NodePtrPtr;
         scoped_ptr<Node, Deallocator>node_deallocator(m_node, *this);
         PtrAlloc ptr_alloc(*this);
         NodePtrPtr  pnext(ptr_alloc.address(m_node->m_next));

         //Make sure destructors are called before memory is freed
         //if an exception is thrown
         {
            ptr_alloc.construct(pnext, m_node);
            scoped_ptr<NodePtr, PtrDestructor>next_destroy(pnext, ptr_alloc);
            next_destroy.release();
         }
         node_deallocator.release();
      }
      else{
         m_node->m_next = m_node;
      }
    }
};

template <class T, class Alloc>
class slist 
   : protected shmem_slist_alloc<T, Alloc, 
               has_convertible_construct<Alloc>::value>
{
   typedef shmem_slist_alloc<T, Alloc, 
               has_convertible_construct<Alloc>::value>     Base;

   typedef typename Base::NodePtr                  NodePtr;
 public:
   typedef Alloc                                   allocator_type;
   typedef T                                       value_type;
   typedef typename Alloc::pointer                 pointer;
   typedef typename Alloc::const_pointer           const_pointer;
   typedef typename Alloc::reference               reference;
   typedef typename Alloc::const_reference         const_reference;
   typedef typename Alloc::size_type               size_type;
   typedef typename Alloc::difference_type         difference_type;

   allocator_type get_allocator() const 
      { return allocator_type(*this); }

   typedef difference_type                         list_difference_type;
   typedef pointer                                 list_pointer;
   typedef const_pointer                           list_const_pointer;
   typedef reference                               list_reference;
   typedef const_reference                         list_const_reference;

   //list const_iterator
   class const_iterator
      : public boost::iterator<std::forward_iterator_tag, 
                                 value_type,         list_difference_type, 
                                 list_const_pointer, list_const_reference>
   {
    protected:
      typedef typename slist::NodePtr  NodePtr;
   
      NodePtr m_ptr;
      explicit const_iterator(NodePtr ptr)  : m_ptr(ptr){}
      void prot_incr() { m_ptr = m_ptr->m_next; }

    public:
      friend class slist<T, Alloc>;
      typedef list_difference_type        difference_type;

      //Constructors
      const_iterator() : m_ptr(0){}

      //Pointer like operators
      const_reference operator*()  const 
         { return  m_ptr->m_data;  }

      const_pointer   operator->() const 
         { return  const_pointer(&m_ptr->m_data); }

      //Increment / Decrement
      const_iterator& operator++()       
         { prot_incr();  return *this; }

      const_iterator operator++(int)      
         { NodePtr tmp = m_ptr; ++*this; return const_iterator(tmp);  }

      //Comparison operators
      bool operator==   (const const_iterator& r)  const
         {  return m_ptr == r.m_ptr;  }

      bool operator!=   (const const_iterator& r)  const
         {  return m_ptr != r.m_ptr;  }

      bool operator<    (const const_iterator& r)  const
         {  return m_ptr < r.m_ptr;  }

      bool operator<=   (const const_iterator& r)  const
         {  return m_ptr <= r.m_ptr;  }

      bool operator>    (const const_iterator& r)  const
         {  return m_ptr > r.m_ptr;  }

      bool operator>=   (const const_iterator& r)  const
         {  return m_ptr >= r.m_ptr;  }
   };

   //list iterator
   class iterator : public const_iterator
   {
      //typedef typename const_iterator::NodePtr  NodePtr;
      typedef typename slist::NodePtr  NodePtr;
    protected:
      explicit iterator(NodePtr ptr) : const_iterator(ptr){}

    public:
      friend class slist<T, Alloc>;
      typedef list_pointer       pointer;
      typedef list_reference     reference;

      //Constructors
      iterator(){}

      //Pointer like operators
      reference operator*()  const {  return  this->m_ptr->m_data;  }
      pointer   operator->() const {  return  pointer(&this->m_ptr->m_data);  }

      //Increment / Decrement
      iterator& operator++()  
         { this->prot_incr(); return *this;  }

      iterator operator++(int)
         { NodePtr tmp = this->m_ptr; ++*this; return iterator(tmp); }
   };

 private:
   typedef shmem_slist_node<Alloc>              Node;
   typedef typename Base::NodeAlloc             NodeAlloc;
   typedef typename Base::PtrAlloc              PtrAlloc;
   typedef typename Base::ValAlloc              ValAlloc;
   typedef typename  NodeAlloc::const_pointer   NodeCPtr;

 public:
   explicit slist(const allocator_type& a = allocator_type()) : Base(a) {}

   explicit slist(size_type n, const value_type& x = value_type(),
                  const allocator_type& a =  allocator_type()) : Base(a)
      { this->insert_after_fill(this->m_node, n, x); }

   // We don't need any dispatching tricks here, because insert_after_range
   // already does them.
   template <class InpIt>
   slist(InpIt first, InpIt last,
         const allocator_type& a =  allocator_type()) 
      : Base(a)
      { insert_after_range(this->m_node, first, last); }

   slist(const slist& x) 
      : Base(static_cast<const NodeAlloc&>(x))
      { insert_after_range(this->m_node, x.begin(), x.end()); }

   slist& operator= (const slist& x)
   {
      if (&x != this) {
         this->assign(x.begin(), x.end());
      }
      return *this;
   }

  ~slist() {   this->clear();}

public:
   // assign(), a generalized assignment member function.  Two
   // versions: one that takes a count, and one that takes a range.
   // The range version is a member template, so we dispatch on whether
   // or not the type is an integer.

   void assign(size_type n, const T& val)
      { this->fill_assign(n, val); }

   void fill_assign(size_type n, const T& val)
   {
      NodePtr prev   = this->m_node;
      NodePtr node   = this->m_node->m_next;
      for ( ; node != this->m_node && n > 0 ; --n) {
         node->m_data = val;
         prev = node;
         node = node->m_next;
      }
      if (n > 0)
         this->insert_after_fill(prev, n, val);
      else
         this->erase_after(prev, this->m_node);
   }

   template <class InpIt>
   void assign(InpIt first, InpIt last) 
   {
      const bool aux_boolean = boost::is_integral<InpIt>::value;
      typedef boost::mpl::bool_<aux_boolean> Result;
      this->assign_dispatch(first, last, Result());
   }

   template <class Int>
   void assign_dispatch(Int n, Int val, boost::mpl::true_)
      { this->fill_assign((size_type) n, (T) val); }

   template <class InpIt>
   void assign_dispatch(InpIt first, InpIt last,
                           boost::mpl::false_)
   {
      NodePtr prev = this->m_node;
      NodePtr node = this->m_node->m_next;
      while (node != this->m_node && first != last) {
         node->m_data = *first;
         prev = node;
         node = node->m_next;
         ++first;
      }
      if (first != last)
         this->insert_after_range(prev, first, last);
      else
         this->erase_after(prev, this->m_node);
   }

public:

   iterator begin() 
      { return iterator(this->m_node->m_next); }

   const_iterator begin() const 
      { return const_iterator(this->m_node->m_next);}

   iterator end() { return iterator(this->m_node); }

   const_iterator end() const { return const_iterator(this->m_node); }

   // Experimental new feature: before_begin() returns a
   // non-dereferenceable iterator that, when incremented, yields
   // begin().  This iterator may be used as the argument to
   // insert_after, erase_after, etc.
   iterator before_begin() 
      { return iterator(this->m_node); }

   const_iterator before_begin() const
      { return const_iterator(this->m_node); }

   size_type size() const 
      { return Node::size(this->m_node->m_next, this->m_node->m_next); }

   size_type maxsize() const 
      { return size_type(-1); }

   bool empty() const 
      { return this->m_node->m_next == this->m_node; }

   void swap(slist& x)
      {  Base::swap(x); }

 public:

   reference front() 
      { return this->m_node->m_next->m_data; }

   const_reference front() const 
      { return this->m_node->m_next->m_data; }

   void push_front(const value_type& x = value_type())   
      { Node::make_link(this->m_node, this->create_node(x)); }

   void pop_front() 
   {
      NodePtr node = this->m_node->m_next;
      this->m_node->m_next = node->m_next;
      this->destroy_node(node);
   }

   iterator previous(const_iterator pos) 
      { return iterator( Node::previous(this->m_node, pos.m_ptr));  }

   const_iterator previous(const_iterator pos) const 
      {  return const_iterator(Node::previous(this->m_node, pos.m_ptr));   }

   iterator insert_after(iterator prev_pos, const value_type& x = value_type()) 
      {  return iterator(insert_after(prev_pos.m_ptr, x)); }

   void insert_after(iterator prev_pos, size_type n, const value_type& x) 
      {  this->insert_after_fill(prev_pos.m_ptr, n, x); }

   // We don't need any dispatching tricks here, because insert_after_range
   // already does them.
   template <class InIter>
   void insert_after(iterator prev_pos, InIter first, InIter last) 
      {  this->insert_after_range(prev_pos.m_ptr, first, last);  }

   iterator insert(iterator prev_pos, const value_type& x = value_type()) 
   {
      return iterator(insert_after(
                        Node::previous(this->m_node, prev_pos.m_ptr), x));
   }

   void insert(iterator pos, size_type n, const value_type& x) 
   {
      this->insert_after_fill(Node::previous(this->m_node, pos.m_ptr), n, x);
   } 
      
   // We don't need any dispatching tricks here, because insert_after_range
   // already does them.
   template <class InIter>
   void insert(iterator pos, InIter first, InIter last) 
   {
      this->insert_after_range(Node::previous(this->m_node, pos.m_ptr), 
                               first, last);
   }

   iterator erase_after(iterator prev_pos) 
      {  return iterator(this->erase_after(prev_pos.m_ptr)); }

   iterator erase_after(iterator before_first, iterator last) 
      {  return iterator(this->erase_after(before_first.m_ptr, last.m_ptr));  } 

   iterator erase(iterator pos) 
      {  return iterator(this->erase_after(Node::previous(this->m_node, pos.m_ptr)));   }

   iterator erase(iterator first, iterator last) 
   {
      return iterator(this->erase_after(
         Node::previous(this->m_node, first.m_ptr), last.m_ptr));
   }

   void resize(size_type newsize, const T& x = T())
   {
      NodePtr cur = this->m_node;
      while (cur->m_next != this->m_node && newsize > 0) {
         --newsize;
         cur = cur->m_next;
      }
      if (cur->m_next != this->m_node) 
         this->erase_after(cur, this->m_node);
      else
         this->insert_after_fill(cur, newsize, x);
   }

   void clear() 
      { this->erase_after(this->m_node, this->m_node); }

public:
   // Removes all of the elements from the list x to *this, inserting
   // them immediately after pos.  x must not be *this.  Complexity:
   // linear in x.size().
   void splice_after(iterator prev_pos, slist& x)
   {
      if((NodeAlloc&)*this == (NodeAlloc&)x){
         if (!x.empty()){
            Node::splice_after(prev_pos.m_ptr, x.m_node, 
                               Node::previous(x.m_node, x.m_node));
         }
      }
      else{
         this->insert_after(prev_pos, x.begin(), x.end());
         x.clear();
      }
   }

   // Moves the element that follows prev to *this, inserting it immediately
   //  after pos.  This is constant time.
   void splice_after(iterator prev_pos, slist& x, iterator prev)
   {
      if((NodeAlloc&)*this == (NodeAlloc&)x){
         Node::splice_after(prev_pos.m_ptr, prev.m_ptr, prev.m_ptr->m_next);
      }
      else{
         this->insert_after(prev_pos, *prev);
         x.erase_after(prev);
      }
   }

   // Moves the range [before_first + 1, before_last + 1) to *this,
   //  inserting it immediately after pos.  This is constant time.
   void splice_after(iterator prev_pos,      slist& x, 
                     iterator before_first,  iterator before_last)
   {
      if((NodeAlloc&)*this == (NodeAlloc&)x){
         if (before_first != before_last){
            Node::splice_after(prev_pos.m_ptr, 
               before_first.m_ptr, before_last.m_ptr);
         }
      }
      else{
         this->insert_after(prev_pos, x.begin(), x.end());
         x.erase_after(before_first, before_last);
      }
   }

   // Linear in distance(begin(), pos), and linear in x.size().
   void splice(iterator pos, slist& x) 
   {
      if((NodeAlloc&)*this == (NodeAlloc&)x){
         if (!x.empty()){
            Node::splice_after
               (
                  Node::previous(this->m_node, pos.m_ptr),
                  x.m_node, 
                  Node::previous(x.m_node, x.m_node)
               );
         }
      }
      else{
         insert(pos, x.begin(), x.end());
         x.clear();
      }
   }

   // Linear in distance(begin(), pos), and in distance(x.begin(), i).
   void splice(iterator pos, slist& x, iterator i) 
   {
      if((NodeAlloc&)*this == (NodeAlloc&)x){
         iterator j = i;
         ++j;
         if (pos == i || pos == j) return;
         Node::splice_after
            (
               Node::previous(this->m_node, pos.m_ptr),
               Node::previous(x.m_node, i.m_ptr),
               i.m_ptr
            );
      }
      else{
         insert(pos, *i);
         x.erase(i);
      }
   }

   // Linear in distance(begin(), pos), in distance(x.begin(), first),
   // and in distance(first, last).
   void splice(iterator pos, slist& x, iterator first, iterator last)
   {
      if((NodeAlloc&)*this == (NodeAlloc&)x){
         if (first != last){
            Node::splice_after
               (
                  Node::previous(this->m_node, pos.m_ptr),
                  Node::previous(x.m_node, first.m_ptr),
                  Node::previous(first.m_ptr, last.m_ptr)
               );
         }
      }
      else{
         insert(pos, x.begin(), x.end());
         x.erase(first, last);
      }
   }

   void reverse() 
   { 
      if (this->m_node->m_next != this->m_node)
         this->m_node->m_next = Node::reverse(this->m_node->m_next, 
                                              this->m_node->m_next);
   }

   void remove(const T& value)
      {  this->remove_if(value_equal_to_this(value));  }

   template <class Pred> 
   void remove_if(Pred pred)
   {
      NodePtr cur = this->m_node;
      while (cur->m_next != this->m_node) {
         if (pred(cur->m_next->m_data))
            this->erase_after(cur);
         else
            cur = cur->m_next;
      }
   }

   void unique()
      {  this->unique(value_equal());  }

   template <class Pred> 
   void unique(Pred pred)
   {
      NodePtr cur = this->m_node->m_next;
      if (cur != this->m_node) {
         while (cur->m_next != this->m_node) {
            if (pred(cur->m_data, cur->m_next->m_data))
               this->erase_after(cur);
            else
               cur = cur->m_next;
         }
      }
   }

   void merge(slist& x)
      {  this->merge(x, value_less()); }

   template <class StrictWeakOrdering> 
   void merge(slist& x, StrictWeakOrdering comp)
   {
      NodePtr n1 = this->m_node;
      while ((n1->m_next != this->m_node) && !x.empty()) {
         if (comp(x.m_node->m_next->m_data, n1->m_next->m_data)){
            this->splice_after(iterator(n1), x, x.before_begin(), x.begin());
         }
         n1 = n1->m_next;
      }
  
      if (!x.empty()) {
         this->splice_after(iterator(n1), x);
      }
   }

   void sort()
      {  this->sort(value_less());  }

   template <class StrictWeakOrdering> 
   void sort(StrictWeakOrdering comp)
   {
      //Check if not empty or just 1 element
      if (!this->empty() && 
         this->m_node->m_next->m_next != this->m_node) {
         const allocator_type &alloc = static_cast<NodeAlloc>(*this);
         slist carry(alloc);
         vector<slist> counter(64, carry);
         int fill = 0;
         while (!empty()) {
            this->splice_after(iterator(carry.m_node), *this, 
                               iterator(this->m_node), 
                               iterator(this->m_node->m_next));
            int i = 0;
            while (i < fill && !counter[i].empty()) {
               counter[i].merge(carry, comp);
               carry.swap(counter[i]);
               ++i;
            }
            carry.swap(counter[i]);
            if (i == fill){
               ++fill;
            }
         }

         for (int i = 1; i < fill; ++i){
            counter[i].merge(counter[i-1], comp);
         }
         this->swap(counter[fill-1]);
      }
   }
 private:
   NodePtr erase_after(NodePtr prev_pos)
   {
      NodePtr toerase = prev_pos->m_next;
      NodePtr toerase_next = toerase->m_next;
      prev_pos->m_next = toerase_next;
      this->destroy_node(toerase);
      return toerase_next;
   }

   NodePtr erase_after(NodePtr before_first, 
                       NodePtr last_node)
   {
      NodePtr cur = before_first->m_next;
      while (cur != last_node) {
         NodePtr tmp = cur;
         cur = cur->m_next;
         this->destroy_node(tmp);
      }
      before_first->m_next = last_node;
      return last_node;
   }

   NodePtr insert_after(NodePtr prev_pos, const value_type& x = value_type()) 
   {
      return Node::make_link(prev_pos, this->create_node(x));
   }

   void insert_after_fill(NodePtr prev_pos, size_type n, const value_type& x) 
   {
      for (size_type i = 0; i < n; ++i)
         prev_pos = Node::make_link(prev_pos, this->create_node(x));
   }

   // Check whether it's an integral type.  If so, it's not an iterator.
   template <class InIter>
   void insert_after_range(NodePtr prev_pos, InIter first, InIter last) 
   {
      const bool aux_boolean = boost::is_integral<InIter>::value;
      typedef boost::mpl::bool_<aux_boolean> Result;
      this->insert_after_range(prev_pos, first, last, Result());
   }

   template <class Int>
   void insert_after_range(NodePtr prev_pos, Int n, Int x, boost::mpl::true_) 
      {  this->insert_after_fill(prev_pos, n, x);  }

   template <class InIter>
   void insert_after_range(NodePtr prev_pos, InIter first, InIter last, boost::mpl::false_) 
   {
      while (first != last) {
         prev_pos = Node::make_link(prev_pos, this->create_node(*first));
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
};

template <class T, class Alloc>
inline bool 
operator==(const slist<T,Alloc>& sL1, const slist<T,Alloc>& sL2)
{
   typedef typename slist<T,Alloc>::const_iterator const_iterator;
   const_iterator end1 = sL1.end();
   const_iterator end2 = sL2.end();

   const_iterator i1 = sL1.begin();
   const_iterator i2 = sL2.begin();
   while (i1 != end1 && i2 != end2 && *i1 == *i2) {
      ++i1;
      ++i2;
   }
   return i1 == end1 && i2 == end2;
}


template <class T, class Alloc>
inline bool
operator<(const slist<T,Alloc>& sL1, const slist<T,Alloc>& sL2)
{
   return lexicographical_compare(sL1.begin(), sL1.end(), 
                                 sL2.begin(), sL2.end());
}

template <class T, class Alloc>
inline bool 
operator!=(const slist<T,Alloc>& sL1, const slist<T,Alloc>& sL2) 
   {  return !(sL1 == sL2);   }

template <class T, class Alloc>
inline bool 
operator>(const slist<T,Alloc>& sL1, const slist<T,Alloc>& sL2) 
   {  return sL2 < sL1; }

template <class T, class Alloc>
inline bool 
operator<=(const slist<T,Alloc>& sL1, const slist<T,Alloc>& sL2)
   {  return !(sL2 < sL1); }

template <class T, class Alloc>
inline bool 
operator>=(const slist<T,Alloc>& sL1, const slist<T,Alloc>& sL2)
   {  return !(sL1 < sL2); }

template <class T, class Alloc>
inline void swap(slist<T,Alloc>& x, slist<T,Alloc>& y) 
   {  x.swap(y);  }

}} //namespace boost{  namespace interprocess{

// Specialization of insert_iterator so that insertions will be constant
// time rather than linear time.

//iG
//Ummm, I don't like to define things in namespace std, but 
//there is no other way
namespace std {

template <class T, class Alloc>
class insert_iterator<boost::interprocess::slist<T, Alloc> > 
{
 protected:
   typedef boost::interprocess::slist<T, Alloc> Container;
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
      : container(&x), iter(is_previous ? i : x.previous(i)) { }

   insert_iterator<Container>& 
      operator=(const typename Container::value_type& value) 
   { 
      iter = container->insert_after(iter, value);
      return *this;
   }
   insert_iterator<Container>& operator*() { return *this; }
   insert_iterator<Container>& operator++() { return *this; }
   insert_iterator<Container>& operator++(int) { return *this; }
};

}  //namespace std;



#include <boost/interprocess/detail/config_end.hpp>

#endif /* BOOST_INTERPROCESS_SLIST_HPP */
