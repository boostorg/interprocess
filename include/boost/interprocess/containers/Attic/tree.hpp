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

#include <boost/interprocess/detail/move.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/containers/detail/tree_func.hpp>
#include <boost/interprocess/smart_ptr/scoped_ptr.hpp>
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

   rb_tree_node(const detail::moved_object<value_type> & value) 
      : m_value(move(value.get()))
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

template <class T, class Alloc>
class rb_tree_alloc_base/*<T, Alloc, true>*/
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

   typedef typename node_val_t::base_t                  base_node_t;

   typedef detail::scoped_deallocator<node_allocator_t>  NodeDeallocator;
   typedef detail::scoped_destructor <basic_node_ptr_allocator_t> BasicPtrDestructor;

   rb_tree_alloc_base(const value_allocator_t& a)
      :  node_allocator_t(a)
      {  this->initialize_when_empty();   }

   rb_tree_alloc_base(const rb_tree_alloc_base &x)
      :  node_allocator_t(x)
      {  this->initialize_when_empty();   }

   rb_tree_alloc_base(const detail::moved_object<rb_tree_alloc_base> &x)
      :  node_allocator_t(move((node_allocator_t&)x.get()))
      {  this->initialize_when_empty();   this->swap(x.get()); }

   ~rb_tree_alloc_base()
      {  this->uninitialize_when_empty(); }

   template<class Convertible>
   static void construct(const node_pointer &ptr, const Convertible &value)
   {  new(detail::get_pointer(ptr)) node_val_t(value);  }

   template<class Convertible1, class Convertible2>
   static void construct(const node_pointer &ptr, 
                         const detail::moved_object<std::pair<Convertible1, Convertible2> > &value)
   {  
      //std::pair is not movable so we define our own type and overwrite it
      typedef detail::pair<typename node_val_t::value_type::first_type
                          ,typename node_val_t::value_type::second_type> hack_pair_t;

      typedef typename boost::detail::allocator::
            rebind_to<Alloc, hack_pair_t>::type hack_pair_allocator_t;

      typedef rb_tree_node<hack_pair_allocator_t>  hack_node_t;

      new((void*)detail::get_pointer(ptr)) hack_node_t(value);  
   }

   static void destroy(const node_pointer &ptr)
   {  detail::get_pointer(ptr)->~node_val_t();  }

   template<class Convertible>
      node_pointer allocate_and_construct_node(const Convertible &x)
   {
      node_pointer p = node_allocator_t::allocate(1);
      scoped_ptr<node_val_t, NodeDeallocator>node_deallocator(p, *this);
//      node_allocator_t::construct(p, x);      
      construct(p, x);
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

      std::swap(this->m_end, x.m_end);

      if (this->m_end.parent())
         this->m_end.parent()->parent() = this->end_node();
      else
         this->m_end.left() = this->m_end.right() = this->end_node();

      if (x.m_end.parent())
         x.m_end.parent()->parent() = x.end_node();
      else
         x.m_end.left() = x.m_end.right() = x.end_node();
   /*
      node_allocator_t& this_alloc  = static_cast<node_allocator_t&>(*this);
      node_allocator_t& other_alloc = static_cast<node_allocator_t&>(x);
      if (this_alloc != other_alloc){
         detail::do_swap(this_alloc, other_alloc);
      }
      detail::do_swap(this->m_header, x.m_header);     
*/
   }

   private:
   void initialize_when_empty()
   {
/*
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
*/
      //m_value is not constructed since it is not used
      //m_color is POD so we don't need to construct it.
      //used to distinguish header from root, in iterator.operator++
      this->m_end.color()  = node_val_t::red_color; 
      this->m_end.parent() = 0;
      this->m_end.left()   = this->end_node();
      this->m_end.right()  = this->end_node();

/*
      //m_value is not constructed since it is not used
      //m_color is POD so we don't need to construct it
      this->m_header->color() = node_val_t::red_color; // used to distinguish header from 
      // root, in iterator.operator++
      this->m_header->parent() = 0;
      this->m_header->left()  = this->m_header;
      this->m_header->right() = this->m_header;
*/
   }

   void uninitialize_when_empty()
   {/*
      if(this->m_header){
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
      }*/
   }

   protected:

   basic_node_ptr_t end_node() const
   {  return basic_node_ptr_t(const_cast<base_node_t *>(&m_end));  }

   base_node_t m_end;
//   node_pointer m_header;
   
};

template <class Key, class Value, class KeyOfValue, 
          class Compare, class Alloc>
