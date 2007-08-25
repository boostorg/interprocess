//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_NODE_ALLOC_HPP_
#define BOOST_INTERPROCESS_DETAIL_NODE_ALLOC_HPP_

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/detail/version_type.hpp>
#include <boost/interprocess/detail/move.hpp>
#include <boost/interprocess/detail/type_traits.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/algorithms.hpp>
#include <boost/interprocess/detail/mpl.hpp>

#include <utility>
#include <functional>


namespace boost {
namespace interprocess {
namespace detail {

template<class ValueCompare, class Node>
struct node_compare
   :  private ValueCompare
{
   typedef typename ValueCompare::key_type     key_type;
   typedef typename ValueCompare::value_type   value_type;
   typedef typename ValueCompare::key_of_value key_of_value;

   node_compare(const ValueCompare &pred)
      :  ValueCompare(pred)
   {}

   ValueCompare &value_comp()
   {  return static_cast<ValueCompare &>(*this);  }

   ValueCompare &value_comp() const
   {  return static_cast<const ValueCompare &>(*this);  }

   bool operator()(const Node &a, const Node &b) const
   {  return ValueCompare::operator()(a.m_data, b.m_data);  }
};

template<class A, class ICont>
struct node_alloc_holder
   :  public A::template rebind<typename ICont::value_type>::other
{
   typedef node_alloc_holder<A, ICont>                self_t;
   typedef typename A::value_type                     value_type;
   typedef typename ICont::value_type                 Node;
   typedef typename A::template rebind<Node>::other   NodeAlloc;
   typedef A                                          ValAlloc;
   typedef typename NodeAlloc::pointer                NodePtr;
   typedef detail::scoped_deallocator<NodeAlloc>      Deallocator;
   typedef typename NodeAlloc::size_type              size_type;
   typedef typename NodeAlloc::difference_type        difference_type;
   typedef detail::integral_constant<unsigned, 1>     allocator_v1;
   typedef detail::integral_constant<unsigned, 2>     allocator_v2;
   typedef detail::integral_constant<unsigned,
      boost::interprocess::detail::
         version<NodeAlloc>::value>                   alloc_version;

   node_alloc_holder(const ValAlloc &a) 
      : NodeAlloc(a)
   {}

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   node_alloc_holder(const detail::moved_object<ValAlloc> &a) 
      : NodeAlloc(a.get())
   {}
   #else
   node_alloc_holder(ValAlloc &&a) 
      : NodeAlloc(a)
   {}
   #endif

   node_alloc_holder(const node_alloc_holder &other)
      : NodeAlloc(other)
   {}

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   node_alloc_holder(const detail::moved_object<node_alloc_holder> &other)
      : NodeAlloc(move((NodeAlloc&)other.get()))
   {  this->swap(other.get());  }
   #else
   node_alloc_holder(node_alloc_holder &&other)
      : NodeAlloc(move((NodeAlloc&)other))
   {  this->swap(other);  }
   #endif

   template<class Pred>
   node_alloc_holder(const ValAlloc &a, const Pred &c) 
      : NodeAlloc(a), m_icont(typename ICont::value_compare(c))
   {}

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   template<class Pred>
   node_alloc_holder(const detail::moved_object<ValAlloc> &a, const Pred &c) 
      : NodeAlloc(a.get()), m_icont(typename ICont::value_compare(c))
   {}
   #else
   template<class Pred>
   node_alloc_holder(ValAlloc &&a, const Pred &c) 
      : NodeAlloc(a), m_icont(typename ICont::value_compare(c))
   {}
   #endif

   template<class Pred>
   node_alloc_holder(const node_alloc_holder &other, const Pred &c)
      : NodeAlloc(other), m_icont(typename ICont::value_compare(c))
   {}

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   template<class Pred>
   node_alloc_holder
      (const detail::moved_object<node_alloc_holder> &other, const Pred &c)
      : NodeAlloc(move((NodeAlloc&)other.get())), m_icont(typename ICont::value_compare(c))
   {  this->swap(other.get());  }
   #else
   template<class Pred>
   node_alloc_holder(node_alloc_holder &&other, const Pred &c)
      : NodeAlloc(move((NodeAlloc&)other)), m_icont(typename ICont::value_compare(c))
   {  this->swap(other);  }
   #endif

   ~node_alloc_holder()
   {}

   size_type max_size() const
   {  return NodeAlloc::max_size();  }

   NodePtr allocate_one()
   {  return this->allocate_one(alloc_version());   }

   NodePtr allocate_one(allocator_v1)
   {  return NodeAlloc::allocate(1);   }

   NodePtr allocate_one(allocator_v2)
   {  return NodeAlloc::allocate_one();   }

   void deallocate_one(NodePtr p)
   {  return this->deallocate_one(p, alloc_version());   }

   void deallocate_one(NodePtr p, allocator_v1)
   {  NodeAlloc::deallocate(p, 1);   }

   void deallocate_one(NodePtr p, allocator_v2)
   {  NodeAlloc::deallocate_one(p);   }

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   template<class Convertible>
   static void construct(const NodePtr &ptr, const Convertible &value)
   {  new(detail::get_pointer(ptr)) Node(value);  }
   #else
   template<class Convertible>
   static void construct(const NodePtr &ptr, Convertible &&value)
   {  new(detail::get_pointer(ptr)) Node(forward<Convertible>(value));  }
   #endif

   static void construct(const NodePtr &ptr)
   {  new(detail::get_pointer(ptr)) Node();  }

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   template<class Convertible1, class Convertible2>
   static void construct(const NodePtr &ptr, 
                         const detail::moved_object<std::pair<Convertible1, Convertible2> > &value)
   {
      typedef typename Node::hook_type                hook_type;
      typedef typename Node::value_type::first_type   first_type;
      typedef typename Node::value_type::second_type  second_type;
      Node *nodeptr = detail::get_pointer(ptr);

      //Hook constructor does not throw
      new(static_cast<hook_type*>(nodeptr))hook_type();
      //Now construct pair members
      value_type *valueptr = &nodeptr->m_data;
      new((void*)&valueptr->first) first_type(move(value.get().first));
      BOOST_TRY{
         new((void*)&valueptr->second) second_type(move(value.get().second));
      }
      BOOST_CATCH(...){
         valueptr->first.~first_type();
         static_cast<hook_type*>(nodeptr)->~hook_type();
         throw;
      }
      BOOST_CATCH_END
   }
   #else
   template<class Convertible1, class Convertible2>
   static void construct(const NodePtr &ptr, 
                         std::pair<Convertible1, Convertible2> &&value)
   {  
      typedef typename Node::hook_type                hook_type;
      typedef typename Node::value_type::first_type   first_type;
      typedef typename Node::value_type::second_type  second_type;
      Node *nodeptr = detail::get_pointer(ptr);

      //Hook constructor does not throw
      new(static_cast<hook_type*>(nodeptr))hook_type();
      //Now construct pair members
      value_type *valueptr = &nodeptr->m_data;
      new((void*)&valueptr->first) first_type(move(value.first));
      BOOST_TRY{
         new((void*)&valueptr->second) second_type(move(value.second));
      }
      BOOST_CATCH(...){
         valueptr->first.~first_type();
         static_cast<hook_type*>(nodeptr)->~hook_type();
         throw;
      }
      BOOST_CATCH_END
   }
   #endif

   static void destroy(const NodePtr &ptr)
   {  detail::get_pointer(ptr)->~Node();  }

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   template<class Convertible>
   NodePtr create_node(const Convertible& x)
   {
      NodePtr p = this->allocate_one();
      Deallocator node_deallocator(p, *this);
      self_t::construct(p, x);
      node_deallocator.release();
      return (p);
   }
   #else
   template<class Convertible>
   NodePtr create_node(Convertible &&x)
   {
      NodePtr p = this->allocate_one();
      Deallocator node_deallocator(p, *this);
      self_t::construct(p, forward<Convertible>(x));
      node_deallocator.release();
      return (p);
   }
   #endif

   template<class It>
   NodePtr create_node_from_it(It it)
   {
      NodePtr p = this->allocate_one();
      Deallocator node_deallocator(p, *this);
      construct_in_place(detail::get_pointer(p), it);
      node_deallocator.release();
      return (p);
   }

   NodePtr create_node()
   {
      NodePtr p = this->allocate_one();
      Deallocator node_deallocator(p, *this);
      self_t::construct(p);
      node_deallocator.release();
      return (p);
   }

   void destroy_node(NodePtr node)
   {
      self_t::destroy(node);
      this->deallocate_one(node);
   }

   void swap(node_alloc_holder &x)
   {
      NodeAlloc& this_alloc   = static_cast<NodeAlloc&>(*this);
      NodeAlloc& other_alloc  = static_cast<NodeAlloc&>(x);

      if (this_alloc != other_alloc){
         detail::do_swap(this_alloc, other_alloc);
      }

      this->m_icont.swap(x.m_icont);
   }

   template<class FwdIterator, class MultiAllocator>
   FwdIterator allocate_many_and_construct
      (FwdIterator beg, difference_type n, MultiAllocator &multi_beg, size_type &constructed)
   {
      typedef typename NodeAlloc::multiallocation_iterator multiallocation_iterator;

      //Try to allocate memory in a single chunk
      MultiAllocator itbeg = NodeAlloc::allocate_individual(1, n, constructed), itend, it;

      //Prepare exception-safety machinery
      detail::multiallocation_deallocator<NodeAlloc> multi_deallocator(itbeg, *this);

      //Initialize all the data (this can throw)
      FwdIterator next = 
         boost::interprocess::n_uninitialized_copy_n(beg, constructed, itbeg);

      //Exception-unsafe zone passed. Disable auto-deallocation
      multi_deallocator.release();
      multi_beg = itbeg;
      return next;
   }

   protected:
   struct cloner
   {
      cloner(node_alloc_holder &holder)
         :  m_holder(holder)
      {}

      NodePtr operator()(const Node &other) const
      {  return m_holder.create_node(other.m_data);  }

      node_alloc_holder &m_holder;
   };

   struct destroyer
   {
      destroyer(node_alloc_holder &holder)
         :  m_holder(holder)
      {}

      void operator()(NodePtr n) const
      {  m_holder.destroy_node(n);  }

      node_alloc_holder &m_holder;
   };

   //The intrusive container
   ICont m_icont;

   ICont &non_const_icont() const
   {  return const_cast<ICont&>(this->m_icont);   }
};

}  //namespace detail {
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif // BOOST_INTERPROCESS_DETAIL_NODE_ALLOC_HPP_
