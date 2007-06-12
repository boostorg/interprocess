//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
//
// This file comes from SGI's stl_list.h file. Modified by Ion Gaztanaga 2004
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

#ifndef BOOST_INTERPROCESS_LIST_HPP_
#define BOOST_INTERPROCESS_LIST_HPP_

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/detail/move.hpp>
#include <boost/iterator.hpp>
#include <boost/iterator/reverse_iterator.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/type_traits/has_trivial_destructor.hpp>
#include <boost/interprocess/detail/mpl.hpp>
#include <boost/intrusive/list.hpp>

#include <iterator>
#include <utility>
#include <memory>
#include <functional>
#include <algorithm>

namespace boost {
namespace interprocess {

/// @cond
namespace detail {

template <class T, class VoidPointer>
struct list_node
   :  public boost::intrusive::list_base_hook
         < boost::intrusive::tag
         , boost::intrusive::safe_link
         , VoidPointer>
{
   typedef boost::intrusive::list_base_hook
         < boost::intrusive::tag
         , boost::intrusive::safe_link
         , VoidPointer>   IlistData;

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   template<class Convertible>
   list_node(const Convertible &conv)
      : m_data(conv)
   {}
   #else
   template<class Convertible>
   list_node(Convertible &&conv)
      : m_data(forward<Convertible>(conv))
   {}
   #endif

   T m_data;
};

template<class A>
struct list_alloc
   :  public A::template rebind<list_node
            < typename A::value_type
            , typename detail::pointer_to_other<typename A::pointer, void>::type>
            >::other
{
   typedef list_alloc<A>                 self_t;
   typedef typename A::value_type                     value_type;
   typedef list_node
      <typename A::value_type
      ,typename detail::pointer_to_other
         <typename A::pointer, void>::type>           Node;
   typedef typename A::template rebind<Node>::other   NodeAlloc;
   typedef A                                          ValAlloc;
   typedef typename NodeAlloc::pointer                NodePtr;
   typedef detail::scoped_deallocator<NodeAlloc>      Deallocator;

   list_alloc(const ValAlloc &a) 
      : NodeAlloc(a)
   {}

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   list_alloc(const detail::moved_object<ValAlloc> &a) 
      : NodeAlloc(a.get())
   {}
   #else
   list_alloc(ValAlloc &&a) 
      : NodeAlloc(a)
   {}
   #endif

   list_alloc(const list_alloc &other)
      : NodeAlloc(other)
   {}

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   list_alloc(const detail::moved_object<list_alloc> &other)
      : NodeAlloc(move((NodeAlloc&)other.get()))
   {  this->swap(other.get());  }
   #else
   list_alloc(list_alloc &&other)
      : NodeAlloc(move((NodeAlloc&)other))
   {  this->swap(other);  }
   #endif

   ~list_alloc()
   {}

   typename NodeAlloc::size_type max_size() const
   {  return NodeAlloc::max_size();  }

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   template<class Convertible>
   static void construct(const NodePtr &ptr, const Convertible &value)
   {  new(detail::get_pointer(ptr)) Node(value);  }
   #else
   template<class Convertible>
   static void construct(const NodePtr &ptr, Convertible &&value)
   {  new(detail::get_pointer(ptr)) Node(forward<Convertible>(value));  }
   #endif

   static void destroy(const NodePtr &ptr)
   {  detail::get_pointer(ptr)->~Node();  }

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   template<class Convertible>
   NodePtr create_node(const Convertible& x)
   {
      NodePtr p = NodeAlloc::allocate(1);
      Deallocator node_deallocator(p, *this);
      self_t::construct(p, x);
      node_deallocator.release();
      return (p);
   }
   #else
   template<class Convertible>
   NodePtr create_node(Convertible &&x)
   {
      NodePtr p = NodeAlloc::allocate(1);
      Deallocator node_deallocator(p, *this);
      self_t::construct(p, forward<Convertible>(x));
      node_deallocator.release();
      return (p);
   }
   #endif

   void destroy_node(NodePtr node)
   {
      self_t::destroy(node);
      NodeAlloc::deallocate(node, 1);
   }

   void swap(list_alloc &x)
   {
      NodeAlloc& this_alloc   = static_cast<NodeAlloc&>(*this);
      NodeAlloc& other_alloc  = static_cast<NodeAlloc&>(x);

      if (this_alloc != other_alloc){
         detail::do_swap(this_alloc, other_alloc);
      }

      this->m_ilist.swap(x.m_ilist);
   }

   protected:

   struct destroyer
   {
      destroyer(list_alloc &holder)
         :  m_holder(holder)
      {}

      void operator()(NodePtr n) const
      {  m_holder.destroy_node(n);  }

      list_alloc &m_holder;
   };

   //The intrusive list
   typedef typename boost::intrusive::list
      <typename Node::IlistData::template value_traits<Node>
      ,true
      ,typename A::size_type> Ilist;
   Ilist m_ilist;

   Ilist &non_const_ilist() const
   {  return const_cast<Ilist&>(this->m_ilist);   }
};

}  //namespace detail {
/// @endcond

//! A list is a doubly linked list. That is, it is a Sequence that supports both
//! forward and backward traversal, and (amortized) constant time insertion and 
//! removal of elements at the beginning or the end, or in the middle. Lists have 
//! the important property that insertion and splicing do not invalidate iterators 
//! to list elements, and that even removal invalidates only the iterators that point 
//! to the elements that are removed. The ordering of iterators may be changed 
//! (that is, list<T>::iterator might have a different predecessor or successor 
//! after a list operation than it did before), but the iterators themselves will 
//! not be invalidated or made to point to different elements unless that invalidation 
//! or mutation is explicit.
template <class T, class A>
class list 
   : protected detail::list_alloc<A>
{
   /// @cond
   typedef detail::list_alloc<A>                      AllocHolder;
   typedef typename AllocHolder::NodePtr              NodePtr;
   typedef list <T, A>                                ThisType;
   typedef typename AllocHolder::NodeAlloc            NodeAlloc;
   typedef typename AllocHolder::ValAlloc             ValAlloc;
   typedef typename AllocHolder::Node                 Node;
   typedef typename AllocHolder::Ilist                Ilist;
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
      : public boost::iterator<std::bidirectional_iterator_tag, 
                                 value_type,         list_difference_type, 
                                 list_const_pointer, list_const_reference>
   {
      /// @cond
      protected:
      typename Ilist::iterator m_it;
      explicit const_iterator(typename Ilist::iterator it)  : m_it(it){}
      void prot_incr() { ++m_it; }
      void prot_decr() { --m_it; }

      private:
      typename Ilist::iterator get()
      {  return this->m_it;   }
      /// @endcond

      public:
      friend class list<T, A>;
      typedef list_difference_type        difference_type;

      //Constructors
      const_iterator()
         :  m_it()
      {}

      //Pointer like operators
      const_reference operator*()  const 
      { return  m_it->m_data;  }

      const_pointer   operator->() const 
      { return  const_pointer(&m_it->m_data); }

      //Increment / Decrement
      const_iterator& operator++()       
      { prot_incr();  return *this; }

      const_iterator operator++(int)      
      { typename Ilist::iterator tmp = m_it; ++*this; return const_iterator(tmp);  }

      const_iterator& operator--()
      {   prot_decr(); return *this;   }

      const_iterator operator--(int)
      {  typename Ilist::iterator tmp = m_it; --*this; return const_iterator(tmp); }

      //Comparison operators
      bool operator==   (const const_iterator& r)  const
      {  return m_it == r.m_it;  }

      bool operator!=   (const const_iterator& r)  const
      {  return m_it != r.m_it;  }
   };

   //! Iterator used to iterate through a list
   class iterator
      : public const_iterator
   {
      /// @cond
      private:
      explicit iterator(typename Ilist::iterator it)
         :  const_iterator(it)
      {}
   
      typename Ilist::iterator get()
      {  return this->m_it;   }
      /// @endcond

      public:
      friend class list<T, A>;
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
         { typename Ilist::iterator tmp = this->m_it; ++*this; return iterator(tmp); }
      
      iterator& operator--()
         {  this->prot_decr(); return *this;  }

      iterator operator--(int)
         {  iterator tmp = *this; --*this; return tmp; }
   };

   //! Iterator used to iterate backwards through a list. 
   typedef boost::reverse_iterator<iterator>        reverse_iterator;
   //! Const iterator used to iterate backwards through a list. 
   typedef boost::reverse_iterator<const_iterator>  const_reverse_iterator;

   //! <b>Effects</b>: Constructs a list taking the allocator as parameter.
   //! 
   //! <b>Throws</b>: If allocator_type's copy constructor throws.
   //! 
   //! <b>Complexity</b>: Constant.
   explicit list(const allocator_type &a = A()) 
      : AllocHolder(a)
   {}

//   list(size_type n)
//      : AllocHolder(move(allocator_type()))
//   {  this->resize(n);  }

   //! <b>Effects</b>: Constructs a list that will use a copy of allocator a
   //!   and inserts n copies of value.
   //!
   //! <b>Throws</b>: If allocator_type's default constructor or copy constructor
   //!   throws or T's default or copy constructor throws.
   //! 
   //! <b>Complexity</b>: Linear to n.
   list(size_type n, const T& value = T(), const A& a = A())
      : AllocHolder(a)
   {  this->insert(begin(), n, value);  }

   //! <b>Effects</b>: Copy constructs a list.
   //!
   //! <b>Postcondition</b>: x == *this.
   //! 
   //! <b>Throws</b>: If allocator_type's default constructor or copy constructor throws.
   //! 
   //! <b>Complexity</b>: Linear to the elements x contains.
   list(const list& x) 
      : AllocHolder(x)
   {  this->insert(begin(), x.begin(), x.end());   }

   //! <b>Effects</b>: Move constructor. Moves mx's resources to *this.
   //!
   //! <b>Throws</b>: If allocator_type's default constructor throws.
   //! 
   //! <b>Complexity</b>: Constant.
   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   list(const detail::moved_object<list> &x)
      : AllocHolder(move((AllocHolder&)x.get()))
   {}
   #else
   list(list &&x)
      : AllocHolder(move((AllocHolder&)x))
   {}
   #endif

   //! <b>Effects</b>: Constructs a list that will use a copy of allocator a
   //!   and inserts a copy of the range [first, last) in the list.
   //!
   //! <b>Throws</b>: If allocator_type's default constructor or copy constructor
   //!   throws or T's constructor taking an dereferenced InIt throws.
   //!
   //! <b>Complexity</b>: Linear to the range [first, last).
   template <class InpIt>
   list(InpIt first, InpIt last, const A &a = A())
      : AllocHolder(a)
   {  insert(begin(), first, last);  }

   //! <b>Effects</b>: Destroys the list. All stored values are destroyed
   //!   and used memory is deallocated.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of elements.
   ~list() 
   {  this->clear();  }

   //! <b>Effects</b>: Returns a copy of the internal allocator.
   //! 
   //! <b>Throws</b>: If allocator's copy constructor throws.
   //! 
   //! <b>Complexity</b>: Constant.
   allocator_type get_allocator() const
   {  return allocator_type(*this); }

   //! <b>Effects</b>: Erases all the elements of the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of elements in the list.
   void clear()
   {  this->m_ilist.clear_and_destroy(Destroyer(*this));  }

   //! <b>Effects</b>: Returns an iterator to the first element contained in the list.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   iterator begin()
   { return iterator(this->m_ilist.begin()); }

   //! <b>Effects</b>: Returns a const_iterator to the first element contained in the list.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_iterator begin() const
   {  return const_iterator(this->non_const_ilist().begin());   }

   //! <b>Effects</b>: Returns an iterator to the end of the list.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   iterator end()
   {  return iterator(this->m_ilist.end());  }

   //! <b>Effects</b>: Returns a const_iterator to the end of the list.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_iterator end() const
   {  return const_iterator(this->non_const_ilist().end());  }

   //! <b>Effects</b>: Returns a reverse_iterator pointing to the beginning 
   //! of the reversed list. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   reverse_iterator rbegin()
   {  return reverse_iterator(end());  }

   //! <b>Effects</b>: Returns a const_reverse_iterator pointing to the beginning 
   //! of the reversed list. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_reverse_iterator rbegin() const
   {  return const_reverse_iterator(end());  }

   //! <b>Effects</b>: Returns a reverse_iterator pointing to the end
   //! of the reversed list. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   reverse_iterator rend()
   {  return reverse_iterator(begin());   }

   //! <b>Effects</b>: Returns a const_reverse_iterator pointing to the end
   //! of the reversed list. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_reverse_iterator rend() const
   {  return const_reverse_iterator(begin());   }

   //! <b>Effects</b>: Returns true if the list contains no elements.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   bool empty() const 
   {  return !this->size();  }

   //! <b>Effects</b>: Returns the number of the elements contained in the list.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   size_type size() const 
   {   return this->m_ilist.size();   }

   //! <b>Effects</b>: Returns the largest possible size of the list.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   size_type max_size() const 
   {  return AllocHolder::max_size();  }

   //! <b>Effects</b>: Inserts a copy of t in the beginning of the list.
   //!
   //! <b>Throws</b>: If memory allocation throws or
   //!   T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   void push_front(const T& x)   
   {  this->insert(this->begin(), x);  }

   //! <b>Effects</b>: Constructs a new element in the beginning of the list
   //!   and moves the resources of t to this new element.
   //!
   //! <b>Throws</b>: If memory allocation throws.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   void push_front(const detail::moved_object<T>& x)   
   {  this->insert(this->begin(), x);  }
   #else
   void push_front(T &&x)   
   {  this->insert(this->begin(), move(x));  }
   #endif

   //! <b>Effects</b>: Removes the last element from the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   void push_back (const T& x)   
   {  this->insert(this->end(), x);    }

   //! <b>Effects</b>: Removes the first element from the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   void push_back (const detail::moved_object<T>& x)   
   {  this->insert(this->end(), x);    }
   #else
   void push_back (T &&x)   
   {  this->insert(this->end(), move(x));    }
   #endif

   //! <b>Effects</b>: Removes the first element from the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   void pop_front()              
   {  this->erase(this->begin());      }

   //! <b>Effects</b>: Removes the last element from the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   void pop_back()               
   {  iterator tmp = this->end(); this->erase(--tmp);  }

   //! <b>Requires</b>: !empty()
   //!
   //! <b>Effects</b>: Returns a reference to the first element 
   //!   from the beginning of the container.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   reference front()             
   { return *this->begin(); }

   //! <b>Requires</b>: !empty()
   //!
   //! <b>Effects</b>: Returns a const reference to the first element 
   //!   from the beginning of the container.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_reference front() const 
   { return *this->begin(); }

   //! <b>Requires</b>: !empty()
   //!
   //! <b>Effects</b>: Returns a reference to the first element 
   //!   from the beginning of the container.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   reference back()              
   { return *(--this->end()); }

   //! <b>Requires</b>: !empty()
   //!
   //! <b>Effects</b>: Returns a const reference to the first element 
   //!   from the beginning of the container.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_reference back()  const 
   { return *(--this->end()); }

   //! <b>Effects</b>: Inserts or erases elements at the end such that
   //!   the size becomes n. New elements are copy constructed from x.
   //!
   //! <b>Throws</b>: If memory allocation throws, or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to the difference between size() and new_size.
   void resize(size_type new_size, const T& x)
   {
      iterator i = this->begin(), iend = this->end();
      size_type len = this->size();
      
      if(len > new_size){
         size_type to_erase = len - new_size;
         while(to_erase--){
            --iend;
         }
         this->erase(iend, this->end());
      }
      else{
         this->insert(iend, new_size - len, x);
      }
   }

   //! <b>Effects</b>: Inserts or erases elements at the end such that
   //!   the size becomes n. New elements are default constructed.
   //!
   //! <b>Throws</b>: If memory allocation throws, or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to the difference between size() and new_size.
   void resize(size_type new_size)
   {
      iterator i = this->begin(), iend = this->end();
      size_type len = this->size();
      
      if(len > new_size){
         size_type to_erase = len - new_size;
         while(to_erase--){
            --iend;
         }
         this->erase(iend, this->end());
      }
      else{
         size_type n = new_size - len;
         while(n--){
            T default_constructed;
            if(boost::is_scalar<T>::value){
               //Value initialization hack. Fix me!
               new(&default_constructed)T();
            }
            this->push_back(move(default_constructed));
         }
      }
   }

   //! <b>Effects</b>: Swaps the contents of *this and x.
   //!   If this->allocator_type() != x.allocator_type()
   //!   allocators are also swapped.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   void swap(ThisType& x)
   {  AllocHolder::swap(x);   }

   //! <b>Effects</b>: Swaps the contents of *this and x.
   //!   If this->allocator_type() != x.allocator_type()
   //!   allocators are also swapped.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   //void swap(const detail::moved_object<ThisType>& x)
   //{  this->swap(x.get());  }

   //! <b>Effects</b>: Makes *this contain the same elements as x.
   //!
   //! <b>Postcondition</b>: this->size() == x.size(). *this contains a copy 
   //! of each of x's elements. 
   //!
   //! <b>Throws</b>: If memory allocation throws or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to the number of elements in x.
   ThisType& operator=(const ThisType& x)
   {
      if (this != &x) {
         this->assign(x.begin(), x.end());
      }
      return *this;
   }

   //! <b>Effects</b>: Move assignment. All mx's values are transferred to *this.
   //!
   //! <b>Postcondition</b>: x.empty(). *this contains a the elements x had
   //!   before the function.
   //!
   //! <b>Throws</b>: If allocator_type's copy constructor throws.
   //!
   //! <b>Complexity</b>: Constant.
   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   ThisType& operator=(const detail::moved_object<ThisType>& mx)
   {
      this->clear();
      this->swap(mx.get());
      return *this;
   }
   #else
   ThisType& operator=(ThisType &&mx)
   {
      this->clear();
      this->swap(mx);
      return *this;
   }
   #endif

   //! <b>Requires</b>: p must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Inserts n copies of x before p.
   //!
   //! <b>Throws</b>: If memory allocation throws or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to n.
   void insert(iterator p, size_type n, const T& x)
   { this->priv_fill_insert(p, n, x); }

   //! <b>Requires</b>: p must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Insert a copy of the [first, last) range before p.
   //!
   //! <b>Throws</b>: If memory allocation throws, T's constructor from a
   //!   dereferenced InpIt throws.
   //!
   //! <b>Complexity</b>: Linear to std::distance [first, last).
   template <class InpIt>
   void insert(iterator p, InpIt first, InpIt last) 
   {
      const bool aux_boolean = boost::is_integral<InpIt>::value;
      typedef bool_<aux_boolean> Result;
      this->priv_insert_dispatch(p, first, last, Result());
   }

   //! <b>Requires</b>: p must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Insert a copy of x before p.
   //!
   //! <b>Throws</b>: If memory allocation throws or x's copy constructor throws.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   iterator insert(iterator p, const T& x) 
   {
      NodePtr tmp = AllocHolder::create_node(x);
      return iterator(this->m_ilist.insert(p.get(), *tmp));
   }

   //! <b>Requires</b>: p must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Insert a new element before p with mx's resources.
   //!
   //! <b>Throws</b>: If memory allocation throws.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   iterator insert(iterator p, const detail::moved_object<T>& x) 
   {
      NodePtr tmp = AllocHolder::create_node(x);
      return iterator(this->m_ilist.insert(p.get(), *tmp));
   }
   #else
   iterator insert(iterator p, T &&x) 
   {
      NodePtr tmp = AllocHolder::create_node(move(x));
      return iterator(this->m_ilist.insert(p.get(), *tmp));
   }
   #endif

   //! <b>Requires</b>: p must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Erases the element at p p.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   iterator erase(iterator p) 
   {  return iterator(this->m_ilist.erase_and_destroy(p.get(), Destroyer(*this))); }

   //! <b>Requires</b>: first and last must be valid iterator to elements in *this.
   //!
   //! <b>Effects</b>: Erases the elements pointed by [first, last).
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the distance between first and last.
   iterator erase(iterator first, iterator last)
   {  return iterator(this->m_ilist.erase_and_destroy(first.get(), last.get(), Destroyer(*this))); }

   //! <b>Effects</b>: Assigns the n copies of val to *this.
   //!
   //! <b>Throws</b>: If memory allocation throws or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to n.
   void assign(size_type n, const T& val) 
   {  this->priv_fill_assign(n, val);  }

   //! <b>Effects</b>: Assigns the the range [first, last) to *this.
   //!
   //! <b>Throws</b>: If memory allocation throws or
   //!   T's constructor from dereferencing InpIt throws.
   //!
   //! <b>Complexity</b>: Linear to n.
   template <class InpIt>
   void assign(InpIt first, InpIt last) 
   {
      const bool aux_boolean = boost::is_integral<InpIt>::value;
      typedef bool_<aux_boolean> Result;
      this->priv_assign_dispatch(first, last, Result());
   }

   //! <b>Requires</b>: p must point to an element contained
   //!   by the list. x != *this
   //!
   //! <b>Effects</b>: Transfers all the elements of list x to this list, before the
   //!   the element pointed by p. No destructors or copy constructors are called.
   //!
   //! <b>Throws</b>: std::runtime_error if this' allocator and x's allocator
   //!   are not equal.
   //!
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Note</b>: Iterators of values obtained from list x now point to elements of
   //!    this list. Iterators of this list and all the references are not invalidated.
   void splice(iterator p, ThisType& x) 
   {
      if((NodeAlloc&)*this == (NodeAlloc&)x){
         this->m_ilist.splice(p.get(), x.m_ilist);
      }
      else{
         throw std::runtime_error("list::splice called with unequal allocators");
      }
   }

//   void splice(iterator p, const detail::moved_object<ThisType>& x) 
//   {  this->splice(p, x.get());  }

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
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Note</b>: Iterators of values obtained from list x now point to elements of this
   //!   list. Iterators of this list and all the references are not invalidated.
   void splice(iterator p, ThisType &x, iterator i) 
   {
      if((NodeAlloc&)*this == (NodeAlloc&)x){
         this->m_ilist.splice(p.get(), x.m_ilist, i.get());
      }
      else{
         throw std::runtime_error("list::splice called with unequal allocators");
      }
   }

//   void splice(iterator p, const detail::moved_object<ThisType> &x, iterator i) 
//   {  this->splice(p, x.get(), i);  }

   //! <b>Requires</b>: p must point to an element contained
   //!   by this list. first and last must point to elements contained in list x.
   //! 
   //! <b>Effects</b>: Transfers the range pointed by first and last from list x to this list, 
   //!   before the the element pointed by p. No destructors or copy constructors are called.
   //! 
   //! <b>Throws</b>: std::runtime_error if this' allocator and x's allocator
   //!   are not equal.
   //! 
   //! <b>Complexity</b>: Linear to the number of elements transferred.
   //! 
   //! <b>Note</b>: Iterators of values obtained from list x now point to elements of this
   //!   list. Iterators of this list and all the references are not invalidated.
   void splice(iterator p, ThisType &x, iterator first, iterator last) 
   {
      if((NodeAlloc&)*this == (NodeAlloc&)x){
         this->m_ilist.splice(p.get(), x.m_ilist, first.get(), last.get());
      }
      else{
         throw std::runtime_error("list::splice called with unequal allocators");
      }
   }

//   void splice(iterator p, detail::moved_object<ThisType> &x, iterator first, iterator last) 
//   {  return this->splice(p, x.get(), first, last);   }

   //! <b>Requires</b>: p must point to an element contained
   //!   by this list. first and last must point to elements contained in list x.
   //!   n == std::distance(first, last)
   //! 
   //! <b>Effects</b>: Transfers the range pointed by first and last from list x to this list, 
   //!   before the the element pointed by p. No destructors or copy constructors are called.
   //! 
   //! <b>Throws</b>: std::runtime_error if this' allocator and x's allocator
   //!   are not equal.
   //! 
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Note</b>: Iterators of values obtained from list x now point to elements of this
   //!   list. Iterators of this list and all the references are not invalidated.
   void splice(iterator p, ThisType &x, iterator first, iterator last, size_type n) 
   {
      if((NodeAlloc&)*this == (NodeAlloc&)x){
         this->m_ilist.splice(p.get(), x.m_ilist, first.get(), last.get(), n);
      }
      else{
         throw std::runtime_error("list::splice called with unequal allocators");
      }
   }

//   void splice(iterator p, detail::moved_object<ThisType> &x, iterator first, iterator last, size_type n) 
//   {  return this->splice(p, x.get(), first, last, n);   }

   //! <b>Effects</b>: Reverses the order of elements in the list. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: This function is linear time.
   //! 
   //! <b>Note</b>: Iterators and references are not invalidated
   void reverse()
   {  this->m_ilist.reverse(); }    

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
      this->m_ilist.remove_and_destroy_if(Predicate(pred), Destroyer(*this));
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
   template <class BinaryPredicate>
   void unique(BinaryPredicate binary_pred)
   {
      typedef ValueCompareToNodeCompare<BinaryPredicate> Predicate;
      this->m_ilist.unique_and_destroy(Predicate(binary_pred), Destroyer(*this));
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
   void merge(list<T, A>& x)
   {  this->merge(x, value_less());  }

   //! <b>Effects</b>: This function removes all of moved mx's elements and inserts them
   //!   in order into *this according to std::less<value_type>. The merge is stable; 
   //!   that is, if an element from *this is equivalent to one from x, then the element 
   //!   from *this will precede the one from x. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: This function is linear time: it performs at most
   //!   size() + x.size() - 1 comparisons.
   //!
   //! <b>Note</b>: Iterators and references to *this are not invalidated.
   //void merge(const detail::moved_object<list<T, A> >& x)
   //{  this->merge(x.get());  }

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
   void merge(list<T, A>& x, StrictWeakOrdering comp)
   {
      if((NodeAlloc&)*this == (NodeAlloc&)x){
         this->m_ilist.merge(x.m_ilist,
            ValueCompareToNodeCompare<StrictWeakOrdering>(comp));
      }
      else{
         throw std::runtime_error("list::merge called with unequal allocators");
      }
   }

   //! <b>Requires</b>: p must be a comparison function that induces a strict weak
   //!   ordering and both *this and x must be sorted according to that ordering
   //!   The lists x and *this must be distinct. 
   //! 
   //! <b>Effects</b>: This function removes all of moved mx's elements and inserts them
   //!   in order into *this. The merge is stable; that is, if an element from *this is 
   //!   equivalent to one from x, then the element from *this will precede the one from x. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: This function is linear time: it performs at most
   //!   size() + x.size() - 1 comparisons.
   //! 
   //! <b>Note</b>: Iterators and references to *this are not invalidated.
   //template <class StrictWeakOrdering>
   //void merge(const detail::moved_object<list<T, A> >& x, StrictWeakOrdering comp)
   //{  return this->merge(x.get(), comp);  }

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
      // nothing if the list has length 0 or 1.
      if (this->size() < 2)
         return;
      this->m_ilist.sort(ValueCompareToNodeCompare<StrictWeakOrdering>(comp));
   }

   /// @cond
   private:
   void priv_fill_insert(iterator p, size_type n, const T& x)
   {
      for ( ; n > 0; --n)
         this->insert(p, x);
   }

   template <class InputIter>
   void priv_insert_dispatch(iterator p,
                             InputIter first, InputIter last,
                             false_)
   {
      for ( ; first != last; ++first){
         this->insert(p, *first);
      }
   }

   template<class Integer>
   void priv_insert_dispatch(iterator p, Integer n, Integer x, true_) 
   {  this->priv_fill_insert(p, n, x);  }

   void priv_fill_assign(size_type n, const T& val) 
   {
      iterator i = this->begin(), iend = this->end();

      for ( ; i != iend && n > 0; ++i, --n)
         *i = val;
      if (n > 0)
         this->insert(end(), n, val);
      else
         this->erase(i, end());
   }

   template <class Integer>
   void priv_assign_dispatch(Integer n, Integer val, true_)
   {  this->priv_fill_assign((size_type) n, (T) val); }

   template <class InputIter>
   void priv_assign_dispatch(InputIter first2, InputIter last2,
                                       false_)
   {
      iterator first1   = this->begin();
      iterator last1    = this->end();
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
   /// @endcond
};

template <class T, class A>
inline bool operator==(const list<T,A>& x, const list<T,A>& y)
{
   if(x.size() != y.size()){
      return false;
   }
   typedef typename list<T,A>::const_iterator const_iterator;
   const_iterator end1 = x.end();

   const_iterator i1 = x.begin();
   const_iterator i2 = y.begin();
   while (i1 != end1 && *i1 == *i2) {
      ++i1;
      ++i2;
   }
   return i1 == end1;
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

#ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
template <class T, class A>
inline void swap(list<T, A>& x, list<T, A>& y)
{
  x.swap(y);
}

template <class T, class A>
inline void swap(const detail::moved_object<list<T, A> >& x, list<T, A>& y)
{
  x.get().swap(y);
}

template <class T, class A>
inline void swap(list<T, A>& x, const detail::moved_object<list<T, A> >& y)
{
  x.swap(y.get());
}
#else
template <class T, class A>
inline void swap(list<T, A> &&x, list<T, A> &&y)
{
  x.swap(y);
}

#endif

/// @cond
/*!This class is movable*/
template <class T, class A>
struct is_movable<list<T, A> >
{
   enum {   value = true };
};

/*!This class is movable*/
template <class T, class VoidPointer>
struct is_movable<detail::list_node<T, VoidPointer> >
{
   enum {   value = true };
};

/*!This class is movable*/
template <class A>
struct is_movable<detail::list_alloc<A> >
{
   enum {   value = true };
};

//!has_trivial_destructor_after_move<> == true_type
//!specialization for optimizations
template <class T, class A>
struct has_trivial_destructor_after_move<list<T, A> >
{
   enum {   value = has_trivial_destructor<A>::value  };
};
/// @endcond

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif // BOOST_INTERPROCESS_LIST_HPP_
