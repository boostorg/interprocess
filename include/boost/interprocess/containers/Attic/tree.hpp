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
// This file comes from SGI's stl_tree file. Modified by Ion Gaztañaga 2005.
// Renaming, isolating and porting to generic algorithms. Pointer typedef 
// set to allocator::pointer to allow placing it in shared memory.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_TREE_HPP
#define BOOST_INTERPROCESS_TREE_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/containers/detail/tree_func.hpp>
#include <boost/detail/allocator_utilities.hpp>
#include <boost/interprocess/detail/generic_cast.hpp>
#include <boost/iterator/reverse_iterator.hpp>
#include <boost/type_traits/has_trivial_constructor.hpp>
#include <boost/type_traits/has_trivial_destructor.hpp>
#include <boost/detail/no_exceptions_support.hpp>
#include <boost/mpl/if.hpp>

#include <iterator>
#include <algorithm>

//Red-black tree class, designed for use in implementing STL
//associative containers (set, multiset, map, and multimap). The
//insertion and deletion algorithms are based on those in Cormen,
//Leiserson, and Rivest, Introduction to Algorithms (MIT Press, 1990),
//except that
//
//(1) the header cell is maintained with links not only to the root
//but also to the leftmost node of the tree, to enable constant time
//this->begin(), and to the rightmost node of the tree, to enable linear time
//performance when used with the generic set algorithms (set_union,
//etc.);
//
//(2) when a node being deleted has two children its successor node is
//relinked into its place, rather than copied, so that the only
//iterators invalidated are those referring to the deleted node.

