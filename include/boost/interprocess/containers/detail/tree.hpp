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
// This file comes from SGI's stl_tree file. Modified by Ion Gaztanaga 2005.
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
#ifndef BOOST_INTERPROCESS_TREE_HPP
#define BOOST_INTERPROCESS_TREE_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/detail/move.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/algorithms.hpp>
#include <boost/type_traits/has_trivial_destructor.hpp>
#include <boost/detail/no_exceptions_support.hpp>
#include <boost/interprocess/containers/detail/node_alloc_holder.hpp>
#include <boost/intrusive/rbtree.hpp>

#include <iterator>
#include <algorithm>

namespace boost {
namespace interprocess {
namespace detail {

template<class Key, class Value, class KeyCompare, class KeyOfValue>
struct value_compare_impl
   :  public KeyCompare
{
   typedef Value        value_type;
   typedef KeyCompare   key_compare; 
   typedef KeyOfValue   key_of_value;
   typedef Key          key_type;

   value_compare_impl(key_compare kcomp)
      :  key_compare(kcomp)
   {}

   const key_compare &key_comp() const
   {  return static_cast<const key_compare &>(*this);  }

   key_compare &key_comp()
   {  return static_cast<key_compare &>(*this);  }

   template<class A, class B>
   bool operator()(const A &a, const B &b) const
   {  return key_compare::operator()(KeyOfValue()(a), KeyOfValue()(b)); }
};

template <class T, class VoidPointer>
struct rbtree_node
   :  public boost::intrusive::set_base_hook
         < boost::intrusive::tag
         , boost::intrusive::safe_link
         , VoidPointer>
{
   typedef boost::intrusive::set_base_hook
         < boost::intrusive::tag
         , boost::intrusive::safe_link
         , VoidPointer>   hook_type;

   typedef T value_type;

   typedef rbtree_node<T, VoidPointer> node_type;

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   template<class Convertible>
   rbtree_node(const Convertible &conv)
      : m_data(conv){}
   #else
   template<class Convertible>
   rbtree_node(Convertible &&conv)
      : m_data(forward<Convertible>(conv)){}
   #endif

   rbtree_node &operator=(const rbtree_node &other)
   {  do_assign(other.m_data);   return *this;  }

   T m_data;
   private:   

   template<class A, class B>
   void do_assign(const std::pair<const A, B> &p)
   {
      const_cast<A&>(m_data.first) = p.first;
      m_data.second  = p.second;
   }

   template<class V>
   void do_assign(const V &v)
   {  m_data = v; }

   public:
   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   template<class Convertible>
   static void construct(node_type *ptr, const Convertible &value)
   {  new(ptr) node_type(value);  }
   #else
   template<class Convertible>
   static void construct(node_type *ptr, Convertible &&value)
   {  new(ptr) node_type(forward<Convertible>(value));  }
   #endif

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   template<class Convertible1, class Convertible2>
   static void construct(node_type *ptr, 
                         const detail::moved_object<std::pair<Convertible1, Convertible2> > &value)
   {  
      //std::pair is not movable so we define our own type and overwrite it
      typedef detail::pair<typename node_type::value_type::first_type
                          ,typename node_type::value_type::second_type> hack_pair_t;

      typedef rbtree_node<hack_pair_t, VoidPointer>  hack_node_t;

      new((void*)ptr) hack_node_t(value);  
   }
   #else
   template<class Convertible1, class Convertible2>
   static void construct(node_type *ptr, 
                         std::pair<Convertible1, Convertible2> &&value)
   {  
      //std::pair is not movable so we define our own type and overwrite it
      typedef detail::pair<typename node_type::value_type::first_type
                          ,typename node_type::value_type::second_type> hack_pair_t;

      typedef rbtree_node<hack_pair_t, VoidPointer>  hack_node_t;

      new((void*)ptr) hack_node_t(value);  
   }
   #endif
};

}//namespace detail {

template<class T, class VoidPointer>
struct has_own_construct_from_it
   < boost::interprocess::detail::rbtree_node<T, VoidPointer> >
{
   static const bool value = true;
};

namespace detail {

template<class A, class ValueCompare>
struct intrusive_rbtree_type
{
   typedef typename A::value_type                  value_type;
   typedef typename detail::pointer_to_other
      <typename A::pointer, void>::type            void_pointer;
   typedef typename detail::rbtree_node
         <value_type, void_pointer>                node_type;
   typedef node_compare<ValueCompare, node_type>   node_compare_type;

