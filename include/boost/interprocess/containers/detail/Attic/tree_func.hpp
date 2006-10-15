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

#ifndef BOOST_INTERPROCESS_TREE_FUNC_HPP
#define BOOST_INTERPROCESS_TREE_FUNC_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/iterator.hpp>

namespace boost  { 
namespace interprocess  { 
namespace detail {

template <class VoidPointer>
struct rb_tree_node_base
{
   typedef typename boost::interprocess::detail::pointer_to_other
      <VoidPointer, rb_tree_node_base<VoidPointer> >::type   basic_node_pointer;

   enum color_t { red_color    = false, black_color  = true };

   const basic_node_pointer &parent() const  {  return mp_parent; }
         basic_node_pointer &parent()        {  return mp_parent; }
   const basic_node_pointer &left()   const  {  return mp_left;   }
         basic_node_pointer &left()          {  return mp_left;   }
   const basic_node_pointer &right()  const  {  return mp_right;  }
         basic_node_pointer &right()         {  return mp_right;  }
   const color_t      &color()  const        {  return m_color;   }
         color_t      &color()               {  return m_color;   }
   private:
   basic_node_pointer   mp_parent;
   basic_node_pointer   mp_left;
   basic_node_pointer   mp_right;
   color_t              m_color; 
};

/*!Basic red-black tree functions that don't need 
   comparison operator*/
template <class NodeBase>
class rb_tree_algo
{
   typedef  typename NodeBase::basic_node_pointer basic_node_pointer;
   public:
   static basic_node_pointer next_node(basic_node_pointer node)
   {
      if (node->right()) {
         node = node->right();
         while (node->left()){
            node = node->left();
         }
      }
      else{
         basic_node_pointer y = node->parent();
         while (node == y->right()) {
            node = y;
            y = y->parent();
         }
         if (node->right() != y)
         node = y;
      }
      return node;
   }

   static basic_node_pointer previous_node(basic_node_pointer node)
   {
      if (node->color() == NodeBase::red_color &&
         node->parent()->parent() == node){
         node = node->right();
      }
      else if (node->left()) {
         basic_node_pointer y = node->left();
         while (y->right()){
            y = y->right();
         }
         node = y;
      }
      else{
         basic_node_pointer y = node->parent();
         while (node == y->left()) {
            node = y;
            y = y->parent();
         }
         node = y;
      }
      return node;
   }

   static bool is_header (const basic_node_pointer p)
   {
      return p->color() == NodeBase::red_color && p->parent()->parent() == p;
   }

   static basic_node_pointer minimum_node(basic_node_pointer x)
   {
      while (x->left()){
         x = x->left();
      }
      return x;
   }

   static basic_node_pointer maximum_node(basic_node_pointer x)
   {
      while (x->right()){
         x = x->right();
      }
      return x;
   }

   static void replace_own (const basic_node_pointer &header, 
                            const basic_node_pointer &own, 
                            const basic_node_pointer &p)
   {
      if (header->parent() == own)
         header->parent() = p;
      else if (own->parent()->left() == own)
         own->parent()->left() = p;
      else
         own->parent()->right() = p;
   }

   static void rotate_left(const basic_node_pointer &header, basic_node_pointer x)
   {
      basic_node_pointer y = x->right();
      x->right() = y->left();
      if (y->left() !=0)
         y->left()->parent() = x;
      y->parent() = x->parent();
      replace_own(header, x, y);
      y->left() = x;
      x->parent() = y;
   }

   static void rotate_right(const basic_node_pointer &header, basic_node_pointer x)
   {
      basic_node_pointer y = x->left();
      x->left() = y->right();
      if (y->right())
         y->right()->parent() = x;
      y->parent() = x->parent();
      replace_own(header, x, y);
      y->right()  = x;
      x->parent() = y;
   }

