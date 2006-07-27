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
// This file comes from SGI's stl_list.h file. Modified by Ion Gaztañaga 2004
// Renaming, isolating and porting to generic algorithms. Pointer typedef 
// set to allocator::pointer to allow placing it in shared memory.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_LIST_HPP_
#define BOOST_INTERPROCESS_LIST_HPP_

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/iterator.hpp>
#include <boost/iterator/reverse_iterator.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/detail/allocator_utilities.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/type_traits/has_trivial_constructor.hpp>
#include <boost/type_traits/has_trivial_destructor.hpp>
#include <boost/mpl/if.hpp>

#include <utility>
#include <memory>
#include <functional>
#include <algorithm>

namespace boost {

namespace interprocess {

namespace detail {

template <class A>
struct interprocess_list_node;

template <class A>
struct interprocess_list_node_base
{
   typedef typename boost::detail::allocator::
      rebind_to<A, interprocess_list_node<A> >::type      NodeAlloc;
   typedef typename NodeAlloc::pointer             pointer;

   pointer  m_next;
   pointer  m_prev;

   static void reverse(pointer &head)
   {
      pointer tmp = head;
      do {
         detail::do_swap(tmp->m_next, tmp->m_prev);
         tmp = tmp->m_prev;     // Old next node is now prev.
      } while (tmp != head);
   }
   
   static std::size_t size(const pointer &head)
   {  
      std::size_t count = 0;
      for(pointer node = head; node->m_next != head; node = node->m_next){
         ++count;
      }
      return count;
   }
};

template <class A>
struct interprocess_list_node
   : public interprocess_list_node_base<A>
{
   typedef typename boost::detail::allocator::
      rebind_to<A, interprocess_list_node<A> >::type   NodeAlloc;
   typedef typename NodeAlloc::pointer          pointer;
   typedef typename A::value_type               value_t;

   interprocess_list_node(const value_t &value)
      : m_data(value){}

   value_t  m_data;
};

template<class T, class A, bool convertible_construct>
struct interprocess_list_alloc
   :  public boost::detail::allocator::
               rebind_to<A, interprocess_list_node<A> >::type,
      public boost::detail::allocator::
            rebind_to<A, typename boost::detail::allocator::
               rebind_to<A, interprocess_list_node<A> >::
                  type::pointer >::type,
      public A
{
   typedef interprocess_list_node<A>                         Node;
   typedef typename boost::detail::allocator::
      rebind_to<A, interprocess_list_node<A> >::type         NodeAlloc;
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

   interprocess_list_alloc(const ValAlloc &a) 
      : NodeAlloc(a), PtrAlloc(a), ValAlloc(a) 
      {  priv_init();   }

   interprocess_list_alloc(const interprocess_list_alloc &other)
      : NodeAlloc(other), PtrAlloc(other), ValAlloc(other)
      {  priv_init();   }

   ~interprocess_list_alloc()
      {  this->priv_destroy();   }

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
            NodePtrPtr  pnext(PtrAlloc::address(m_node->m_next)), 
               pprev(PtrAlloc::address(m_node->m_prev));
              PtrAlloc::construct(pnext, m_node);
            scoped_ptr<NodePtr, PtrDestructor> next_destroy(pnext, *this);
              PtrAlloc::construct(pprev, m_node);
            next_destroy.release();
         }
         node_deallocator.release();
      }
      else{
         m_node->m_next = m_node;
         m_node->m_prev = m_node;
      }
   }

   void priv_destroy()
   {
      if(!boost::has_trivial_constructor<NodePtr>::value){
         PtrAlloc::destroy(&m_node->m_next);
         PtrAlloc::destroy(&m_node->m_prev);
      }
      NodeAlloc::deallocate(m_node, 1); 
   }