namespace boost { namespace interprocess { namespace detail {

template <class Alloc>
struct rb_tree_node
   : rb_tree_node_base<typename pointer_to_other<typename Alloc::pointer, void>::type>
{
   typedef rb_tree_node_base<typename pointer_to_other
      <typename Alloc::pointer, void>::type>          base_t;
   typedef typename boost::detail::allocator::
         rebind_to<Alloc, rb_tree_node<Alloc> >::type node_allocator_t;

   typedef typename base_t::basic_node_pointer        basic_node_pointer;
   typedef typename node_allocator_t::pointer         node_pointer;
   typedef typename Alloc::value_type                 value_type;
   typedef typename Alloc::const_pointer              const_pointer;
   typedef typename Alloc::pointer                    pointer;
   typedef typename Alloc::const_reference            const_reference;
   typedef typename Alloc::reference                  reference;
   typedef typename Alloc::difference_type            difference_type;

   rb_tree_node(const value_type & value) 
      : m_value(value)
   {}

   static node_pointer downcast(basic_node_pointer upclass_pointer/*const basic_node_pointer &upclass_pointer*/)
   {
      return boost::interprocess::cast_to<node_pointer>::using_static_cast(upclass_pointer);
//      return boost::interprocess::do_static_cast<node_pointer>(upclass_pointer);
   }

   const value_type &value() const   {  return m_value; }
         value_type &value()         {  return m_value; }
   private:
   value_type     m_value;
};

template <class T, class Alloc, bool convertible_construct>
class rb_tree_alloc_base
   //Inherit from node allocator
: public boost::detail::allocator::
            rebind_to<Alloc, rb_tree_node<Alloc> >::type,
   //Inherit from node_ptr allocator
  public boost::detail::allocator::
            rebind_to<Alloc, typename
               boost::detail::allocator::rebind_to
                  <Alloc, rb_tree_node<Alloc> >::type::value_type::basic_node_pointer>::type,
   //Inherit from value_ptr allocator
  public boost::detail::allocator::rebind_to<Alloc, T>::type
{

   public:
   typedef typename boost::detail::allocator::
         rebind_to<Alloc, rb_tree_node<Alloc> >::type       node_allocator_t;
   typedef typename node_allocator_t::value_type            node_val_t;
   typedef typename node_allocator_t::pointer               node_pointer;

   typedef typename boost::detail::allocator::
         rebind_to<Alloc, T>::type                          value_allocator_t;
   typedef typename value_allocator_t::value_type           value_val_t;
   typedef typename value_allocator_t::pointer              value_ptr_t;
   typedef typename value_allocator_t::const_pointer        value_cptr_t;
   typedef typename value_allocator_t::reference            value_ref_t;
   typedef typename value_allocator_t::const_reference      value_cref_t;

   typedef typename boost::detail::allocator::
         rebind_to<Alloc, typename node_val_t::
                    basic_node_pointer>::type               basic_node_ptr_allocator_t;
   typedef typename basic_node_ptr_allocator_t::pointer     basic_node_ptr_ptr_t;
   typedef typename basic_node_ptr_allocator_t::value_type  basic_node_ptr_t;

   typedef detail::scoped_deallocator<node_allocator_t>     NodeDeallocator;
   typedef detail::scoped_destructor <basic_node_ptr_allocator_t> BasicPtrDestructor;

   rb_tree_alloc_base(const value_allocator_t& a)
      :  node_allocator_t(a), 
         basic_node_ptr_allocator_t(a), 
         value_allocator_t(a)
      {  this->initialize_when_empty();   }
   
   rb_tree_alloc_base(const rb_tree_alloc_base &x)
      :  node_allocator_t(x), 
         basic_node_ptr_allocator_t(x), 
         value_allocator_t(x)
   {
      if (!x.m_header->parent()){
         this->initialize_when_empty();
      }
   }

   ~rb_tree_alloc_base()
      {  this->uninitialize_when_empty(); }

   node_pointer allocate_and_construct_node(value_cref_t x)
   {
      node_pointer p = node_allocator_t::allocate(1);
      scoped_ptr<node_val_t, NodeDeallocator>node_deallocator(p, *this);

      basic_node_ptr_ptr_t pleft(basic_node_ptr_allocator_t::address(p->left())), 
         pright(basic_node_ptr_allocator_t::address(p->right())),
         pparent(basic_node_ptr_allocator_t::address(p->parent()));

      //Make sure destructors are called before memory is freed
      //if an exception is thrown
      {
         basic_node_ptr_allocator_t::construct(pleft, basic_node_ptr_t());
         scoped_ptr<basic_node_ptr_t, BasicPtrDestructor>
            left_destroy(pleft, *this);

         basic_node_ptr_allocator_t::construct(pright, basic_node_ptr_t());
         scoped_ptr<basic_node_ptr_t, BasicPtrDestructor>
            right_destroy(pright, *this);

         basic_node_ptr_allocator_t::construct(pparent, basic_node_ptr_t());
         scoped_ptr<basic_node_ptr_t, BasicPtrDestructor>
            parent_destroy(pparent, *this);
         value_allocator_t::construct(value_allocator_t::address(p->value()), x);

         left_destroy.release();
         right_destroy.release();
         parent_destroy.release();
      }
      node_deallocator.release();
      return p;
   }

   void destroy_and_deallocate_node(node_pointer p)
   {
      node_allocator_t::destroy(p);
      node_allocator_t::deallocate(p, 1);
   }

   typename node_allocator_t::size_type max_size() const
      {  return node_allocator_t::max_size();  }

   void swap (rb_tree_alloc_base &x)
   {
      node_allocator_t& this_alloc  = static_cast<node_allocator_t&>(*this);
      node_allocator_t& other_alloc = static_cast<node_allocator_t&>(x);
      if (this_alloc != other_alloc){
         detail::do_swap(static_cast<value_allocator_t&>(*this), 
                               static_cast<value_allocator_t&>(x));
         detail::do_swap(static_cast<basic_node_ptr_allocator_t&>(*this), 
                               static_cast<basic_node_ptr_allocator_t&>(x));
         detail::do_swap(this_alloc, other_alloc);
      }
      detail::do_swap(this->m_header, x.m_header);     
   }

   private:
   void initialize_when_empty()
   {
      this->m_header = node_allocator_t::allocate(1); 
      scoped_ptr<node_val_t, NodeDeallocator>node_deallocator(this->m_header, *this);

      basic_node_ptr_ptr_t pleft(basic_node_ptr_allocator_t::address(this->m_header->left())), 
         pright(basic_node_ptr_allocator_t::address(this->m_header->right())),
         pparent(basic_node_ptr_allocator_t::address(this->m_header->parent()));

      //Make sure destructors are called before memory is freed
      //if an exception is thrown
      {
         basic_node_ptr_allocator_t::construct(pleft, basic_node_ptr_t());
         scoped_ptr<basic_node_ptr_t, BasicPtrDestructor>
            left_destroy(pleft, *this);

         basic_node_ptr_allocator_t::construct(pright, basic_node_ptr_t());
         scoped_ptr<basic_node_ptr_t, BasicPtrDestructor>
            right_destroy(pleft, *this);

         basic_node_ptr_allocator_t::construct(pparent, basic_node_ptr_t());

         left_destroy.release();
         right_destroy.release();
      }
      //m_value is not constructed since it is not used
      //m_color is POD so we don't need to construct it

      this->m_header->color() = node_val_t::red_color; // used to distinguish header from 
      // root, in iterator.operator++
      this->m_header->parent() = 0;
      this->m_header->left()  = this->m_header;
      this->m_header->right() = this->m_header;
      node_deallocator.release();
   }

   void uninitialize_when_empty()
   {
      //Destructors must not throw
      basic_node_ptr_allocator_t::destroy(basic_node_ptr_allocator_t::address(this->m_header->left()));
      basic_node_ptr_allocator_t::destroy(basic_node_ptr_allocator_t::address(this->m_header->right()));
      basic_node_ptr_allocator_t::destroy(basic_node_ptr_allocator_t::address(this->m_header->parent()));
      //m_color is POD so we don't need to destroy it
      //m_value was not constructed so we don't need to destroy it
      node_allocator_t::deallocate(this->m_header, 1);
   }

 protected:
   node_pointer m_header;
};

template <class T, class Alloc>
class rb_tree_alloc_base<T, Alloc, true>
   //Inherit from node allocator
   : public boost::detail::allocator::
         rebind_to<Alloc, rb_tree_node<Alloc> >::type
{
   public:
   typedef typename boost::detail::allocator::
      rebind_to<Alloc, rb_tree_node<Alloc> >::type       node_allocator_t;
   typedef typename node_allocator_t::value_type         node_val_t;
   typedef typename node_allocator_t::pointer            node_pointer;

   typedef typename boost::detail::allocator::
      rebind_to<Alloc, T>::type                          value_allocator_t;
   typedef typename value_allocator_t::value_type        value_val_t;
   typedef typename value_allocator_t::pointer           value_ptr_t;
   typedef typename value_allocator_t::const_pointer     value_cptr_t;
   typedef typename value_allocator_t::reference         value_ref_t;
   typedef typename value_allocator_t::const_reference   value_cref_t;

   typedef typename boost::detail::allocator::
         rebind_to<Alloc, typename node_val_t::
                    basic_node_pointer>::type               basic_node_ptr_allocator_t;
   typedef typename basic_node_ptr_allocator_t::pointer     basic_node_ptr_ptr_t;
   typedef typename basic_node_ptr_allocator_t::value_type  basic_node_ptr_t;

   typedef detail::scoped_deallocator<node_allocator_t>  NodeDeallocator;
   typedef detail::scoped_destructor <basic_node_ptr_allocator_t> BasicPtrDestructor;

   rb_tree_alloc_base(const value_allocator_t& a)
      :  node_allocator_t(a)
      {  this->initialize_when_empty();   }

   rb_tree_alloc_base(const rb_tree_alloc_base &x)
      :  node_allocator_t(x)
      {  this->initialize_when_empty();   }

   ~rb_tree_alloc_base()
      {  this->uninitialize_when_empty(); }

   template<class Convertible>
      node_pointer allocate_and_construct_node(const Convertible &x)
   {
      node_pointer p = node_allocator_t::allocate(1);
      scoped_ptr<node_val_t, NodeDeallocator>node_deallocator(p, *this);
      node_allocator_t::construct(p, x);      
      node_deallocator.release();
      return p;
   }

   void destroy_and_deallocate_node(node_pointer p)
   {
      node_allocator_t::destroy(p);
      node_allocator_t::deallocate(p, 1);
   }

   typename node_allocator_t::size_type max_size() const
      {  return node_allocator_t::max_size();  }

   void swap (rb_tree_alloc_base &x)
   {
      node_allocator_t& this_alloc  = static_cast<node_allocator_t&>(*this);
      node_allocator_t& other_alloc = static_cast<node_allocator_t&>(x);
      if (this_alloc != other_alloc){
         detail::do_swap(this_alloc, other_alloc);
      }
      detail::do_swap(this->m_header, x.m_header);     
   }

   private:
   void initialize_when_empty()
   {
      this->m_header = node_allocator_t::allocate(1); 

      //If the pointer type a has trivial constructor we can avoid this
      if(!boost::has_trivial_constructor<node_pointer>::value){
         basic_node_ptr_allocator_t node_ptr_allocator(*this);
         scoped_ptr<node_val_t, NodeDeallocator>node_deallocator(this->m_header, *this);
         basic_node_ptr_ptr_t pleft(node_ptr_allocator.address(this->m_header->left())), 
            pright(node_ptr_allocator.address(this->m_header->right())),
            pparent(node_ptr_allocator.address(this->m_header->parent()));

         //Make sure destructors are called before memory is freed
         //if an exception is thrown
         {
            node_ptr_allocator.construct(pleft, basic_node_ptr_t());
            scoped_ptr<basic_node_ptr_t, BasicPtrDestructor>
               left_destroy(pleft, node_ptr_allocator);

            node_ptr_allocator.construct(pright, basic_node_ptr_t());
            scoped_ptr<basic_node_ptr_t, BasicPtrDestructor>
               right_destroy(pright, node_ptr_allocator);

            node_ptr_allocator.construct(pparent, basic_node_ptr_t());

            left_destroy.release();
            right_destroy.release();
         }
         node_deallocator.release();
      }

      //m_value is not constructed since it is not used
      //m_color is POD so we don't need to construct it
      this->m_header->color() = node_val_t::red_color; // used to distinguish header from 
      // root, in iterator.operator++
      this->m_header->parent() = 0;
      this->m_header->left()  = this->m_header;
      this->m_header->right() = this->m_header;
   }

   void uninitialize_when_empty()
   {
      //If the pointer type a has trivial destructor we can avoid this
      if(!boost::has_trivial_destructor<node_pointer>::value){
         basic_node_ptr_allocator_t node_ptr_allocator(*this);
         //Destructors must not throw
         node_ptr_allocator.destroy(node_ptr_allocator.address(this->m_header->left()));
         node_ptr_allocator.destroy(node_ptr_allocator.address(this->m_header->right()));
         node_ptr_allocator.destroy(node_ptr_allocator.address(this->m_header->parent()));
      }
      //m_color is POD so we don't need to destroy it
      //m_value was not constructed so we don't need to destroy it
      node_allocator_t::deallocate(this->m_header, 1);
   }

   protected:
   node_pointer m_header;
};

template <class Key, class Value, class KeyOfValue, 
          class Compare, class Alloc>
class rb_tree
   : protected rb_tree_alloc_base<Value, Alloc, 
               has_convertible_construct<Alloc>::value>
{
 private:
   typedef rb_tree_alloc_base<Value, Alloc, 
               has_convertible_construct<Alloc>::value> base_t;
   typedef typename base_t::node_pointer              node_pointer;
   typedef typename base_t::node_val_t                node_val_t;
   typedef typename node_val_t::basic_node_pointer    basic_node_pointer;
   typedef rb_tree_algo<node_val_t>                   rb_tree_algo_t;
   typedef rb_tree_func<node_val_t, Key,
                        KeyOfValue, Compare >         rb_tree_func_t;

   public:
   typedef Key                                        key_type;
   typedef Value                                      value_type;
   typedef typename base_t::value_ptr_t               pointer;
   typedef typename base_t::value_cptr_t              const_pointer;
   typedef typename base_t::value_ref_t               reference;
   typedef typename base_t::value_cref_t              const_reference;
   typedef typename base_t::value_allocator_t         allocator_type;
   typedef typename allocator_type::size_type         size_type;
   typedef typename allocator_type::difference_type   difference_type;
   typedef rb_tree_const_iterator<node_val_t>         const_iterator;
   typedef rb_tree_iterator<node_val_t>               iterator;
	typedef boost::reverse_iterator<iterator>          reverse_iterator;
	typedef boost::reverse_iterator<const_iterator>    const_reverse_iterator;

   rb_tree(const Compare& comp = Compare(), 
           const allocator_type& a = allocator_type())
      : base_t(a), m_data(comp, 0)
      { }

   rb_tree(const rb_tree& x) 
      :  base_t(x),
         m_data(x.get_compare(), 0)
   { 
      if (x.root()){
         this->m_header->color() = node_val_t::red_color;
         this->root()= 
            this->copy_node(node_val_t::downcast(x.m_header->parent()), 
                            this->m_header);
         this->leftmost()  = 
            rb_tree_algo_t::minimum_node(node_val_t::downcast(this->root()));
         this->rightmost() = 
            rb_tree_algo_t::maximum_node(node_val_t::downcast(this->root()));
      }

      this->m_data.m_node_count = x.m_data.m_node_count;
   }

   ~rb_tree() { this->clear(); }

   rb_tree&  operator=(const rb_tree& x)
   {
      if (this != &x) {
         // Note that Key may be a constant type.
         this->clear();
         this->m_data.m_node_count = 0;
         this->get_compare() = x.get_compare();        
         if (!x.root()) {
            this->root() = 0;
            this->leftmost()  = this->m_header;
            this->rightmost() = this->m_header;
         }
         else{
            this->root()      = this->copy_node(x.root(), this->m_header);
            this->leftmost()  = 
               rb_tree_algo_t::minimum_node(node_val_t::downcast(this->root()));
            this->rightmost() = 
               rb_tree_algo_t::maximum_node(node_val_t::downcast(this->root()));
            this->m_data.m_node_count = x.m_data.m_node_count;
         }
      }
      return *this;
   }

   public:    
   // accessors:
   Compare key_comp() const 
      { return this->get_compare(); }

   allocator_type get_allocator() const 
      { return allocator_type(*this); }

   iterator begin() 
      { return iterator(this->leftmost()); }

   const_iterator begin() const 
      { return const_iterator(this->leftmost()); }

   iterator end() 
      { return iterator(this->m_header); }

   const_iterator end() const 
      { return const_iterator(this->m_header); }

   reverse_iterator rbegin() 
      { return reverse_iterator(this->end()); }

   const_reverse_iterator rbegin() const 
      {  return const_reverse_iterator(this->end());  }

   reverse_iterator rend() 
      { return reverse_iterator(this->begin()); }

   const_reverse_iterator rend() const 
      { return const_reverse_iterator(this->begin()); } 

   bool empty() const 
      { return !this->m_data.m_node_count; }

   size_type size() const 
      { return this->m_data.m_node_count; }

   size_type max_size() const 
      { return base_t::max_size(); }

   void swap(rb_tree& t) 
   {
      detail::do_swap(this->m_data.m_node_count, t.m_data.m_node_count);
      detail::do_swap(this->get_compare(), t.get_compare());
      base_t::swap(t);
   }
    
   public:
   // insert/erase
   std::pair<iterator,bool> insert_unique(const value_type& v)
   {
      KeyOfValue  key_of_value;
      node_pointer  out;
      typename rb_tree_func_t::insert_unique_context context;
      if(!rb_tree_func_t::insert_unique_prepare
            (this->m_header, this->m_data, key_of_value(v), out, context)){
         return std::pair<iterator,bool>(iterator(out), false);
      }
      node_pointer new_node = this->allocate_and_construct_node(v);      
      ++this->m_data.m_node_count;
      rb_tree_func_t::insert_unique_commit
         (this->m_header, this->m_data, new_node, out, context);
      return std::pair<iterator,bool>(iterator(out), true);
   }

   iterator insert_unique(iterator hint, const value_type& v)
   {
      KeyOfValue  key_of_value;
      node_pointer out;
      typename rb_tree_func_t::insert_unique_context  context;
      if(!rb_tree_func_t::insert_unique_hint_prepare
            (this->m_header, this->m_data, hint.get_ptr(), 
             key_of_value(v), out, context)){
         return iterator(out);
      }
      node_pointer new_node = this->allocate_and_construct_node(v);
      ++this->m_data.m_node_count;
      rb_tree_func_t::insert_unique_commit(this->m_header, this->m_data, new_node, out, context);
      return iterator(out);
   }

   template <class InputIterator>
   void insert_unique(InputIterator first, InputIterator last)
   {
      for ( ; first != last; ++first)
         this->insert_unique(*first);
   }

   iterator insert_equal(const value_type& v)
   {
      node_pointer tmp = this->allocate_and_construct_node(v);
      rb_tree_func_t::insert_equal(this->m_header, this->m_data, tmp);
      ++this->m_data.m_node_count;
      return iterator(tmp);
   }

   iterator insert_equal(iterator hint, const value_type& v)
   {
      node_pointer tmp = this->allocate_and_construct_node(v);
      rb_tree_func_t::insert_equal_hint(this->m_header, this->m_data, 
                                        hint.get_ptr(), tmp);
      ++this->m_data.m_node_count;
      return iterator(tmp);
   }

   template <class InputIterator>
   void insert_equal(InputIterator first, InputIterator last)
   {
      for ( ; first != last; ++first)
         this->insert_equal(*first);
   }

   iterator erase(const_iterator position)
   {
      iterator ret(position.get_ptr());
      ++ret;
      node_pointer y = node_val_t::downcast(rb_tree_func_t::erase_node(this->m_header, position.get_ptr()));
      this->destroy_and_deallocate_node(y);
      --this->m_data.m_node_count;
      return ret;
   }

   size_type erase(const key_type& x)
   {
      std::pair<iterator,iterator> p = this->equal_range(x);
      size_type n = std::distance(p.first, p.second);
      this->erase(p.first, p.second);
      return n;
   }

   iterator erase(const_iterator first, const_iterator last)
   {
      if (first == this->begin() && last == this->end()){
         this->clear();
         return begin();
      }
      else{
         while (first != last){
            first = this->erase(first);
         }
         return iterator(last.get_ptr());
      }
   }

   void clear() 
   {
      if (this->m_data.m_node_count) {
         this->erase_node(node_val_t::downcast(this->root()));
         this->leftmost()  = this->m_header;
         this->root()      = 0;
         this->rightmost() = this->m_header;
         this->m_data.m_node_count  = 0;
      }
   }

   // set operations:
   iterator find(const key_type& k)
      {  return iterator(rb_tree_func_t::find(this->m_header, this->m_data, k));   }

   const_iterator find(const key_type& k) const
      {  return const_iterator(rb_tree_func_t::find(this->m_header, this->m_data, k));   }

   size_type count(const key_type& k) const
      {  return size_type(rb_tree_func_t::count(this->m_header, this->m_data, k)); }

   iterator lower_bound(const key_type& k)
      {  return iterator(rb_tree_func_t::lower_bound(this->m_header, this->m_data, k));  }

   const_iterator lower_bound(const key_type& k) const
      {  return const_iterator(rb_tree_func_t::lower_bound(this->m_header, this->m_data, k));  }

   iterator upper_bound(const key_type& k)
      {  return iterator (rb_tree_func_t::upper_bound(this->m_header, this->m_data, k)); }

   const_iterator upper_bound(const key_type& k) const
      {  return const_iterator (rb_tree_func_t::upper_bound(this->m_header, this->m_data, k)); }

   std::pair<iterator,iterator> equal_range(const key_type& k)
   {  
      node_pointer lower, upper;
      rb_tree_func_t::equal_range(this->m_header, this->m_data, k, lower, upper);
      return std::pair<iterator, iterator>(iterator(lower), iterator(upper));   
   }

   std::pair<const_iterator, const_iterator> equal_range(const key_type& k) const
   {  
      node_pointer lower, upper;
      rb_tree_func_t::equal_range(this->m_header, this->m_data, k, lower, upper);
      return std::pair<const_iterator, const_iterator>(const_iterator(lower), const_iterator(upper));   
   }

 private:

   basic_node_pointer &root() const 
      { return this->m_header->parent(); }

   basic_node_pointer &leftmost() const 
      { return this->m_header->left(); }

   basic_node_pointer &rightmost() const 
      { return this->m_header->right(); }


   node_pointer clone_node(basic_node_pointer x)
   {
      basic_node_pointer tmp = this->allocate_and_construct_node(node_val_t::downcast(x)->value());
      tmp->color() = x->color();
      tmp->left()  = 0;
      tmp->right() = 0;
      return node_val_t::downcast(tmp);
   }

   const Compare & get_compare() const
      {  return static_cast<const Compare&>(m_data);  }

   Compare & get_compare()
      {  return static_cast<Compare&>(m_data);  }

   static const Key& get_node_key(node_pointer x)
      { return KeyOfValue()(x->value()); }

   node_pointer copy_node(basic_node_pointer x, basic_node_pointer p)
   {
      // structural copy.  x and p must be non-null.
      node_pointer top = this->clone_node(x);
      top->parent() = p;
       
      BOOST_TRY {
         if (x->right())
            top->right() = this->copy_node(x->right(), top);
         p = top;
         x = x->left();

         while (x) {
            node_pointer y = this->clone_node(x);
            p->left() = y;
            y->parent() = p;
            if (x->right()){
               y->right() = this->copy_node(x->right(), y);
            }
            p = y;
            x = x->left();
         }
      }
      BOOST_CATCH(...){
         this->erase_node(top);
         BOOST_RETHROW;
      }
      BOOST_CATCH_END

      return top;
   }

   void erase_node(node_pointer x)
   {
      // erase without rebalancing
      while (x) {
         this->erase_node(node_val_t::downcast(x->right()));
         node_pointer y = node_val_t::downcast(x->left());
         this->destroy_and_deallocate_node(x);
         x = y;
      }
   }

 private:
   struct Data
      : public Compare
   {
      Data(const Compare &comp, size_type node_count)
         : Compare(comp), m_node_count(node_count){}
      size_type m_node_count; // keeps track of size of tree
   } m_data;
};

template <class Key, class Value, class KeyOfValue, 
          class Compare, class Alloc>
inline bool 
operator==(const rb_tree<Key,Value,KeyOfValue,Compare,Alloc>& x, 
           const rb_tree<Key,Value,KeyOfValue,Compare,Alloc>& y)
{
  return x.size() == y.size() &&
         std::equal(x.begin(), x.end(), y.begin());
}

template <class Key, class Value, class KeyOfValue, 
          class Compare, class Alloc>
inline bool 
operator<(const rb_tree<Key,Value,KeyOfValue,Compare,Alloc>& x, 
          const rb_tree<Key,Value,KeyOfValue,Compare,Alloc>& y)
{
  return std::lexicographical_compare(x.begin(), x.end(), 
                                      y.begin(), y.end());
}

template <class Key, class Value, class KeyOfValue, 
          class Compare, class Alloc>
inline bool 
operator!=(const rb_tree<Key,Value,KeyOfValue,Compare,Alloc>& x, 
           const rb_tree<Key,Value,KeyOfValue,Compare,Alloc>& y) {
  return !(x == y);
}

template <class Key, class Value, class KeyOfValue, 
          class Compare, class Alloc>
inline bool 
operator>(const rb_tree<Key,Value,KeyOfValue,Compare,Alloc>& x, 
          const rb_tree<Key,Value,KeyOfValue,Compare,Alloc>& y) {
  return y < x;
}

template <class Key, class Value, class KeyOfValue, 
          class Compare, class Alloc>
inline bool 
operator<=(const rb_tree<Key,Value,KeyOfValue,Compare,Alloc>& x, 
           const rb_tree<Key,Value,KeyOfValue,Compare,Alloc>& y) {
  return !(y < x);
}

template <class Key, class Value, class KeyOfValue, 
          class Compare, class Alloc>
inline bool 
operator>=(const rb_tree<Key,Value,KeyOfValue,Compare,Alloc>& x, 
           const rb_tree<Key,Value,KeyOfValue,Compare,Alloc>& y) {
  return !(x < y);
}


template <class Key, class Value, class KeyOfValue, 
          class Compare, class Alloc>
inline void 
swap(rb_tree<Key,Value,KeyOfValue,Compare,Alloc>& x, 
     rb_tree<Key,Value,KeyOfValue,Compare,Alloc>& y)
{
  x.swap(y);
}

} //namespace detail {
} //namespace interprocess  {
} //namespace boost  {

#include <boost/interprocess/detail/config_end.hpp>

#endif //BOOST_INTERPROCESS_TREE_HPP