   static void rebalance(const basic_node_pointer &header, basic_node_pointer x)
   {
      x->color() = NodeBase::red_color;
      while (x != header->parent() && x->parent()->color() == NodeBase::red_color) {
         if (x->parent() == x->parent()->parent()->left()) {
            basic_node_pointer y = x->parent()->parent()->right();
            if (y && y->color() == NodeBase::red_color) {
               x->parent()->color() = NodeBase::black_color;
               y->color() = NodeBase::black_color;
               x->parent()->parent()->color() = NodeBase::red_color;
               x = x->parent()->parent();
            }
            else{
               if (x == x->parent()->right()) {
                  x = x->parent();
                  rotate_left(header, x);
               }
               x->parent()->color() = NodeBase::black_color;
               x->parent()->parent()->color() = NodeBase::red_color;
               rotate_right(header, x->parent()->parent());
            }
         }
         else{
            basic_node_pointer y = x->parent()->parent()->left();
            if (y && y->color() == NodeBase::red_color) {
               x->parent()->color() = NodeBase::black_color;
               y->color() = NodeBase::black_color;
               x->parent()->parent()->color() = NodeBase::red_color;
               x = x->parent()->parent();
            }
            else{
               if (x == x->parent()->left()) {
                  x = x->parent();
                  rotate_right(header, x);
               }
               x->parent()->color() = NodeBase::black_color;
               x->parent()->parent()->color() = NodeBase::red_color;
               rotate_left(header, x->parent()->parent());
            }
         }
      }
      header->parent()->color() = NodeBase::black_color;
   }

   static basic_node_pointer erase_node 
            (const basic_node_pointer &header, basic_node_pointer z)
   {
      basic_node_pointer& root      = header->parent();
      basic_node_pointer& leftmost  = header->left();
      basic_node_pointer& rightmost = header->right();
      basic_node_pointer y = z;
      basic_node_pointer x = 0;
      basic_node_pointer x_parent = 0;
      if (!y->left())     // z has at most one non-null child. y == z.
         x = y->right();     // x might be null.
      else
         if (!y->right())  // z has exactly one non-null child. y == z.
            x = y->left();    // x is not null.
         else{                   // z has two non-null children.  Set y to
            y = y->right();   //   z's successor.  x might be null.
            while (y->left())
               y = y->left();
            x = y->right();
         }
      if (y != z) {          // relink y in place of z.  y is z's successor
         z->left()->parent() = y; 
         y->left() = z->left();
         if (y != z->right()) {
            x_parent = y->parent();
            if (x) 
               x->parent() = y->parent();
            y->parent()->left() = x;      // y must be a child of mp_left
            y->right() = z->right();
            z->right()->parent() = y;
         }
         else
            x_parent = y;

         replace_own(header, z, y);
         y->parent() = z->parent();
         std::swap(y->color(), z->color());
         y = z;
         // y now points to node to be actually deleted
      }
      else{                        // y == z
         x_parent = y->parent();
         if (x) 
            x->parent() = y->parent(); 

         replace_own(header, z, x);
  
         if (leftmost == z) 
            if (!z->right())        // z->left must be null also
               leftmost = z->parent();
            // makes leftmost == this->m_header if z == root
            else
               leftmost = minimum_node(x);
         if (rightmost == z)  
            if (!z->left())         // z->right must be null also
               rightmost = z->parent();  
            // makes rightmost == this->m_header if z == root
            else                      // x == z->left
               rightmost = maximum_node(x);
      }
      if (y->color() != NodeBase::red_color) { 
         while (x != root && (!x || x->color() == NodeBase::black_color))
            if (x == x_parent->left()) {
               basic_node_pointer w = x_parent->right();
               if (w->color() == NodeBase::red_color) {
                  w->color() = NodeBase::black_color;
                  x_parent->color() = NodeBase::red_color;
                  rotate_left(header, x_parent);
                  w = x_parent->right();
               }
               if ((!w->left() || 
                     w->left()->color() == NodeBase::black_color) &&
                     (!w->right() || 
                     w->right()->color() == NodeBase::black_color)) {
                  w->color() = NodeBase::red_color;
                  x = x_parent;
                  x_parent = x_parent->parent();
               }
               else{
                  if (!w->right() || 
                     w->right()->color() == NodeBase::black_color) {
                     if (w->left()) 
                        w->left()->color() = NodeBase::black_color;
                     w->color() = NodeBase::red_color;
                     rotate_right(header, w);
                     w = x_parent->right();
                  }
                  w->color() = x_parent->color();
                  x_parent->color() = NodeBase::black_color;
                  if (w->right()) 
                     w->right()->color() = NodeBase::black_color;
                  rotate_left(header, x_parent);
                  break;
               }
            } 
            else{
               // same as above, with mp_right <-> mp_left.
               basic_node_pointer w = x_parent->left();
               if (w->color() == NodeBase::red_color) {
                  w->color() = NodeBase::black_color;
                  x_parent->color() = NodeBase::red_color;
                  rotate_right(header, x_parent);
                  w = x_parent->left();
               }
               if ((!w->right() || 
                     w->right()->color() == NodeBase::black_color) &&
                     (!w->left() || 
                     w->left()->color() == NodeBase::black_color)) {
                  w->color() = NodeBase::red_color;
                  x = x_parent;
                  x_parent = x_parent->parent();
               }
               else{
                  if (!w->left() || 
                     w->left()->color() == NodeBase::black_color) {
                     if (w->right()) 
                        w->right()->color() = NodeBase::black_color;
                     w->color() = NodeBase::red_color;
                     rotate_left(header, w);
                     w = x_parent->left();
                  }
                  w->color() = x_parent->color();
                  x_parent->color() = NodeBase::black_color;
                  if (w->left()) 
                     w->left()->color() = NodeBase::black_color;
                  rotate_right(header, x_parent);
                  break;
               }
            }
         if (x) 
            x->color() = NodeBase::black_color;
      }
      return y;
   }