   template <class Convertible>
   NodePtr create_node(NodePtr next, NodePtr prev, const Convertible& x)
   {
      NodePtr p = NodeAlloc::allocate(1);
      scoped_ptr<Node, Deallocator>node_deallocator(m_node, *this);

      //Make sure destructors are called before memory is freed
      //if an exception is thrown
      {
         if(!boost::has_trivial_constructor<NodePtr>::value){
            typedef typename PtrAlloc::pointer NodePtrPtr;
            NodePtrPtr  pnext(PtrAlloc::address(p->m_next)), 
               pprev(PtrAlloc::address(p->m_prev));
            PtrAlloc::construct(pnext, next);
            scoped_ptr<NodePtr, PtrDestructor> next_destroy(pnext, *this);

            PtrAlloc::construct(pprev, prev);

            scoped_ptr<NodePtr, PtrDestructor> prev_destroy(pprev, *this);
         
            ValAlloc::construct(ValAlloc::address(p->m_data), x);

            next_destroy.release();
            prev_destroy.release();
         }
         else{
            p->m_next = next;
            p->m_prev = prev;
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
   
   void swap(interprocess_list_alloc &x)
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
struct interprocess_list_alloc<T, A, true>
   :  public boost::detail::allocator::
         rebind_to<A, interprocess_list_node<A> >::type
{
   typedef interprocess_list_node<A>                         Node;
   typedef typename boost::detail::allocator::
      rebind_to<A, interprocess_list_node<A> >::type         NodeAlloc;
   typedef typename boost::detail::allocator::
      rebind_to<A, typename NodeAlloc::pointer>::type PtrAlloc;
   typedef A                                          ValAlloc;
   typedef typename NodeAlloc::pointer                NodePtr;
   typedef detail::scoped_deallocator<NodeAlloc>      Deallocator;
   typedef detail::scoped_destructor<PtrAlloc>         PtrDestructor;

   enum {   
      node_has_trivial_destructor =  
         boost::has_trivial_destructor<NodePtr>::value |
         boost::has_trivial_destructor<T>::value 
   };

   interprocess_list_alloc(const ValAlloc &a) 
      : NodeAlloc(a)
   {  this->priv_init();   }

   interprocess_list_alloc(const interprocess_list_alloc &other)
      : NodeAlloc(other)
   {  this->priv_init();   }

  ~interprocess_list_alloc()
   {  this->priv_destroy();   }

   typename NodeAlloc::size_type max_size() const
      {  return NodeAlloc::max_size();  }

   template<class Convertible>
   NodePtr create_node(NodePtr next, NodePtr prev, const Convertible& x)
   {
      NodePtr p = NodeAlloc::allocate(1);
      scoped_ptr<Node, Deallocator>node_deallocator(p, *this);
      NodeAlloc::construct(p, x);
      node_deallocator.release();
      p->m_next = next;
      p->m_prev = prev;
      return (p);
   }

   void destroy_node(NodePtr node)
   {
      if(!node_has_trivial_destructor){
         NodeAlloc::destroy(node);
      }
      NodeAlloc::deallocate(node, 1);
   }

   void swap(interprocess_list_alloc &x)
   {
      NodeAlloc& this_alloc = static_cast<NodeAlloc&>(*this);
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
         NodePtrPtr  pnext(ptr_alloc.address(m_node->m_next)), 
            pprev(ptr_alloc.address(m_node->m_prev));

         //Make sure destructors are called before memory is freed
         //if an exception is thrown
         {
            ptr_alloc.construct(pnext, m_node);
            scoped_ptr<NodePtr, PtrDestructor> next_destroy(pnext, ptr_alloc);

            ptr_alloc.construct(pprev, m_node);
            next_destroy.release();
         }
         node_deallocator.release();
      }
      else{
         m_node->m_next = m_node;
         m_node->m_prev = m_node;
      }
   }

   void priv_destroy()
   {
      if(!boost::has_trivial_destructor<NodePtr>::value){
         PtrAlloc ptr_alloc(*this);
         ptr_alloc.destroy(ptr_alloc.address(m_node->m_next));
         ptr_alloc.destroy(ptr_alloc.address(m_node->m_prev));
      }
      NodeAlloc::deallocate(m_node, 1); 
   }
};

}  //namespace detail {

template <class T, class A>
class list 
   : protected detail::interprocess_list_alloc<T, A, 
      has_convertible_construct<A>::value>
{
   typedef detail::interprocess_list_alloc<T, A,
      has_convertible_construct< A>::value>   AllocHolder;
   typedef typename AllocHolder::NodePtr              NodePtr;
   typedef list <T, A>                                ThisType;
   typedef typename AllocHolder::NodeAlloc            NodeAlloc;
   typedef typename AllocHolder::PtrAlloc             PtrAlloc;
   typedef typename AllocHolder::ValAlloc             ValAlloc;
   typedef detail::interprocess_list_node<A>                 Node;

public:
   typedef A                                       allocator_type;
   typedef T                                       value_type;
   typedef typename A::pointer                     pointer;
   typedef typename A::const_pointer               const_pointer;
   typedef typename A::reference                   reference;
   typedef typename A::const_reference             const_reference;
   typedef typename A::size_type                   size_type;
   typedef typename A::difference_type             difference_type;
   typedef difference_type                         list_difference_type;
   typedef pointer                                 list_pointer;
   typedef const_pointer                           list_const_pointer;
   typedef reference                               list_reference;
   typedef const_reference                         list_const_reference;

public:
   //list const_iterator
    class const_iterator
      : public boost::iterator<std::bidirectional_iterator_tag, 
                                 value_type,         list_difference_type, 
                                 list_const_pointer, list_const_reference>
    {
    private:
      const NodePtr &get_ptr() const   {  return   m_ptr;  }
      NodePtr &get_ptr()               {  return   m_ptr;  }

    protected:
      NodePtr m_ptr;
        explicit const_iterator(NodePtr ptr)  : m_ptr(ptr){}
      void prot_incr() { m_ptr = m_ptr->m_next; }
      void prot_decr() { m_ptr = m_ptr->m_prev; }

     public:
      friend class list<T, A>;
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

        const_iterator& operator--()
            {   prot_decr(); return *this;   }

        const_iterator operator--(int)
            {  NodePtr tmp = m_ptr; --*this; return const_iterator(tmp); }

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
    protected:
        explicit iterator(NodePtr ptr) : const_iterator(ptr){}

     public:
      friend class list<T, A>;
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
        
      iterator& operator--()
        {  this->prot_decr(); return *this;  }

        iterator operator--(int)
          {  iterator tmp = *this; --*this; return tmp; }
    };

    typedef boost::reverse_iterator<iterator>        reverse_iterator;
    typedef boost::reverse_iterator<const_iterator>  const_reverse_iterator;

   explicit list(const allocator_type &a = A()) 
      : AllocHolder(a)
      {}

   list(size_type n, const T& value = T(), const A& a = A())
      : AllocHolder(a)
      {  insert(begin(), n, value);  }

   list(const list& x) 
      : AllocHolder(x)
      {  insert(begin(), x.begin(), x.end());   }

   template <class InpIt>
   list(InpIt first, InpIt last, const A &a = A())
      : AllocHolder(a)
      {  insert(begin(), first, last);  }

  ~list() 
      {  clear();  }

   allocator_type get_allocator() const { return allocator_type(*this); }

   void clear() 
   {
      NodePtr cur = this->m_node->m_next;
      while (cur != this->m_node) {
         NodePtr tmp = cur;
         cur = cur->m_next;
         AllocHolder::destroy_node(tmp);
      }
      this->m_node->m_next = this->m_node;
      this->m_node->m_prev = this->m_node;
   }
   iterator begin()            
      { return iterator(this->m_node->m_next); }

   const_iterator begin() const
      { return const_iterator(this->m_node->m_next); }

   iterator end()              
      { return iterator(this->m_node); }

   const_iterator end()   const
      { return const_iterator(this->m_node); }

   reverse_iterator rbegin() 
      { return reverse_iterator(end()); }

   const_reverse_iterator rbegin() const 
      { return const_reverse_iterator(end()); }

   reverse_iterator rend()
      { return reverse_iterator(begin()); }

   const_reverse_iterator rend() const
      { return const_reverse_iterator(begin()); }

   bool empty() const 
      { return this->m_node->m_next == this->m_node; }

   size_type size() const 
      {   return Node::size(this->m_node);  }

   size_type max_size() const 
      { return AllocHolder::max_size(); }

   void push_front(const T& x)   
      {  insert(begin(), x);  }

   void push_back (const T& x)   
      {  insert(end(), x);    }

   void pop_front()              
      {  erase(begin());      }

   void pop_back()               
      {  iterator tmp = end(); erase(--tmp);  }

   reference front()             
      { return *begin(); }

   const_reference front() const 
      { return *begin(); }

   reference back()              
      { return *(--end()); }

   const_reference back()  const 
      { return *(--end()); }

   void resize(size_type new_size, const T& x = T())
   {
      iterator i = begin();
      size_type len = 0;
      for ( ; i != end() && len < new_size; ++i, ++len){}

      if (len == new_size)
         erase(i, end());
      else                          // i == end()
         insert(end(), new_size - len, x);
   }

   void swap(ThisType& x)
      {  AllocHolder::swap(x);   }

   ThisType& operator=(const ThisType& x)
   {
      if (this != &x) {
         this->assign(x.begin(), x.end());
      }
      return *this;
   }

   void insert(iterator pos, size_type n, const T& x)
      { this->priv_fill_insert(pos, n, x); }

   template <class InpIt>
   void insert(iterator pos, InpIt first, InpIt last) 
   {
      const bool aux_boolean = boost::is_integral<InpIt>::value;
      typedef boost::mpl::bool_<aux_boolean> Result;
      this->priv_insert_dispatch(pos, first, last, Result());
   }

   iterator insert(iterator position, const T& x) 
   {
      NodePtr tmp = AllocHolder::create_node(position.get_ptr(),
                                             position.get_ptr()->m_prev,
                                             x);
      position.get_ptr()->m_prev->m_next  = tmp;
      position.get_ptr()->m_prev          = tmp;
      return iterator(tmp);
   }

   iterator erase(const_iterator position) 
   {
      NodePtr next_node = position.get_ptr()->m_next;
      NodePtr prev_node = position.get_ptr()->m_prev;
      NodePtr node      = position.get_ptr();
      prev_node->m_next = next_node;
      next_node->m_prev = prev_node;
      AllocHolder::destroy_node(node);
      return iterator(next_node);
   }

   iterator erase(const_iterator first, const_iterator last)
   {
      while (first != last)
         this->erase(first++);
      return iterator(last.get_ptr());
   }

   // assign(), a generalized assignment member function.  Two
   // versions: one that takes a count, and one that takes a range.
   // The range version is a member template, so we dispatch on whether
   // or not the type is an integer.
   void assign(size_type n, const T& val) 
      {  this->priv_fill_assign(n, val);  }

   template <class InpIt>
   void assign(InpIt first, InpIt last) 
   {
      const bool aux_boolean = boost::is_integral<InpIt>::value;
      typedef boost::mpl::bool_<aux_boolean> Result;
      this->priv_assign_dispatch(first, last, Result());
   }

   void splice(iterator position, ThisType& x) 
   {
      if((NodeAlloc&)*this == (NodeAlloc&)x){
         if (!x.empty()) 
            this->priv_transfer(position.get_ptr(), x.m_node->m_next, x.m_node);
      }
      else{
           this->insert(position, x.begin(), x.end());
           x.clear();
      }
   }

   void splice(iterator position, ThisType &x, iterator i) 
   {
      if((NodeAlloc&)*this == (NodeAlloc&)x){
         iterator j = i;
         ++j;
         if (position == i || position == j) return;
         this->priv_transfer(position.get_ptr(), i.get_ptr(), j.get_ptr());
      }
      else{
           this->insert(position, *i);
           x.erase(i);
      }
   }

   void splice(iterator position, ThisType &x, iterator first, iterator last) 
   {
      if((NodeAlloc&)*this == (NodeAlloc&)x){
         if (first != last) 
            this->priv_transfer(position.get_ptr(), first.get_ptr(), last.get_ptr());
      }
      else{
           this->insert(position, x.begin(), x.end());
           x.erase(first, last);
      }
   }

   void remove(const T& value)
   {
      iterator first = begin();
      iterator last = end();
      while (first != last) {
         iterator next = first;
         ++next;
         if (*first == value) 
            this->erase(first);
         first = next;
      }
   }

   void reverse() 
      {  Node::reverse(this->m_node); }    

   template <class Pred>
   void remove_if(Pred pred)
   {
      iterator first = begin();
      iterator last = end();
      while (first != last) {
         iterator next = first;
         ++next;
         if (pred(*first)) 
            this->erase(first);
         first = next;
      }
   }

   void unique()
      {  this->unique(value_equal());  }

   template <class BinaryPredicate>
   void unique(BinaryPredicate binary_pred)
   {
      iterator first = begin();
      iterator last = end();
      if (first == last) return;
      iterator next = first;
      while (++next != last) {
         if (binary_pred(*first, *next))
            erase(next);
         else
            first = next;
         next = first;
      }
   }

   void merge(list<T, A>& x)
      {  this->merge(x, value_less());  }

   template <class StrictWeakOrdering>
   void merge(list<T, A>& x, StrictWeakOrdering comp)
   {
      iterator first1   = begin();
      iterator last1    = end();
      iterator first2   = x.begin();
      iterator last2    = x.end();
      while (first1 != last1 && first2 != last2){
         if (comp(*first2, *first1)) {
            iterator next = first2;
            this->splice(first1, x, first2, ++next);
            first2 = next;
         }
         else{
            ++first1;
         }
      }
      if (first2 != last2) 
         this->splice(last1, x, first2, last2);
   }

   void sort()
      {  this->sort(value_less());  }

   template <class StrictWeakOrdering>
   void sort(StrictWeakOrdering comp)
   {
      // Do nothing if the list has length 0 or 1.
      if (this->m_node->m_next != this->m_node && 
          this->m_node->m_next->m_next != this->m_node) {
         ThisType carry(this->get_allocator());
         vector<ThisType> counter;
         counter.resize(64, carry);

         int fill = 0;
         while (!this->empty()) {
            carry.splice(carry.begin(), *this, begin());
            int i = 0;
            while(i < fill && !counter[i].empty()) {
               counter[i].merge(carry, comp);
               carry.swap(counter[i++]);
            }
            carry.swap(counter[i]);         
            if (i == fill) ++fill;
         } 

         for (int i = 1; i < fill; ++i) 
            counter[i].merge(counter[i-1], comp);
         this->swap(counter[fill-1]);
      }
   }

 private:

   void priv_fill_insert(iterator position, size_type n, const T& x)
   {
      for ( ; n > 0; --n)
         this->insert(position, x);
   }

   template <class InputIter>
   void priv_insert_dispatch(iterator position,
                             InputIter first, InputIter last,
                             boost::mpl::false_)
   {
      for ( ; first != last; ++first){
         this->insert(position, *first);
      }
   }

   template<class Integer>
   void priv_insert_dispatch(iterator pos, Integer n, Integer x, boost::mpl::true_) 
   {
      this->priv_fill_insert(pos, n, x);
   }

   void priv_fill_assign(size_type n, const T& val) 
   {
      iterator i = begin();
      for ( ; i != end() && n > 0; ++i, --n)
         *i = val;
      if (n > 0)
         this->insert(end(), n, val);
      else
         this->erase(i, end());
   }

   template <class Integer>
   void priv_assign_dispatch(Integer n, Integer val, boost::mpl::true_)
      {  this->priv_fill_assign((size_type) n, (T) val); }

   void priv_transfer(NodePtr position, NodePtr first, NodePtr last) 
   {
      if (position != last) {
         // Remove [first, last) from its old position.
         last->m_prev->m_next     = position;
         first->m_prev->m_next    = last;
         position->m_prev->m_next = first; 

         // Splice [first, last) into its new position.
         NodePtr tmp      = position->m_prev;
         position->m_prev = last->m_prev;
         last->m_prev     = first->m_prev; 
         first->m_prev    = tmp;
      }
   }

   template <class InputIter>
   void priv_assign_dispatch(InputIter first2, InputIter last2,
                                       boost::mpl::false_)
   {
      iterator first1   = begin();
      iterator last1    = end();
      for ( ; first1 != last1 && first2 != last2; ++first1, ++first2)
         *first1 = *first2;
      if (first2 == last2)
         this->erase(first1, last1);
      else
         this->insert(last1, first2, last2);
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
};


template <class T, class A>
inline bool operator==(const list<T,A>& x, const list<T,A>& y)
{
  typedef typename list<T,A>::const_iterator const_iterator;
  const_iterator end1 = x.end();
  const_iterator end2 = y.end();

  const_iterator i1 = x.begin();
  const_iterator i2 = y.begin();
  while (i1 != end1 && i2 != end2 && *i1 == *i2) {
    ++i1;
    ++i2;
  }
  return i1 == end1 && i2 == end2;
}

template <class T, class A>
inline bool operator<(const list<T,A>& x,
                      const list<T,A>& y)
{
  return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
}

template <class T, class A>
inline bool operator!=(const list<T,A>& x, const list<T,A>& y) 
{
  return !(x == y);
}

template <class T, class A>
inline bool operator>(const list<T,A>& x, const list<T,A>& y) 
{
  return y < x;
}

template <class T, class A>
inline bool operator<=(const list<T,A>& x, const list<T,A>& y) 
{
  return !(y < x);
}

template <class T, class A>
inline bool operator>=(const list<T,A>& x, const list<T,A>& y) 
{
  return !(x < y);
}

template <class T, class A>
inline void swap(list<T, A>& x, list<T, A>& y)
{
  x.swap(y);
}

}  //namespace interprocess {

}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif // BOOST_INTERPROCESS_LIST_HPP_