class rb_tree
   : protected rb_tree_alloc_base<Value, Alloc/*, 
               has_convertible_construct<Alloc>::value*/>
{
 private:
   typedef rb_tree_alloc_base<Value, Alloc/*, 
               has_convertible_construct<Alloc>::value*/> base_t;
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
         this->m_end.color() = node_val_t::red_color;
         this->root()= 
            this->copy_node(node_val_t::downcast(this->m_end.parent()), 
                            this->end_node());
         this->leftmost()  = 
            rb_tree_algo_t::minimum_node(node_val_t::downcast(this->root()));
         this->rightmost() = 
            rb_tree_algo_t::maximum_node(node_val_t::downcast(this->root()));
      }

      this->m_data.m_node_count = x.m_data.m_node_count;
   }

   rb_tree(const detail::moved_object<rb_tree>& x) 
      :  base_t(move((base_t&)x.get())),
         m_data(move(x.get().get_compare()), 0)
   { 
      x.get().m_data.m_node_count = 0;
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
            this->leftmost()  = this->end_node();
            this->rightmost() = this->end_node();
         }
         else{
            this->root()      = this->copy_node(x.root(), this->end_node());
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
      {  basic_node_pointer a;
         iterator b(a);

         return iterator(this->leftmost()); }

   const_iterator begin() const 
      { return const_iterator(this->leftmost()); }

   iterator end() 
      { return iterator(this->end_node()); }

   const_iterator end() const 
      { return const_iterator(this->end_node()); }

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

   void swap(const detail::moved_object<rb_tree>& mt) 
   {  this->swap(mt.get());   }

   public:
   // insert/erase
   std::pair<iterator,bool> insert_unique(const value_type& v)
   {
      KeyOfValue  key_of_value;
      typename rb_tree_func_t::insert_unique_context context;
      if(!rb_tree_func_t::insert_unique_prepare
            (node_val_t::downcast(this->end_node()), this->m_data, key_of_value(v), context)){
         return std::pair<iterator,bool>(iterator(context.node), false);
      }
      node_pointer new_node = this->allocate_and_construct_node(v);      
      ++this->m_data.m_node_count;
      rb_tree_func_t::insert_unique_commit
         (node_val_t::downcast(this->end_node()), this->m_data, new_node, context);
      return std::pair<iterator,bool>(iterator(new_node), true);
   }

   template<class MovableConvertible>
   std::pair<iterator,bool> insert_unique(const detail::moved_object<MovableConvertible>& mv)
   {
      MovableConvertible &v = mv.get();
      KeyOfValue  key_of_value;
      typename rb_tree_func_t::insert_unique_context context;
      if(!rb_tree_func_t::insert_unique_prepare
            (node_val_t::downcast(this->end_node()), this->m_data, key_of_value(v), context)){
         return std::pair<iterator,bool>(iterator(context.node), false);
      }
      node_pointer new_node = this->allocate_and_construct_node(mv);      
      ++this->m_data.m_node_count;
      rb_tree_func_t::insert_unique_commit
         (node_val_t::downcast(this->end_node()), this->m_data, new_node, context);
      return std::pair<iterator,bool>(iterator(new_node), true);
   }

   iterator insert_unique(iterator hint, const value_type& v)
   {
      KeyOfValue  key_of_value;
      typename rb_tree_func_t::insert_unique_context  context;
      if(!rb_tree_func_t::insert_unique_hint_prepare
            (node_val_t::downcast(this->end_node()), this->m_data, hint.get_ptr(), 
             key_of_value(v), context)){
         return iterator(context.node);
      }
      node_pointer new_node = this->allocate_and_construct_node(v);
      ++this->m_data.m_node_count;
      rb_tree_func_t::insert_unique_commit(node_val_t::downcast(this->end_node()), this->m_data, new_node, context);
      return iterator(new_node);
   }

   template<class MovableConvertible>
   iterator insert_unique(iterator hint, const detail::moved_object<MovableConvertible> &mv)
   {
      MovableConvertible &v = mv.get();
      KeyOfValue  key_of_value;
      typename rb_tree_func_t::insert_unique_context  context;
      if(!rb_tree_func_t::insert_unique_hint_prepare
            (node_val_t::downcast(this->end_node()), this->m_data, hint.get_ptr(), 
             key_of_value(v), context)){
         return iterator(context.node);
      }
      node_pointer new_node = this->allocate_and_construct_node(mv);
      ++this->m_data.m_node_count;
      rb_tree_func_t::insert_unique_commit(node_val_t::downcast(this->end_node()), this->m_data, new_node, context);
      return iterator(new_node);
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
      rb_tree_func_t::insert_equal(node_val_t::downcast(this->end_node()), this->m_data, tmp);
      ++this->m_data.m_node_count;
      return iterator(tmp);
   }

   template<class MovableConvertible>
   iterator insert_equal(const detail::moved_object<MovableConvertible> &mv)
   {
      node_pointer tmp = this->allocate_and_construct_node(mv);
      rb_tree_func_t::insert_equal(node_val_t::downcast(this->end_node()), this->m_data, tmp);
      ++this->m_data.m_node_count;
      return iterator(tmp);
   }

   iterator insert_equal(iterator hint, const value_type& v)
   {
      node_pointer tmp = this->allocate_and_construct_node(v);
      rb_tree_func_t::insert_equal_hint(node_val_t::downcast(this->end_node()), this->m_data, 
                                        hint.get_ptr(), tmp);
      ++this->m_data.m_node_count;
      return iterator(tmp);
   }

   template<class MovableConvertible>
   iterator insert_equal(iterator hint, const detail::moved_object<MovableConvertible> &mv)
   {
      node_pointer tmp = this->allocate_and_construct_node(mv);
      rb_tree_func_t::insert_equal_hint(node_val_t::downcast(this->end_node()), this->m_data, 
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
      node_pointer y = node_val_t::downcast
         (rb_tree_func_t::erase_node(node_val_t::downcast(this->end_node()), position.get_ptr()));
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
/*
         this->m_end.color()  = node_val_t::red_color; 
         this->m_end.parent() = 0;
         this->m_end.left()   = this->end_node();
         this->m_end.right()  = this->end_node();
*/
         this->m_end.color()  = node_val_t::red_color; 
         this->root()      = 0;
         this->leftmost()  = this->end_node();
         this->rightmost() = this->end_node();

         this->m_data.m_node_count  = 0;
      }
   }

   // set operations:
   iterator find(const key_type& k)
      {  return iterator(rb_tree_func_t::find(node_val_t::downcast(this->end_node()), this->m_data, k));   }

   const_iterator find(const key_type& k) const
      {  return const_iterator(rb_tree_func_t::find(node_val_t::downcast(this->end_node()), this->m_data, k));   }

   size_type count(const key_type& k) const
      {  return size_type(rb_tree_func_t::count(node_val_t::downcast(this->end_node()), this->m_data, k)); }

   iterator lower_bound(const key_type& k)
      {  return iterator(rb_tree_func_t::lower_bound(node_val_t::downcast(this->end_node()), this->m_data, k));  }

   const_iterator lower_bound(const key_type& k) const
      {  return const_iterator(rb_tree_func_t::lower_bound(node_val_t::downcast(this->end_node()), this->m_data, k));  }

   iterator upper_bound(const key_type& k)
      {  return iterator (rb_tree_func_t::upper_bound(node_val_t::downcast(this->end_node()), this->m_data, k)); }

   const_iterator upper_bound(const key_type& k) const
      {  return const_iterator (rb_tree_func_t::upper_bound(node_val_t::downcast(this->end_node()), this->m_data, k)); }

   std::pair<iterator,iterator> equal_range(const key_type& k)
   {  
      node_pointer lower, upper;
      rb_tree_func_t::equal_range(node_val_t::downcast(this->end_node()), this->m_data, k, lower, upper);
      return std::pair<iterator, iterator>(iterator(lower), iterator(upper));   
   }

   std::pair<const_iterator, const_iterator> equal_range(const key_type& k) const
   {  
      node_pointer lower, upper;
      rb_tree_func_t::equal_range(node_val_t::downcast(this->end_node()), this->m_data, k, lower, upper);
      return std::pair<const_iterator, const_iterator>(const_iterator(lower), const_iterator(upper));   
   }

 private:
   basic_node_pointer &root() const 
      { return const_cast<basic_node_pointer &>(this->m_end.parent()); }

   basic_node_pointer &leftmost() const 
      { return const_cast<basic_node_pointer &>(this->m_end.left()); }

   basic_node_pointer &rightmost() const 
      { return const_cast<basic_node_pointer &>(this->m_end.right()); }

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

      Data(const detail::moved_object<Compare> &comp, size_type node_count)
         : Compare(move(comp.get())), m_node_count(node_count){}

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

/*!This class is movable*/
template <class T, class V, class K, class C, class A>
struct is_movable<detail::rb_tree<T, V, K, C, A> >
{
   enum {   value = true };
};

/*!This class is movable*/
template <class A>
struct is_movable<detail::rb_tree_node<A> >
{
   enum {   value = true };
};

/*!This class is movable*/
template <class T, class A>
struct is_movable<detail::rb_tree_alloc_base<T, A/*, true*/> >
{
   enum {   value = true };
};


} //namespace interprocess  {
} //namespace boost  {

#include <boost/interprocess/detail/config_end.hpp>

#endif //BOOST_INTERPROCESS_TREE_HPP