   static basic_node_pointer link_and_rebalance
      (basic_node_pointer header, bool link_left, basic_node_pointer y, basic_node_pointer z)
   {

      z->parent() = y;
      z->left()   = 0;
      z->right()  = 0;

      //when y == _header
      if (y == header) {
         header->left()    = z;
         header->parent()  = z;
         header->right()   = z;
      }
      else if (link_left) {
         y->left()      = z;  
         if (y == header->left()){
            header->left() = z;  // maintain header->right() pointing to max node
         }
      }
      else{
         y->right() = z;
         if (y == header->right()){
            header->right() = z;  // maintain header->right() pointing to max node
         }
      }
      rebalance(header, z);
      return z;
/*
      if (link_left) {
         y->left() = z;               // also makes header->left() = z 
                                      //    when y == _header
         if (y == header) {
            header->parent() = z;
            header->right() = z;
         }
         else if (y == header->left())
            header->left() = z;   // maintain header->left() pointing to min node
      }
      else{
         y->right() = z;
         if (y == header->right())
            header->right() = z;  // maintain header->right() pointing to max node
      }
      z->parent() = y;
      z->left()   = 0;
      z->right()  = 0;
      rebalance(header, z);
      return z;*/
   }

   static void clear(basic_node_pointer header)
   {
      header->left()    = header;
      header->right()   = header;
      header->parent()  = 0;
   }

   static bool empty(const basic_node_pointer &header)  { return !header->parent(); }
};

/*!Basic red-black tree functions that only need 
   the comparison functor and functors to obtain the 
   key from a value*/

template <class Node, class Key, class KeyOfValue, 
          class KeyCompare>
class rb_tree_func : public rb_tree_algo<Node>
{
   typedef rb_tree_algo<rb_tree_node_base<typename pointer_to_other
      <typename Node::basic_node_pointer, void>::type> >     rb_tree_algo_t;

//   typedef rb_tree_algo<Node>                      rb_tree_algo_t;
   typedef typename Node::node_pointer             node_pointer;
   typedef typename Node::basic_node_pointer       basic_node_pointer;

   static bool node_compare (const KeyCompare   &key_compare,
                             const node_pointer &node, 
                             const node_pointer &other_node)
   {
      return key_compare(KeyOfValue()(node->value()), 
                         KeyOfValue()(other_node->value()));
   }

   public:

   struct insert_unique_context
   {
      bool link_left;
      basic_node_pointer node;
   };

   static node_pointer find(const node_pointer &header, 
                            const KeyCompare &key_compare, 
                            const Key& k)
   {
      basic_node_pointer y = header;               // Last node which is not less than k. 
      basic_node_pointer x = header->parent();     // Current node. 
      while (x) {
         if (!(key_compare(KeyOfValue()(Node::downcast(x)->value()), k))){
            y = x, x = x->left();
         }
         else{
            x = x->right();
         }
      }
      
      if(y == header || key_compare(k, KeyOfValue()(Node::downcast(y)->value()))){
         y = header;
      }
      return Node::downcast(y);
   }