   typedef typename boost::intrusive::rbtree
      <typename node_type::hook_type::
            template value_traits<node_type>
      ,node_compare_type
      ,boost::intrusive::safe_link
      ,typename A::size_type>                      container_type;

   typedef container_type                          type ;
};

}  //namespace detail {

namespace detail {

template <class Key, class Value, class KeyOfValue, 
          class KeyCompare, class A>
class rbtree
   : protected detail::node_alloc_holder
      <A, typename detail::intrusive_rbtree_type
         <A, value_compare_impl<Key, Value, KeyCompare, KeyOfValue>  
         >::type
      >
{
   typedef typename detail::intrusive_rbtree_type
         <A, value_compare_impl
            <Key, Value, KeyCompare, KeyOfValue>
         >::type                                      Icont;
   typedef detail::node_alloc_holder<A, Icont>        AllocHolder;
   typedef typename AllocHolder::NodePtr              NodePtr;
   typedef rbtree < Key, Value, KeyOfValue
                  , KeyCompare, A>                    ThisType;
   typedef typename AllocHolder::NodeAlloc            NodeAlloc;
   typedef typename AllocHolder::ValAlloc             ValAlloc;
   typedef typename AllocHolder::Node                 Node;
   typedef typename Icont::iterator                   iiterator;
   typedef typename Icont::const_iterator             iconst_iterator;
   typedef detail::allocator_destroyer<NodeAlloc>     Destroyer;
   typedef typename AllocHolder::allocator_v1         allocator_v1;
   typedef typename AllocHolder::allocator_v2         allocator_v2;
   typedef typename AllocHolder::alloc_version        alloc_version;

   class RecyclingCloner;
   friend class RecyclingCloner;
   
   class RecyclingCloner
   {
      public:
      RecyclingCloner(AllocHolder &holder, Icont &irbtree)
         :  m_holder(holder), m_icont(irbtree)
      {}

      NodePtr operator()(const Node &other) const
      {
         if(!m_icont.empty()){
            //First recycle a node (this can't throw)
            NodePtr p = m_icont.unlink_leftmost_without_rebalance();
            try{
               //This can throw
               *p = other;
               return p;
            }
            catch(...){
               //If there is an exception destroy the whole source
               m_holder.destroy_node(p);
               while((p = m_icont.unlink_leftmost_without_rebalance())){
                  m_holder.destroy_node(p);
               }
               throw;
            }
         }
         else{
            return m_holder.create_node(other);
         }
      }

      AllocHolder &m_holder;
      Icont &m_icont;
   };

   public:
   typedef Key                                        key_type;
   typedef Value                                      value_type;
   typedef A                                          allocator_type;
   typedef KeyCompare                                 key_compare;
   typedef value_compare_impl< Key, Value
                        , KeyCompare, KeyOfValue>     value_compare;
   typedef typename A::pointer                        pointer;
   typedef typename A::const_pointer                  const_pointer;
   typedef typename A::reference                      reference;
   typedef typename A::const_reference                const_reference;
   typedef typename A::size_type                      size_type;
   typedef typename A::difference_type                difference_type;
   typedef difference_type                            rbtree_difference_type;
   typedef pointer                                    rbtree_pointer;
   typedef const_pointer                              rbtree_const_pointer;
   typedef reference                                  rbtree_reference;
   typedef const_reference                            rbtree_const_reference;
   typedef NodeAlloc                                  stored_allocator_type;

   private:

   template<class KeyValueCompare>
   struct key_node_compare
      :  private KeyValueCompare
   {
      key_node_compare(KeyValueCompare comp)
         :  KeyValueCompare(comp)
      {}
      
      template<class KeyType>
      bool operator()(const Node &n, const KeyType &k) const
      {  return KeyValueCompare::operator()(n.m_data, k);  }

      template<class KeyType>
      bool operator()(const KeyType &k, const Node &n) const
      {  return KeyValueCompare::operator()(k, n.m_data);  }
   };

   typedef key_node_compare<value_compare>  KeyNodeCompare;

   public:
   //rbtree const_iterator
   class const_iterator
      : public std::iterator
         < std::bidirectional_iterator_tag
         , value_type            , rbtree_difference_type
         , rbtree_const_pointer  , rbtree_const_reference>
   {
      protected:
      typedef typename Icont::iterator  iiterator;
      iiterator m_it;
      explicit const_iterator(iiterator it)  : m_it(it){}
      void prot_incr() { ++m_it; }
      void prot_decr() { --m_it; }

      private:
      iiterator get()
      {  return this->m_it;   }

      public:
      friend class rbtree <Key, Value, KeyOfValue, KeyCompare, A>;
      typedef rbtree_difference_type        difference_type;

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
      { iiterator tmp = m_it; ++*this; return const_iterator(tmp);  }

      const_iterator& operator--()
      {   prot_decr(); return *this;   }

      const_iterator operator--(int)
      {  iiterator tmp = m_it; --*this; return const_iterator(tmp); }

      //Comparison operators
      bool operator==   (const const_iterator& r)  const
      {  return m_it == r.m_it;  }

      bool operator!=   (const const_iterator& r)  const
      {  return m_it != r.m_it;  }
   };

   //rbtree iterator
   class iterator : public const_iterator
   {
      private:
      explicit iterator(iiterator it)
         :  const_iterator(it)
      {}
   
      iiterator get()
      {  return this->m_it;   }

      public:
      friend class rbtree <Key, Value, KeyOfValue, KeyCompare, A>;
      typedef rbtree_pointer       pointer;
      typedef rbtree_reference     reference;

      //Constructors
      iterator(){}

      //Pointer like operators
      reference operator*()  const {  return  this->m_it->m_data;  }
      pointer   operator->() const {  return  pointer(&this->m_it->m_data);  }

      //Increment / Decrement
      iterator& operator++()  
         { this->prot_incr(); return *this;  }

      iterator operator++(int)
         { iiterator tmp = this->m_it; ++*this; return iterator(tmp); }
      
      iterator& operator--()
         {  this->prot_decr(); return *this;  }

      iterator operator--(int)
         {  iterator tmp = *this; --*this; return tmp; }
   };

   typedef std::reverse_iterator<iterator>        reverse_iterator;
   typedef std::reverse_iterator<const_iterator>  const_reverse_iterator;

   rbtree(const key_compare& comp = key_compare(), 
           const allocator_type& a = allocator_type())
      : AllocHolder(a, comp)
   {}

   template <class InputIterator>
   rbtree(InputIterator first, InputIterator last, const key_compare& comp,
          const allocator_type& a, bool unique_insertion)
      : AllocHolder(a, comp)
   {
      typedef typename std::iterator_traits<InputIterator>::iterator_category ItCat;
      priv_create_and_insert_nodes(first, last, unique_insertion, alloc_version(), ItCat());
   }

   rbtree(const rbtree& x) 
      :  AllocHolder(x, x.key_comp())
   {
      this->m_icont.clone_from
         (x.m_icont, typename AllocHolder::cloner(*this), Destroyer(*this));
   }

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   rbtree(const detail::moved_object<rbtree>& x) 
      :  AllocHolder(x.get(), x.get().key_comp())
   {  this->swap(x.get());  }
   #else
   rbtree(rbtree &&x) 
      :  AllocHolder(x, x.key_comp())
   {  this->swap(x);  }
   #endif

   ~rbtree()
   {  this->clear(); }

   rbtree& operator=(const rbtree& x)
   {
      if (this != &x) {
         //Transfer all the nodes to a temporary tree
         //If anything goes wrong, all the nodes will be destroyed
         //automatically
         Icont other_tree(this->m_icont.value_comp());
         other_tree.swap(this->m_icont);

         //Now recreate the source tree reusing nodes stored by other_tree
         this->m_icont.clone_from
            (x.m_icont, RecyclingCloner(*this, other_tree), Destroyer(*this));

         //If there are remaining nodes, destroy them
         NodePtr p;
         while((p = other_tree.unlink_leftmost_without_rebalance())){
            AllocHolder::destroy_node(p);
         }
      }
      return *this;
   }

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   rbtree& operator=(const detail::moved_object<rbtree>& mx)
   {  this->clear(); this->swap(mx.get());   return *this;  }
   #else
   rbtree& operator=(rbtree &&mx)
   {  this->clear(); this->swap(mx);   return *this;  }
   #endif

   public:    
   // accessors:
   value_compare value_comp() const 
   {  return this->m_icont.value_comp().value_comp(); }

   key_compare key_comp() const 
   {  return this->m_icont.value_comp().value_comp().key_comp(); }

   allocator_type get_allocator() const 
   {  return allocator_type(*this); }

   const stored_allocator_type &get_stored_allocator() const 
   {  return static_cast<const stored_allocator_type &>(*this); }

   stored_allocator_type &get_stored_allocator()
   {  return static_cast<stored_allocator_type&>(*this); }

   iterator begin()
   { return iterator(this->m_icont.begin()); }

   const_iterator begin() const
   {  return const_iterator(this->non_const_icont().begin());   }

   iterator end()
   {  return iterator(this->m_icont.end());  }

   const_iterator end() const
   {  return const_iterator(this->non_const_icont().end());  }

   reverse_iterator rbegin()
   {  return reverse_iterator(end());  }

   const_reverse_iterator rbegin() const
   {  return const_reverse_iterator(end());  }

   reverse_iterator rend()
   {  return reverse_iterator(begin());   }

   const_reverse_iterator rend() const
   {  return const_reverse_iterator(begin());   }

   bool empty() const 
   {  return !this->size();  }

   size_type size() const 
   {  return this->m_icont.size();   }

   size_type max_size() const 
   {  return AllocHolder::max_size();  }

   void swap(ThisType& x)
   {  AllocHolder::swap(x);   }

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   void swap(const detail::moved_object<rbtree>& mt) 
   {  this->swap(mt.get());   }
   #else
   void swap(rbtree &&mt) 
   {  this->swap(mt);   }
   #endif

   public:

   typedef typename Icont::insert_commit_data insert_commit_data;

   // insert/erase
   std::pair<iterator,bool> insert_unique_check
      (const key_type& key, insert_commit_data &data)
   {
      std::pair<iiterator, bool> ret = 
         this->m_icont.insert_unique_check(key, KeyNodeCompare(value_comp()), data);
      return std::pair<iterator, bool>(iterator(ret.first), ret.second);
   }

   std::pair<iterator,bool> insert_unique_check
      (const_iterator hint, const key_type& key, insert_commit_data &data)
   {
      std::pair<iiterator, bool> ret = 
         this->m_icont.insert_unique_check(hint.get(), key, KeyNodeCompare(value_comp()), data);
      return std::pair<iterator, bool>(iterator(ret.first), ret.second);
   }

   iterator insert_unique_commit(const value_type& v, insert_commit_data &data)
   {
      NodePtr tmp = AllocHolder::create_node(v);
      iiterator it(this->m_icont.insert_unique_commit(*tmp, data));
      return iterator(it);
   }

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   template<class MovableConvertible>
   iterator insert_unique_commit
      (const detail::moved_object<MovableConvertible>& mv, insert_commit_data &data)
   {
      NodePtr tmp = AllocHolder::create_node(mv);
      iiterator it(this->m_icont.insert_unique_commit(*tmp, data));
      return iterator(it);
   }
   #else
   template<class MovableConvertible>
   iterator insert_unique_commit
      (MovableConvertible && mv, insert_commit_data &data)
   {
      NodePtr tmp = AllocHolder::create_node(forward<MovableConvertible>(mv));
      iiterator it(this->m_icont.insert_unique_commit(*tmp, data));
      return iterator(it);
   }
   #endif

   std::pair<iterator,bool> insert_unique(const value_type& v)
   {
      insert_commit_data data;
      std::pair<iterator,bool> ret =
         this->insert_unique_check(KeyOfValue()(v), data);
      if(!ret.second)
         return ret;
      return std::pair<iterator,bool>
         (this->insert_unique_commit(v, data), true);
   }

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   template<class MovableConvertible>
   std::pair<iterator,bool> insert_unique
      (const detail::moved_object<MovableConvertible>& mv)
   {
      insert_commit_data data;
      std::pair<iterator,bool> ret =
         this->insert_unique_check(KeyOfValue()(mv.get()), data);
      if(!ret.second)
         return ret;
      return std::pair<iterator,bool>
         (this->insert_unique_commit(mv, data), true);
   }
   #else
   template<class MovableConvertible>
   std::pair<iterator,bool> insert_unique(MovableConvertible &&mv)
   {
      insert_commit_data data;
      std::pair<iterator,bool> ret =
         this->insert_unique_check(KeyOfValue()(mv), data);
      if(!ret.second)
         return ret;
      return std::pair<iterator,bool>
         (this->insert_unique_commit(forward<MovableConvertible>(mv), data), true);
   }
   #endif

   iterator insert_unique(const_iterator hint, const value_type& v)
   {
      insert_commit_data data;
      std::pair<iterator,bool> ret =
         this->insert_unique_check(hint, KeyOfValue()(v), data);
      if(!ret.second)
         return ret.first;
      return this->insert_unique_commit(v, data);
   }

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   template<class MovableConvertible>
   iterator insert_unique
      (const_iterator hint, const detail::moved_object<MovableConvertible> &mv)
   {
      insert_commit_data data;
      std::pair<iterator,bool> ret =
         this->insert_unique_check(hint, KeyOfValue()(mv.get()), data);
      if(!ret.second)
         return ret.first;
      return this->insert_unique_commit(mv, data);
   }
   #else
   template<class MovableConvertible>
   iterator insert_unique
      (const_iterator hint, MovableConvertible &&mv)
   {
      insert_commit_data data;
      std::pair<iterator,bool> ret =
         this->insert_unique_check(hint, KeyOfValue()(mv), data);
      if(!ret.second)
         return ret.first;
      return this->insert_unique_commit(forward<MovableConvertible>(mv), data);
   }
   #endif

   template <class InputIterator>
   void insert_unique(InputIterator first, InputIterator last)
   {
      if(this->empty()){
         //Insert with end hint, to achieve linear
         //complexity if [first, last) is ordered
         iterator end(this->end());
         for( ; first != last; ++first)
            this->insert_unique(end, *first);
      }
      else{
         for( ; first != last; ++first)
            this->insert_unique(*first);
      }
   }

   iterator insert_equal(const value_type& v)
   {
      NodePtr p(AllocHolder::create_node(v));
      return iterator(this->m_icont.insert_equal_upper_bound(*p));
   }

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   template<class MovableConvertible>
   iterator insert_equal(const detail::moved_object<MovableConvertible> &mv)
   {
      NodePtr p(AllocHolder::create_node(mv));
      return iterator(this->m_icont.insert_equal_upper_bound(*p));
   }
   #else
   template<class MovableConvertible>
   iterator insert_equal(MovableConvertible &&mv)
   {
      NodePtr p(AllocHolder::create_node(forward<MovableConvertible>(mv)));
      return iterator(this->m_icont.insert_equal_upper_bound(*p));
   }
   #endif

   iterator insert_equal(const_iterator hint, const value_type& v)
   {
      NodePtr p(AllocHolder::create_node(v));
      return iterator(this->m_icont.insert_equal(hint.get(), *p));
   }

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   template<class MovableConvertible>
   iterator insert_equal(const_iterator hint, const detail::moved_object<MovableConvertible> &mv)
   {
      NodePtr p(AllocHolder::create_node(mv));
      return iterator(this->m_icont.insert_equal(hint.get(), *p));
   }
   #else
   template<class MovableConvertible>
   iterator insert_equal(const_iterator hint, MovableConvertible &&mv)
   {
      NodePtr p(AllocHolder::create_node(move(mv)));
      return iterator(this->m_icont.insert_equal(hint.get(), *p));
   }
   #endif

   template <class InputIterator>
   void insert_equal(InputIterator first, InputIterator last)
   {
      if(this->empty()){
         //Insert with end hint, to achieve linear
         //complexity if [first, last) is ordered
         iterator end(this->end());
         for( ; first != last; ++first)
            this->insert_equal(end, *first);
      }
      else{
         for( ; first != last; ++first)
            this->insert_equal(*first);
      }
   }

   iterator erase(const_iterator position)
   {  return iterator(this->m_icont.erase_and_dispose(position.get(), Destroyer(*this))); }

   size_type erase(const key_type& k)
   {  return this->m_icont.erase_and_dispose(k, KeyNodeCompare(value_comp()), Destroyer(*this)); }

   iterator erase(const_iterator first, const_iterator last)
   {  return iterator(this->m_icont.erase_and_dispose(first.get(), last.get(), Destroyer(*this))); }

   void clear() 
   {  this->m_icont.clear_and_dispose(Destroyer(*this)); }

   // set operations:
   iterator find(const key_type& k)
   {  return iterator(this->m_icont.find(k, KeyNodeCompare(value_comp())));  }

   const_iterator find(const key_type& k) const
   {  return const_iterator(this->non_const_icont().find(k, KeyNodeCompare(value_comp())));  }

   size_type count(const key_type& k) const
   {  return size_type(this->m_icont.count(k, KeyNodeCompare(value_comp()))); }

   iterator lower_bound(const key_type& k)
   {  return iterator(this->m_icont.lower_bound(k, KeyNodeCompare(value_comp())));  }

   const_iterator lower_bound(const key_type& k) const
   {  return const_iterator(this->non_const_icont().lower_bound(k, KeyNodeCompare(value_comp())));  }

   iterator upper_bound(const key_type& k)
   {  return iterator(this->m_icont.upper_bound(k, KeyNodeCompare(value_comp())));   }

   const_iterator upper_bound(const key_type& k) const
   {  return const_iterator(this->non_const_icont().upper_bound(k, KeyNodeCompare(value_comp())));  }

   std::pair<iterator,iterator> equal_range(const key_type& k)
   {  
      std::pair<iiterator, iiterator> ret =
         this->m_icont.equal_range(k, KeyNodeCompare(value_comp()));
      return std::pair<iterator,iterator>(iterator(ret.first), iterator(ret.second));
   }

   std::pair<const_iterator, const_iterator> equal_range(const key_type& k) const
   {  
      std::pair<iiterator, iiterator> ret =
         this->non_const_icont().equal_range(k, KeyNodeCompare(value_comp()));
      return std::pair<const_iterator,const_iterator>
         (const_iterator(ret.first), const_iterator(ret.second));
   }

   private:
   //Iterator range version
   template<class InpIterator>
   void priv_create_and_insert_nodes
      (InpIterator beg, InpIterator end, bool unique)
   {
      typedef typename std::iterator_traits<InpIterator>::iterator_category ItCat;
      priv_create_and_insert_nodes(beg, end, unique, alloc_version(), ItCat());
   }

   template<class InpIterator>
   void priv_create_and_insert_nodes
      (InpIterator beg, InpIterator end, bool unique, allocator_v1, std::input_iterator_tag)
   {
      if(unique){
         for (; beg != end; ++beg){
            this->insert_unique(*beg);
         }
      }
      else{
         for (; beg != end; ++beg){
            this->insert_equal(*beg);
         }
      }
   }

   template<class InpIterator>
   void priv_create_and_insert_nodes
      (InpIterator beg, InpIterator end, bool unique, allocator_v2, std::input_iterator_tag)
   {  //Just forward to the default one
      priv_create_and_insert_nodes(beg, end, unique, allocator_v1(), std::input_iterator_tag());
   }

   template<class FwdIterator>
   void priv_create_and_insert_nodes
      (FwdIterator beg, FwdIterator end, bool unique, allocator_v2, std::forward_iterator_tag)
   {
      if(unique){
         priv_create_and_insert_nodes(beg, end, unique, allocator_v2(), std::input_iterator_tag());
      }
      else{
         //Optimize memory allocation obtaining the distance between iterators
         size_type n = std::distance(beg, end);

         //Allocate and construct as many nodes as possible with
         //the one-shot allocation
         typedef typename NodeAlloc::multiallocation_iterator multiallocation_iterator;
         multiallocation_iterator many_beg, itend, it;
         size_type received_array;
         FwdIterator next = this->allocate_many_and_construct
            (beg, n, many_beg, received_array);

         detail::multiallocation_destroy_dealloc<NodeAlloc>
            multi_destroy_dealloc(many_beg, *this);

         //Insert constructed nodes (this does not throw)
         for (it = many_beg; it != itend; ++it){
            this->m_icont.insert_equal_upper_bound(*it);
            multi_destroy_dealloc.next();
         }

         //Insert remaining nodes using individual allocation
         //(this can throw, but there is no leak)
         for (size_type i = received_array; i < n; ++i, ++next){
            this->insert_equal(*next);
         }
      }
   }
};

template <class Key, class Value, class KeyOfValue, 
          class KeyCompare, class A>
inline bool 
operator==(const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& x, 
           const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& y)
{
  return x.size() == y.size() &&
         std::equal(x.begin(), x.end(), y.begin());
}

template <class Key, class Value, class KeyOfValue, 
          class KeyCompare, class A>
inline bool 
operator<(const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& x, 
          const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& y)
{
  return std::lexicographical_compare(x.begin(), x.end(), 
                                      y.begin(), y.end());
}

template <class Key, class Value, class KeyOfValue, 
          class KeyCompare, class A>
inline bool 
operator!=(const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& x, 
           const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& y) {
  return !(x == y);
}

template <class Key, class Value, class KeyOfValue, 
          class KeyCompare, class A>
inline bool 
operator>(const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& x, 
          const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& y) {
  return y < x;
}

template <class Key, class Value, class KeyOfValue, 
          class KeyCompare, class A>
inline bool 
operator<=(const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& x, 
           const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& y) {
  return !(y < x);
}

template <class Key, class Value, class KeyOfValue, 
          class KeyCompare, class A>
inline bool 
operator>=(const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& x, 
           const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& y) {
  return !(x < y);
}


template <class Key, class Value, class KeyOfValue, 
          class KeyCompare, class A>
inline void 
swap(rbtree<Key,Value,KeyOfValue,KeyCompare,A>& x, 
     rbtree<Key,Value,KeyOfValue,KeyCompare,A>& y)
{
  x.swap(y);
}

} //namespace detail {

//!This class is movable
template <class T, class V, class K, class C, class A>
struct is_movable<detail::rbtree<T, V, K, C, A> >
{
   enum {   value = true };
};

//!This class is movable
template <class T, class VoidPointer>
struct is_movable<detail::rbtree_node<T, VoidPointer> >
{
   enum {   value = true };
};

//!This class is movable
/*
template <class A, class C>
struct is_movable<detail::rbtree_alloc<A, C> >
{
   enum {   value = true };
};
*/

//!has_trivial_destructor_after_move<> == true_type
//!specialization for optimizations
template <class K, class V, class KOV, 
          class C, class A>
struct has_trivial_destructor_after_move<detail::rbtree<K, V, KOV, C, A> >
{
   enum {   value = 
               has_trivial_destructor<A>::value &&
               has_trivial_destructor<C>::value  };
};


} //namespace interprocess  {
} //namespace boost  {

#include <boost/interprocess/detail/config_end.hpp>

#endif //BOOST_INTERPROCESS_TREE_HPP