   static node_pointer upper_bound(const node_pointer &header, 
                                   const KeyCompare &key_compare, 
                                   const Key& k)
   {
      basic_node_pointer y = header;            //Last node which is greater than k.
      basic_node_pointer x = header->parent();  //Current node.

      while (x){
         if (key_compare(k, KeyOfValue()(Node::downcast(x)->value()))){
            y = x, x = x->left();
         }
         else{
            x = x->right();
         }
      }
      return Node::downcast(y);
   }

   static node_pointer lower_bound(const node_pointer &header, 
                                   const KeyCompare  &key_compare,
                                   const Key& k)
   {
      basic_node_pointer y = header;   // Last node which is not less than k.
      basic_node_pointer x = header->parent();        // Current node.

      while (x){
         if (!key_compare(KeyOfValue()(Node::downcast(x)->value()), k)){
            y = x, x = x->left();
         }
         else{
            x = x->right();
         }
      }
      return Node::downcast(y);
   }

   static bool insert_unique (const node_pointer &header, 
                              const KeyCompare  &key_compare,
                              node_pointer &new_node, 
                              node_pointer &out)
   {
      insert_unique_context  context;
      if(!insert_unique_prepare(header, key_compare, KeyOfValue()(new_node->value()), out, context)){
         return false;
      }
      insert_unique_commit(header, key_compare, new_node, out, context);
      return true;
   }

   static bool insert_unique_hint(const node_pointer &header, 
                                  const KeyCompare  &key_compare,
                                  const node_pointer &hint, 
                                  node_pointer &new_node,
                                  node_pointer &out)
   {
      insert_unique_context context;
      if(!insert_unique_hint_prepare
            (header, hint, key_compare, KeyOfValue()(new_node->value()), out, context)){
         return false;
      }
      insert_unique_commit(header, key_compare, new_node, out, context);
      return true;
   }

   static bool insert_unique_prepare (const node_pointer & header, 
                                      const KeyCompare  &key_compare,
                                      const Key &key,
                                      insert_unique_context &context)
   {
      basic_node_pointer x(header->parent());
      basic_node_pointer y(header);
      basic_node_pointer prev(0);

      //Find the upper bound, and cache the previous value and if we should
      //store it in the left or right node
      bool left_child = true;
      while (x) {
         y = x;
         x = (left_child = key_compare(key, KeyOfValue()(Node::downcast(x)->value())))
               ? (x->left()) 
               : (prev = y, x->right());
      }

      //Since we've found the upper bound there is no other value with the same key if:
      //    - There is no previous node
      //    - The previous node is less than the key
	   if(!prev || key_compare(KeyOfValue()(Node::downcast(prev)->value()), key)){
         context.link_left = left_child;
         context.node      = y;
         return true;
	   }
      //If the previous value was not less than key, it means that it's equal
      //(because we've checked the upper bound)
      else{
         context.node = prev;
         return false;
      }
   }

   static bool insert_unique_hint_prepare(const node_pointer &header,
                                          const KeyCompare  &key_compare,
                                          const node_pointer &hint, 
                                          const Key &key,
                                          insert_unique_context &context)
   {
      //hint must be bigger than the key
	   if (hint == header || key_compare(key, KeyOfValue()(hint->value()))){
		   node_pointer prev = hint;
         //The previous value should be less than the key
		   if (prev == header->left() || 
            key_compare(KeyOfValue()((prev = Node::downcast(previous_node(hint)))->value()), key)){
            context.link_left = empty(header) || !hint->left();
            context.node      = context.link_left ? hint : prev;
            return true;
		   }
         //The value is already present
         else{
            context.link_left = false;
            context.node      = prev;
            return false;
         }
	   }
      //The hint was wrong, use hintless insert
      else{
         return insert_unique_prepare(header, key_compare, key, context);
      }
   }

   static void insert_unique_commit (const node_pointer &header,
                                     const KeyCompare  &key_compare, 
                                     node_pointer &new_node, 
                                     insert_unique_context &context)
   {
      rb_tree_algo_t::link_and_rebalance
         (header, context.link_left, context.node, new_node);
   }

   static void insert_equal (const node_pointer & header, 
                             const KeyCompare  &key, 
                             node_pointer &new_node)
   {
      insert_equal_upper(header, key, new_node);
   }

   static void insert_equal_lower  (const node_pointer & header, 
                                    const KeyCompare  &key, 
                                    node_pointer &new_node)
   {
      basic_node_pointer y = header;
      basic_node_pointer x = header->parent();

      while (x) {
         y = x;
         x = !node_compare(key, Node::downcast(x), new_node) 
               ? x->left() : x->right();
      }

      bool link_left = (y == header) || !node_compare(key, Node::downcast(y), new_node);
      rb_tree_algo_t::link_and_rebalance(header, link_left, y, new_node);
   }

   static void insert_equal_upper  (const node_pointer & header, 
                                    const KeyCompare  &key, 
                                    node_pointer &new_node)
   {
      basic_node_pointer y = header;
      basic_node_pointer x = header->parent();

      while (x) {
         y = x;
         x = node_compare(key, new_node, Node::downcast(x)) 
               ? x->left() : x->right();
      }

      bool link_left = (y == header) || node_compare(key, new_node, Node::downcast(y));
      rb_tree_algo_t::link_and_rebalance(header, link_left, y, new_node);
   }

   static void insert_equal_hint (const node_pointer & header,
                                  const KeyCompare  &key_compare, 
                                  const node_pointer &hint, 
                                  node_pointer &new_node)
   {
      if(hint == header || !node_compare(key_compare, hint, new_node)){
         node_pointer prev;
         if(hint == header->left() || 
            !node_compare(key_compare, new_node, 
               (prev = Node::downcast(previous_node(hint))))){
            bool link_left = empty(header) || !hint->left();
            rb_tree_algo_t::link_and_rebalance
               (header, link_left, link_left ? hint : prev, new_node);
         }
         else{
            insert_equal_upper(header, key_compare, new_node);
         }
      }
      else{
         insert_equal_lower(header, key_compare, new_node);
      }
   }

   static std::size_t erase_with_key(const node_pointer &header, 
                                     const KeyCompare  &key_compare, 
                                     const Key& k)
   {
      node_pointer lower, upper, tmp;
      rb_tree_algo_t::equal_range(header, k, lower, upper);
      std::size_t count = 0;
      
      while(lower != upper){
         tmp = lower;
         lower = rb_tree_algo_t::next_node(lower);
         rb_tree_algo_t::erase(header, tmp);
         ++count;
      }
      return count;
   }

   static void equal_range(const node_pointer &header,
                           const KeyCompare  &key_compare,
                           const Key& k,
                           node_pointer &lower,
                           node_pointer &upper)
   {
//      lower = lower_bound(header, key_compare, k);
//      upper = upper_bound(header, key_compare, k);
      basic_node_pointer y = header;   // Last node which is not less than k.
      basic_node_pointer x = header->parent();        // Current node.

      while (x){
         //Execute common logic
         if (key_compare(KeyOfValue()(Node::downcast(x)->value()), k)){
            x = x->right();
         }
         else if (key_compare(k, KeyOfValue()(Node::downcast(x)->value()))){
            y = x, x = x->left();
         }
         //Now lower and upper bounds needs to follow different subtrees
         else{
            basic_node_pointer xu(x), yu(y);
            y = x, x = x->left();
            xu = xu->right();

            while (x){
               if (key_compare(KeyOfValue()(Node::downcast(x)->value()), k)){
                  x = x->right();
               }
               else{
                  y = x, x = x->left();
               }
            }
            while (xu){
               if (key_compare(k, KeyOfValue()(Node::downcast(xu)->value()))){
                  yu = xu, xu = xu->left();
               }
               else{
                  xu = xu->right();
               }
            }
            lower = Node::downcast(y);
            upper = Node::downcast(yu);
            return;
         }
      }
      lower = upper = Node::downcast(y);
   }

   static std::size_t count(const node_pointer &header, 
                            const KeyCompare   &key_compare,
                            const Key& k)
   {
      node_pointer lower, upper;
      equal_range(header, key_compare, k, lower, upper);
      std::size_t count = 0;
      while(lower != upper){
         lower = Node::downcast(rb_tree_algo_t::next_node(lower));
         ++count;
      }
      return count;
   }

};

template <class Node>
class rb_tree_const_iterator
   : public boost::iterator<std::bidirectional_iterator_tag, 
                            typename Node::value_type,        
                            typename Node::difference_type, 
                            typename Node::const_pointer,      
                            typename Node::const_reference>
{
   private:
   typedef boost::iterator<std::bidirectional_iterator_tag, 
                           typename Node::value_type,        
                           typename Node::difference_type, 
                           typename Node::const_pointer,      
                           typename Node::const_reference> base_t;

   protected:
   typedef typename Node::node_pointer                node_pointer;
   typedef typename Node::basic_node_pointer          basic_node_pointer;
   typedef rb_tree_algo<Node>                         rb_tree_algo_t;
   node_pointer mp_node;

   public: 
   typedef typename base_t::pointer                   pointer;
   typedef typename base_t::reference                 reference;
   typedef typename base_t::difference_type           difference_type;
   typedef typename base_t::iterator_category         iterator_category;
   typedef typename base_t::value_type                value_type;

   rb_tree_const_iterator() 
      {}
/*
   explicit rb_tree_const_iterator(const node_pointer &x) 
      : mp_node(x)
      {}

*/
   explicit rb_tree_const_iterator(const basic_node_pointer &x) 
      : mp_node(Node::downcast(x))
      {}

   rb_tree_const_iterator(const rb_tree_const_iterator & it)
      : mp_node(it.get_ptr())
      {}

   reference operator*() const 
      {  return mp_node->value(); }

   pointer operator->() const 
      {  return pointer(&mp_node->value()); }

   rb_tree_const_iterator& operator++() 
   {  
      mp_node = Node::downcast(rb_tree_algo_t::next_node(mp_node)); 
      return *this; 
   }

   rb_tree_const_iterator operator++(int) 
   {  
      rb_tree_const_iterator tmp = *this;  
      mp_node = rb_tree_algo_t::next_node(mp_node);  
      return tmp; 
   }
         
   rb_tree_const_iterator& operator--() 
   { 
      mp_node = rb_tree_algo_t::previous_node(mp_node); 
      return *this; 
   }

   rb_tree_const_iterator operator--(int) 
   {  
      rb_tree_const_iterator tmp = *this;  
      mp_node = rb_tree_algo_t::previous_node(mp_node);   
      return tmp; 
   }

   friend bool operator==(const rb_tree_const_iterator& x, const rb_tree_const_iterator& y) 
      {  return x.get_ptr() == y.get_ptr();   }

   friend bool operator!=(const rb_tree_const_iterator& x, const rb_tree_const_iterator& y) 
      {  return x.get_ptr() != y.get_ptr();   }

   node_pointer &get_ptr()
      {  return mp_node;   }

   const node_pointer &get_ptr() const
      {  return mp_node;   }
};

//list rb_tree_iterator
template <class Node>
class rb_tree_iterator : public rb_tree_const_iterator<Node>
{
	private:
   typedef rb_tree_const_iterator<Node>                  base_t;
   typedef typename base_t::node_pointer                 node_pointer;
   typedef typename base_t::basic_node_pointer           basic_node_pointer;
	public:
   typedef typename Node::pointer                        pointer;
   typedef typename Node::reference                      reference;

   //Constructors
	rb_tree_iterator()
   {}
/*
	explicit rb_tree_iterator(const node_pointer &ptr) 
      : base_t(ptr)
   {}
*/
	explicit rb_tree_iterator(const basic_node_pointer &ptr) 
      : base_t(ptr)
   {}

   //Pointer like operators
	reference operator*()  const {  return  this->mp_node->value();  }
	pointer   operator->() const {  return  pointer(&this->mp_node->value());  }

   //Increment / Decrement
	rb_tree_iterator& operator++()  
      {  base_t::operator++(); return *this;  }

	rb_tree_iterator operator++(int)
      { node_pointer tmp = this->mp_node; ++*this; return rb_tree_iterator(tmp); }
	
   rb_tree_iterator& operator--()
   	{  base_t::operator--(); return *this;  }

	rb_tree_iterator operator--(int)
	   {  rb_tree_iterator tmp = *this; --*this; return tmp; }
};

} //namespace detail {
} //namespace interprocess  {
} //namespace boost  {

#include <boost/interprocess/detail/config_end.hpp>

#endif //BOOST_INTERPROCESS_TREE_FUNC_HPP

