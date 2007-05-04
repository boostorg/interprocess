//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
//
// This file comes from SGI's stl_vector.h file. Modified by Ion Gaztañaga.
// Renaming, isolating and porting to generic algorithms. Pointer typedef 
// set to allocator::pointer to allow placing it in shared memory.
//
///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1994
// Hewlett-Packard Company
// 
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  Hewlett-Packard Company makes no
// representations about the suitability of this software for any
// purpose.  It is provided "as is" without express or implied warranty.
// 
// 
// Copyright (c) 1996
// Silicon Graphics Computer Systems, Inc.
// 
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  Silicon Graphics makes no
// representations about the suitability of this software for any
// purpose.  It is provided "as is" without express or implied warranty.

#ifndef BOOST_INTERPROCESS_VECTOR_HPP
#define BOOST_INTERPROCESS_VECTOR_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/smart_ptr/scoped_ptr.hpp>
#include <boost/iterator.hpp>
#include <boost/iterator/reverse_iterator.hpp>
#include <boost/interprocess/detail/move_iterator.hpp>
#include <boost/interprocess/detail/move.hpp>
#include <cstddef>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <iterator>
#include <utility>
#include <boost/detail/no_exceptions_support.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/interprocess/detail/version_type.hpp>
#include <boost/interprocess/allocators/allocation_type.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_scalar.hpp>
#include <boost/type_traits/has_trivial_destructor.hpp>

namespace boost {
namespace interprocess {

/// @cond
namespace detail {
//!This struct deallocates and allocated memory
template <class A>
struct vector_alloc_holder 
   :  public A
{
   typedef typename A::pointer      pointer;
   typedef typename A::size_type    size_type;
   typedef typename A::value_type   value_type;

   enum {   trivial_dctr_after_move = has_trivial_destructor_after_move<value_type>::value   };

   //Constructor, does not throw
   vector_alloc_holder(const A &a)
      : A(a), m_start(0), m_size(0), m_capacity(0)
   {}

   //Constructor, does not throw
   vector_alloc_holder(const vector_alloc_holder<A> &h)
      : A((A&)h), m_start(0), m_size(0), m_capacity(0)
   {}

   //Constructor, does not throw
   vector_alloc_holder(const detail::moved_object<vector_alloc_holder<A> > &h)
      : A(move((A&)h.get())), m_start(0), m_size(0), m_capacity(0)
   {}

   //Destructor
   ~vector_alloc_holder()
   {  this->prot_deallocate(); }

   typedef boost::integral_constant<unsigned, 1>      allocator_v1;
   typedef boost::integral_constant<unsigned, 2>      allocator_v2;
   typedef boost::integral_constant<unsigned,
      boost::interprocess::detail::version<A>::value> alloc_version;

   std::pair<pointer, bool>
      allocation_command(allocation_type command,
                         size_type limit_size, 
                         size_type preferred_size,
                         size_type &received_size, const pointer &reuse = 0)
   {
      return allocation_command(command, limit_size, preferred_size,
                               received_size, reuse, alloc_version());
   }

   std::pair<pointer, bool>
      allocation_command(allocation_type command,
                         size_type limit_size, 
                         size_type preferred_size,
                         size_type &received_size,
                         const pointer &reuse,
                         allocator_v1)
   {
      if(!(command & allocate_new))
         return std::pair<pointer, bool>(0, 0);
      received_size = preferred_size;
      return std::make_pair(A::allocate(received_size), false);
   }

   std::pair<pointer, bool>
      allocation_command(allocation_type command,
                         size_type limit_size, 
                         size_type preferred_size,
                         size_type &received_size,
                         const pointer &reuse,
                         allocator_v2)
   {
      return A::allocation_command(command, limit_size, preferred_size, 
                                   received_size, reuse);
   }

   size_type next_capacity(size_type additional_objects) const
   {  return get_next_capacity(A::max_size(), this->m_capacity, additional_objects);  }

   pointer     m_start;
   size_type   m_size;
   size_type   m_capacity;

   protected:
   void prot_deallocate()
   {
      if(!m_start)   return;
      this->deallocate(m_start, m_capacity);
      m_start     = 0;
      m_size      = 0;
      m_capacity  = 0;
   }

   void destroy(pointer p)
   {
      if(!has_trivial_destructor<value_type>::value)
         A::destroy(p);
   }

   void destroy_n(pointer p, size_type n)
   {
      if(!has_trivial_destructor<value_type>::value)
         for(; n--; ++p) this->destroy(p);
   }
};

}  //namespace detail {
/// @endcond

//! A vector is a sequence that supports random access to elements, constant 
//! time insertion and removal of elements at the end, and linear time insertion 
//! and removal of elements at the beginning or in the middle. The number of 
//! elements in a vector may vary dynamically; memory management is automatic.
//! boost::interprocess::vector is similar to std::vector but it's compatible
//! with shared memory and memory mapped files.
template <class T, class A>
class vector : private detail::vector_alloc_holder<A>
{
   /// @cond
   typedef vector<T, A>                   self_t;
   typedef detail::vector_alloc_holder<A> base_t;
   typedef detail::scoped_array_deallocator<A> dealloc_t;
   /// @endcond
   public:
   //! The type of object, T, stored in the vector
   typedef T                              value_type;
   //! Pointer to T
   typedef typename A::pointer            pointer;
   //! Const pointer to T
   typedef typename A::const_pointer      const_pointer;
   //! Reference to T
   typedef typename A::reference          reference;
   //! Const reference to T
   typedef typename A::const_reference    const_reference;
   //! An unsigned integral type
   typedef typename A::size_type          size_type;
   //! A signed integral type
   typedef typename A::difference_type    difference_type;
   //! The allocator type
   typedef A                              allocator_type;
   /// @cond
   private:
   //This shouldn't be needed but VC6.0 needs this
   typedef typename A::pointer            vec_ptr;
   typedef typename A::const_pointer      vec_cptr;
   typedef typename A::reference          vec_ref;
   typedef typename A::const_reference    vec_cref;
   typedef typename A::difference_type    vec_diff;

   typedef constant_iterator<T, difference_type> cvalue_iterator;
   /// @endcond
   public:

   //! Const iterator used to iterate through a vector. 
   class const_iterator
      : public boost::iterator<std::random_access_iterator_tag
                              ,value_type
                              ,vec_diff
                              ,vec_cptr
                              ,vec_cref>
   {
      /// @cond
      private:
      vec_ptr get_ptr() const    {  return   m_ptr;  }

      protected:
      vec_ptr m_ptr;
      explicit const_iterator(vec_ptr ptr)  : m_ptr(ptr){}
      /// @endcond
      public:
      friend class vector<T, A>;
      typedef vec_diff        difference_type;

      //Constructors
      const_iterator() : m_ptr(0){}

      //Pointer like operators
      const_reference operator*()   const  
      {  return *m_ptr;  }

      const_pointer   operator->()  const  
      {  return  m_ptr;  }

      const_reference operator[](difference_type off) const
      {  return m_ptr[off];   }

      //Increment / Decrement
      const_iterator& operator++()       
      { ++m_ptr;  return *this; }

      const_iterator operator++(int)      
      { vec_ptr tmp = m_ptr; ++*this; return const_iterator(tmp);  }

      const_iterator& operator--()
      {   --m_ptr; return *this;   }

      const_iterator operator--(int)
      {  vec_ptr tmp = m_ptr; --*this; return const_iterator(tmp); }

      //Arithmetic
      const_iterator& operator+=(difference_type off)
      {  m_ptr += off; return *this;   }

      const_iterator operator+(difference_type off) const
      {  return const_iterator(m_ptr+off);  }

      friend const_iterator operator+(difference_type off, const const_iterator& right)
      {  return const_iterator(off + right.m_ptr); }

      const_iterator& operator-=(difference_type off)
      {  m_ptr -= off; return *this;   }

      const_iterator operator-(difference_type off) const
      {  return const_iterator(m_ptr-off);  }

      difference_type operator-(const const_iterator& right) const
      {  return m_ptr - right.m_ptr;   }

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

   //! Iterator used to iterate through a vector
   class iterator
      :  public const_iterator
   {
      protected:
      explicit iterator(vec_ptr ptr) : const_iterator(ptr){}

      public:
      friend class vector<T, A>;
      typedef vec_ptr   pointer;
      typedef vec_ref   reference;

      //Constructors
      iterator(){}

      //Pointer like operators
      reference operator*()  const  
      {  return *this->m_ptr;  }

      pointer   operator->() const  
      {  return  this->m_ptr;  }

      reference operator[](difference_type off) const 
      {  return this->m_ptr[off];   }

      //Increment / Decrement
      iterator& operator++()  
      { ++this->m_ptr; return *this;  }

      iterator operator++(int)
      { pointer tmp = this->m_ptr; ++*this; return iterator(tmp); }
      
      iterator& operator--()
      {  --this->m_ptr; return *this;  }

      iterator operator--(int)
      {  iterator tmp = *this; --*this; return iterator(tmp); }

      // Arithmetic
      iterator& operator+=(difference_type off)
      {  this->m_ptr += off;  return *this;  }

      iterator operator+(difference_type off) const
      {  return iterator(this->m_ptr+off);  }

      friend iterator operator+(difference_type off, const iterator& right)
      {  return iterator(off + right.m_ptr); }

      iterator& operator-=(difference_type off)
      {  this->m_ptr -= off; return *this;   }

      iterator operator-(difference_type off) const
      {  return iterator(this->m_ptr-off);  }

      difference_type operator-(const const_iterator& right) const
      {  return *((const_iterator*)this) - right;   }
   };

   //! Iterator used to iterate backwards through a vector. 
   typedef typename boost::reverse_iterator<iterator>   
      reverse_iterator;
   //! Const iterator used to iterate backwards through a vector. 
   typedef typename boost::reverse_iterator<const_iterator>                 
      const_reverse_iterator;

   public:

   //! <b>Effects</b>: Constructs a vector taking the allocator as parameter.
   //! 
   //! <b>Throws</b>: If allocator_type's copy constructor throws.
   //! 
   //! <b>Complexity</b>: Constant.
   explicit vector(const A& a = A())
      : base_t(a)
   {}

   //! <b>Effects</b>: Constructs a vector that will use a copy of allocator a
   //!   and inserts n copies of value.
   //!
   //! <b>Throws</b>: If allocator_type's default constructor or copy constructor
   //!   throws or T's default or copy constructor throws.
   //! 
   //! <b>Complexity</b>: Linear to n.
   vector(size_type n, const T& value = T(),
          const allocator_type& a = allocator_type()) 
      :  base_t(a)
   {  this->insert(this->end(), n, value); }

   //! <b>Effects</b>: Copy constructs a vector.
   //!
   //! <b>Postcondition</b>: x == *this.
   //! 
   //! <b>Complexity</b>: Linear to the elements x contains.
   vector(const vector<T, A>& x) 
      :  base_t((base_t&)x)
   {  *this = x;  }

   //! <b>Effects</b>: Move constructor. Moves mx's resources to *this.
   //!
   //! <b>Throws</b>: If allocator_type's copy constructor throws.
   //! 
   //! <b>Complexity</b>: Constant.
   vector(const detail::moved_object<vector<T, A> >& mx) 
      :  base_t(move((base_t&)mx.get()))
   {
      vector<T, A> &x = mx.get();
      this->m_start     = x.m_start;
      this->m_size      = x.m_size;
      this->m_capacity  = x.m_capacity;
      x.m_start         = 0;
      x.m_size          = 0;
      x.m_capacity      = 0;
   }

   //! <b>Effects</b>: Constructs a vector that will use a copy of allocator a
   //!   and inserts a copy of the range [first, last) in the vector.
   //!
   //! <b>Throws</b>: If allocator_type's default constructor or copy constructor
   //!   throws or T's constructor taking an dereferenced InIt throws.
   //!
   //! <b>Complexity</b>: Linear to the range [first, last).
   template <class InIt>
   vector(InIt first, InIt last, const allocator_type& a = allocator_type())
      :  base_t(a)
   {  this->assign(first, last); }

   //! <b>Effects</b>: Destroys the vector. All stored values are destroyed
   //!   and used memory is deallocated.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of elements.
   ~vector() 
   {  this->priv_destroy_all();  }

   //! <b>Effects</b>: Returns an iterator to the first element contained in the vector.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   iterator begin()      
   { return iterator(this->m_start); }

   //! <b>Effects</b>: Returns a const_iterator to the first element contained in the vector.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_iterator begin() const
   { return const_iterator(this->m_start); }

   //! <b>Effects</b>: Returns an iterator to the end of the vector.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   iterator end()        
   { return iterator(this->m_start + this->m_size); }

   //! <b>Effects</b>: Returns a const_iterator to the end of the vector.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_iterator end()   const
   { return const_iterator(this->m_start + this->m_size); }

   //! <b>Effects</b>: Returns a reverse_iterator pointing to the beginning 
   //! of the reversed vector. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   reverse_iterator rbegin()     
   { return reverse_iterator(this->end());      }

   //! <b>Effects</b>: Returns a const_reverse_iterator pointing to the beginning 
   //! of the reversed vector. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_reverse_iterator rbegin()const
   { return const_reverse_iterator(this->end());}

   //! <b>Effects</b>: Returns a reverse_iterator pointing to the end
   //! of the reversed vector. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   reverse_iterator rend()       
   { return reverse_iterator(this->begin());       }

   //! <b>Effects</b>: Returns a const_reverse_iterator pointing to the end
   //! of the reversed vector. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_reverse_iterator rend()  const
   { return const_reverse_iterator(this->begin()); }

   //! <b>Requires</b>: !empty()
   //!
   //! <b>Effects</b>: Returns a reference to the first element 
   //!   from the beginning of the container.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   reference         front()       
   { return *this->m_start; }

   //! <b>Requires</b>: !empty()
   //!
   //! <b>Effects</b>: Returns a const reference to the first element 
   //!   from the beginning of the container.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_reference   front() const 
   { return *this->m_start; }

   //! <b>Requires</b>: !empty()
   //!
   //! <b>Effects</b>: Returns a reference to the first element 
   //!   from the beginning of the container.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   reference         back()        
   { return this->m_start[this->m_size - 1]; }

   //! <b>Requires</b>: !empty()
   //!
   //! <b>Effects</b>: Returns a const reference to the first element 
   //!   from the beginning of the container.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_reference   back()  const 
   { return this->m_start[this->m_size - 1]; }

   //! <b>Effects</b>: Returns the number of the elements contained in the vector.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   size_type size() const 
   { return this->m_size; }

   //! <b>Effects</b>: Returns the largest possible size of the vector.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   size_type max_size() const 
   { return allocator_type::max_size(); }

   //! <b>Effects</b>: Number of elements for which memory has been allocated.
   //!   capacity() is always greater than or equal to size().
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   size_type capacity() const 
   { return this->m_capacity; }

   //! <b>Effects</b>: Returns true if the vector contains no elements.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   bool empty() const 
   { return !this->m_size; }

   //! <b>Requires</b>: size() < n.
   //!
   //! <b>Effects</b>: Returns a reference to the nth element 
   //!   from the beginning of the container.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   reference operator[](size_type n)         
   { return this->m_start[n]; }

   //! <b>Requires</b>: size() < n.
   //!
   //! <b>Effects</b>: Returns a const reference to the nth element 
   //!   from the beginning of the container.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_reference operator[](size_type n) const   
   { return this->m_start[n]; }

   //! <b>Requires</b>: size() < n.
   //!
   //! <b>Effects</b>: Returns a reference to the nth element 
   //!   from the beginning of the container.
   //! 
   //! <b>Throws</b>: std::range_error if n >= size()
   //! 
   //! <b>Complexity</b>: Constant.
   reference at(size_type n)
   { this->priv_check_range(n); return this->m_start[n]; }

   //! <b>Requires</b>: size() < n.
   //!
   //! <b>Effects</b>: Returns a const reference to the nth element 
   //!   from the beginning of the container.
   //! 
   //! <b>Throws</b>: std::range_error if n >= size()
   //! 
   //! <b>Complexity</b>: Constant.
   const_reference at(size_type n) const
   { this->priv_check_range(n); return this->m_start[n]; }

   //! <b>Effects</b>: Returns a copy of the internal allocator.
   //! 
   //! <b>Throws</b>: If allocator's copy constructor throws.
   //! 
   //! <b>Complexity</b>: Constant.
   allocator_type get_allocator() const 
   { return *this;  }

   //! <b>Effects</b>: If n is less than or equal to capacity(), this call has no
   //!   effect. Otherwise, it is a request for allocation of additional memory.
   //!   If the request is successful, then capacity() is greater than or equal to
   //!   n; otherwise, capacity() is unchanged. In either case, size() is unchanged.
   //! 
   //! <b>Throws</b>: If memory allocation allocation throws or T's copy constructor throws.
   void reserve(size_type new_cap)
   {
      if (this->capacity() < new_cap){
         //There is not enough memory, allocate a new
         //buffer or expand the old one.
         bool same_buffer_start;
         std::pair<pointer, bool> ret =
            this->allocation_command
               (allocate_new | expand_fwd | expand_bwd,
                  new_cap, new_cap, new_cap, this->m_start);

         //Check for forward expansion
         same_buffer_start = ret.second && this->m_start == ret.first;
         if(same_buffer_start){
            this->m_capacity  = new_cap;
         }

         //If there is no forward expansion, move objects
         if (!same_buffer_start){
            //We will reuse insert code, so create a dummy input iterator
            detail::move_iterator<pointer> dummy_it(this->m_start);
            //Backwards (and possibly forward) expansion
            if(ret.second){
               this->priv_range_insert_expand_backwards
                  (ret.first, new_cap, this->m_start, dummy_it, dummy_it, 0);
            }
            //New buffer
            else{
               this->priv_range_insert_new_allocation
                  (ret.first, new_cap, this->m_start, dummy_it, dummy_it);
            }
         }
      }
   }

   //! <b>Effects</b>: Makes *this contain the same elements as x.
   //!
   //! <b>Postcondition</b>: this->size() == x.size(). *this contains a copy 
   //! of each of x's elements. 
   //!
   //! <b>Throws</b>: If memory allocation throws or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to the number of elements in x.
   vector<T, A>& operator=(const vector<T, A>& x)
   {
      if (&x != this){
         this->assign(x.m_start, x.m_start + x.m_size);
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
   vector<T, A>& operator=(const detail::moved_object<vector<T, A> >& mx)
   {
      vector<T, A> &x = mx.get();

      if (&x != this){
         this->swap(x);
         x.clear();
//?         base_t::prot_deallocate();
      }
      return *this;
   }

   //! <b>Effects</b>: Assigns the n copies of val to *this.
   //!
   //! <b>Throws</b>: If memory allocation throws or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to n.
   void assign(size_type n, const value_type& val)
   {  this->assign(cvalue_iterator(val, n), cvalue_iterator());   }

   //! <b>Effects</b>: Assigns the the range [first, last) to *this.
   //!
   //! <b>Throws</b>: If memory allocation throws or
   //!   T's constructor from dereferencing InpIt throws.
   //!
   //! <b>Complexity</b>: Linear to n.
   template <class InIt>
   void assign(InIt first, InIt last) 
   {
      //Dispatch depending on integer/iterator
      const bool aux_boolean = boost::is_integral<InIt>::value;
      typedef boost::mpl::bool_<aux_boolean> Result;
      this->priv_assign_dispatch(first, last, Result());
   }

   //! <b>Effects</b>: Inserts a copy of x at the end of the vector.
   //!
   //! <b>Throws</b>: If memory allocation throws or
   //!   T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   void push_back(const T& x) 
   {
      if (this->m_size < this->m_capacity){
         //There is more memory, just construct a new object at the end
         this->construct(this->m_start + this->m_size, x);
         ++this->m_size;
      }
      else{
         this->insert(this->end(), x);
      }
   }

   //! <b>Effects</b>: Constructs a new element in the end of the vector
   //!   and moves the resources of mx to this new element.
   //!
   //! <b>Throws</b>: If memory allocation throws.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   void push_back(const detail::moved_object<T> & mx) 
   {
      if (this->m_size < this->m_capacity){
         //There is more memory, just construct a new object at the end
         this->construct(this->m_start + this->m_size, mx);
         ++this->m_size;
      }
      else{
         this->insert(this->end(), mx);
      }
   }
   
   //! <b>Effects</b>: Swaps the contents of *this and x.
   //!   If this->allocator_type() != x.allocator_type()
   //!   allocators are also swapped.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   void swap(vector<T, A>& x) 
   {
      allocator_type &this_al = *this, &other_al = x;
      //Just swap internals
      detail::do_swap(this->m_start, x.m_start);
      detail::do_swap(this->m_size, x.m_size);
      detail::do_swap(this->m_capacity, x.m_capacity);

      if (this_al != other_al){
         detail::do_swap(this_al, other_al);
      }
   }

   //! <b>Effects</b>: Swaps the contents of *this and x.
   //!   If this->allocator_type() != x.allocator_type()
   //!   allocators are also swapped.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   void swap(const detail::moved_object<vector<T, A> >& mx) 
   {
      vector<T, A> &x = mx.get();
      this->swap(x);
   }

   //! <b>Requires</b>: position must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Insert a copy of x before position.
   //!
   //! <b>Throws</b>: If memory allocation throws or x's copy constructor throws.
   //!
   //! <b>Complexity</b>: If position is begin() or end(), amortized constant time
   //!   Linear time otherwise.
   iterator insert(iterator position, const T& x) 
   {
      //Just call more general insert(pos, size, value) and return iterator
      size_type n = position - begin();
      this->insert(position, (size_type)1, x);
      return iterator(this->m_start + n);
   }

   //! <b>Requires</b>: position must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Insert a new element before position with mx's resources.
   //!
   //! <b>Throws</b>: If memory allocation throws.
   //!
   //! <b>Complexity</b>: If position is begin() or end(), amortized constant time
   //!   Linear time otherwise.
   iterator insert(iterator position, const detail::moved_object<T> &mx) 
   {
      typedef repeat_iterator<T, difference_type> r_iterator;
      typedef detail::move_iterator<r_iterator> move_it;
      //Just call more general insert(pos, size, value) and return iterator
      size_type n = position - begin();
      this->insert(position
                  ,move_it(r_iterator(mx.get(), 1))
                  ,move_it(r_iterator()));
      return iterator(this->m_start + n);
   }

   //! <b>Requires</b>: pos must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Insert a copy of the [first, last) range before pos.
   //!
   //! <b>Throws</b>: If memory allocation throws, T's constructor from a
   //!   dereferenced InpIt throws or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to std::distance [first, last).
   template <class InIt>
   void insert(iterator pos, InIt first, InIt last)
   {
      //Dispatch depending on integer/iterator
      const bool aux_boolean = boost::is_integral<InIt>::value;
      typedef boost::mpl::bool_<aux_boolean> Result;
      this->priv_insert_dispatch(pos, first, last, Result());
   }

   //! <b>Requires</b>: pos must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Insert n copies of x before pos.
   //!
   //! <b>Throws</b>: If memory allocation throws or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to n.
   void insert (iterator p, size_type n, const T& x)
   {  this->insert(p, cvalue_iterator(x, n), cvalue_iterator()); }

   //! <b>Effects</b>: Removes the last element from the vector.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant time.
   void pop_back() 
   {
      //Destroy last element
      --this->m_size;
      this->destroy(this->m_start + this->m_size);
   }

   //! <b>Effects</b>: Erases the element at position pos.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the elements between pos and the 
   //!   last element. Constant if pos is the first or the last element.
   iterator erase(const_iterator position) 
   {
      size_type p_off = position.get_ptr() - this->m_start;
      if (p_off + 1 != this->m_size){
         //If not the last element, copy left [position, end()) elements
         copy_n(detail::make_move_iterator(this->m_start + (p_off + 1))
               ,this->m_size - (p_off + 1)
               ,this->m_start + p_off);
      }
      --this->m_size;
      //Destroy last element
      base_t::destroy(this->m_start + this->m_size);
      return iterator(position.get_ptr());
   }

   //! <b>Effects</b>: Erases the elements pointed by [first, last).
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the distance between first and last.
   iterator erase(const_iterator first, const_iterator last) 
   {
      //Overwrite [last, end()) elements to first
      size_type f_off = first.get_ptr() - this->m_start;
      size_type l_off = last.get_ptr()  - this->m_start;
      size_type n     = l_off - f_off;
      size_type rem   = this->m_size - l_off;
      size_type to_destroy  =  this->m_size - (f_off + rem);
      size_type destroy_off =  f_off + rem;

      //Overwrite [last, end()) elements to first
      copy_n(detail::make_move_iterator(last.get_ptr()), rem, first.get_ptr());
      //Destroy remaining objects
      this->destroy_n(this->m_start + destroy_off, to_destroy);
      this->m_size -= n;
      return iterator(first.get_ptr());
   }

   //! <b>Effects</b>: Inserts or erases elements at the end such that
   //!   the size becomes n. New elements are copy constructed from x.
   //!
   //! <b>Throws</b>: If memory allocation throws, or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to the difference between size() and new_size.
   void resize(size_type new_size, const T& x) 
   {
      pointer finish = this->m_start + this->m_size;
      if (new_size < size()){
         //Destroy last elements
         this->erase(iterator(this->m_start + new_size), this->end());
      }
      else{
         //Insert new elements at the end
         this->insert(iterator(finish), new_size - this->size(), x);
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
      if (new_size < this->size()){
         //Destroy last elements
         this->erase(iterator(this->m_start + new_size), this->end());
      }
      else{
         size_type n = new_size - this->size();
         this->reserve(new_size);
         while(n--){
            T default_constructed;
            if(boost::is_scalar<T>::value){
               //Value initialization
               new(&default_constructed)T();
            }
            this->push_back(move(default_constructed));
         }
      }
   }

   //! <b>Effects</b>: Erases all the elements of the vector.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of elements in the vector.
   void clear() 
   {  this->priv_destroy_all();  }

   /// @cond
   private:

   void priv_destroy_all()
   {
      destroy_n(this->m_start, this->m_size);
      this->m_size = 0;
   }

   template <class FwdIt>
   void priv_range_insert(pointer pos,     FwdIt first, 
                          FwdIt last, std::forward_iterator_tag)
   {
      if (first != last){
         size_type n = std::distance(first, last);
         //Check if we have enough memory or try to expand current memory
         size_type remaining = this->m_capacity - this->m_size;
         bool same_buffer_start;
         std::pair<pointer, bool> ret;
         size_type new_cap;

         //Check if we already have room
         if (n <= remaining){
            same_buffer_start = true;
         }
         else{
            //There is not enough memory, allocate a new
            //buffer or expand the old one.
            new_cap = this->next_capacity(n);
            ret = this->allocation_command
                  (allocate_new | expand_fwd | expand_bwd,
                   this->m_size + n, new_cap, new_cap, this->m_start);

            //Check for forward expansion
            same_buffer_start = ret.second && this->m_start == ret.first;
            if(same_buffer_start){
               this->m_capacity  = new_cap;
            }
         }
         
         //If we had room or we have expanded forward
         if (same_buffer_start){
            this->priv_range_insert_expand_forward
               (pos, first, last, n);
         }
         //Backwards (and possibly forward) expansion
         else if(ret.second){
            this->priv_range_insert_expand_backwards
               (ret.first, new_cap, pos, first, last, n);
         }
         //New buffer
         else{
            this->priv_range_insert_new_allocation
               (ret.first, new_cap, pos, first, last);
         }
      }
   }

   template <class FwdIt>
   void priv_range_insert_expand_forward
         (pointer pos, FwdIt first, FwdIt last, size_type n)
   {
      typedef detail::move_iterator<pointer> move_it;
      //There is enough memory
      pointer old_finish = this->m_start + this->m_size;
      const size_type elems_after = old_finish - pos;

      if (elems_after > n){
         //New elements can be just copied.
         //Move to uninitialized memory last objects
         n_uninitialized_copy(move_it(old_finish - n),  move_it(old_finish), 
                              old_finish,      static_cast<A&>(*this));
         this->m_size += n;
         //Copy previous to last objects to the initialized end
         std::copy_backward(move_it(pos), move_it(old_finish - n), old_finish);
         //Insert new objects in the pos
         copy_n(first, n, pos);
      }
      else {
         //The new elements don't fit in the [pos, end()) range. Copy
         //to the beginning of the unallocated zone the last new elements.
         FwdIt mid = first;
         std::advance(mid, elems_after);
         n_uninitialized_copy(mid, last, old_finish, static_cast<A&>(*this));
         this->m_size += n - elems_after;
         //Copy old [pos, end()) elements to the uninitialized memory
         n_uninitialized_copy(move_it(pos),  move_it(old_finish), 
                              this->m_start + this->m_size, static_cast<A&>(*this));
         this->m_size += elems_after;
         //Copy first new elements in pos
         std::copy(first, mid, pos);
      }
   }

   template <class FwdIt>
   void priv_range_insert_new_allocation
      (pointer new_start, size_type new_cap, pointer pos, FwdIt first, FwdIt last)
   {
      typedef detail::move_iterator<pointer> move_it;
      //Anti-exception rollback
      scoped_ptr<T, dealloc_t> scoped_alloc(new_start, dealloc_t(*this, new_cap));
      pointer new_finish = new_start;
      BOOST_TRY{
         //Initialize with [begin(), pos) old buffer 
         //the start of the new buffer
         new_finish += n_uninitialized_copy
                        (move_it(this->m_start), move_it(pos), new_start, static_cast<A&>(*this));
         //Initialize new objects, starting from previous point
         new_finish += n_uninitialized_copy
                        (first, last, new_finish, static_cast<A&>(*this));
         //Initialize from the rest of the old buffer, 
         //starting from previous point
         new_finish += n_uninitialized_copy
            (move_it(pos), move_it(this->m_start + this->m_size), new_finish, static_cast<A&>(*this));
      }
      BOOST_CATCH(...){
         this->destroy_n(new_start, new_finish - new_start);
         BOOST_RETHROW
      }
      BOOST_CATCH_END
      scoped_alloc.release();
      //Destroy and deallocate old elements except if we have
      //moved them and they have has_trivial_destructor_after_move
      if(!base_t::trivial_dctr_after_move)
         this->priv_destroy_and_deallocate();
      this->m_start     = new_start;
      this->m_size      = new_finish - new_start;
      this->m_capacity  = new_cap;
   }

   template <class FwdIt>
   void priv_range_insert_expand_backwards
         (pointer new_start, size_type new_capacity,
          pointer pos, FwdIt first, FwdIt last, size_type n)
   {
      typedef detail::scoped_destructor_n
         <allocator_type> ValueArrayDestructor;
      typedef detail::move_iterator<pointer> move_it;
      //Backup old data
      pointer old_start  = this->m_start;
      pointer old_finish = this->m_start + this->m_size;
      size_type old_size = this->m_size;

      //We can have 8 possibilities:
      const size_type elemsbefore   = (size_type)(pos - this->m_start);
      const size_type s_before      = (size_type)(old_start - new_start);

      //Update the vector buffer information to a safe state
      this->m_start      = new_start;
      this->m_capacity   = new_capacity;
      this->m_size = 0;

      //If anything goes wrong, this object will destroy
      //all the old objects to fulfill previous vector state
      scoped_ptr<value_type, ValueArrayDestructor>
         old_values_destroyer
            (old_start, ValueArrayDestructor(static_cast<A&>(*this),old_size));

      //Check if s_before is so big that even copying the old data + new data
      //there is a gap between the new data and the old data
      if(s_before >= (old_size + n)){
         //Old situation:
         // _________________________________________________________
         //|            raw_mem                | old_begin | old_end |
         //| __________________________________|___________|_________|
         //
         //New situation:
         // _________________________________________________________
         //| old_begin |    new   | old_end |         raw_mem        |
         //|___________|__________|_________|________________________|
         //
         //Copy first old values before pos, after that the 
         //new objects
         uninitialized_copy_copy
            (move_it(old_start), move_it(pos), first, last, new_start, static_cast<A&>(*this));
         scoped_ptr<value_type, ValueArrayDestructor>
            new_values_destroyer(new_start
                                ,ValueArrayDestructor(static_cast<A&>(*this)
                                                     ,elemsbefore));
         //Now initialize the rest of memory with the last old values
         uninitialized_copy(move_it(pos), move_it(old_finish)
                           ,new_start + elemsbefore + n, static_cast<A&>(*this));
         //All new elements correctly constructed, avoid new element destruction
         new_values_destroyer.release();
         this->m_size = old_size + n;
         //Old values destroyed automatically with "old_values_destroyer"
         //when "old_values_destroyer" goes out of scope unless the have trivial
         //destructor after move.
         if(base_t::trivial_dctr_after_move)
            old_values_destroyer.release();
      }
      //Check if s_before is so big that divides old_end
      else if(difference_type(s_before) >= difference_type(elemsbefore + n)){
         //Old situation:
         // __________________________________________________
         //|            raw_mem         | old_begin | old_end |
         //| ___________________________|___________|_________|
         //
         //New situation:
         // __________________________________________________
         //| old_begin |   new    | old_end |  raw_mem        |
         //|___________|__________|_________|_________________|
         //
         //Copy first old values before pos, after that the 
         //new objects
         uninitialized_copy_copy
            (move_it(old_start), move_it(pos), first, last, new_start, static_cast<A&>(*this));
         scoped_ptr<value_type, ValueArrayDestructor>
            new_values_destroyer(new_start
                              ,ValueArrayDestructor(static_cast<A&>(*this)
                                                   ,elemsbefore));
         size_type raw_gap = s_before - (elemsbefore + n);
         //Now initialize the rest of s_before memory with the
         //first of elements after new values
         uninitialized_copy(move_it(pos), move_it(pos + raw_gap)
                           ,new_start + elemsbefore + n, static_cast<A&>(*this));
         //All new elements correctly constructed, avoid new element destruction
         new_values_destroyer.release();
         //All new elements correctly constructed, avoid old element destruction
         old_values_destroyer.release();
         //Update size since we have a contiguous buffer
         this->m_size = old_size + s_before;
         //Now copy remaining last objects in the old buffer begin
         pointer to_destroy =
            std::copy(move_it(pos + raw_gap), move_it(old_finish), old_start);
         //Now destroy redundant elements except if they were moved and
         //they have trivial destructor after move
         size_type n_destroy =  old_finish - to_destroy;
         if(!base_t::trivial_dctr_after_move)
            this->destroy_n(to_destroy, n_destroy);
         this->m_size -= n_destroy;
      }
      else{
         //Check if we have to do the insertion in two phases
         //since maybe s_before is not big enough and
         //the buffer was expanded both sides
         //
         //Old situation:
         // _________________________________________________
         //| raw_mem | old_begin + old_end |  raw_mem        |
         //|_________|_____________________|_________________|
         //
         //New situation with do_after:
         // _________________________________________________
         //|     old_begin + new + old_end     |  raw_mem    |
         //|___________________________________|_____________|
         //
         //New without do_after:
         // _________________________________________________
         //| old_begin + new + old_end  |  raw_mem           |
         //|____________________________|____________________|
         //
         bool do_after    = n > s_before;
         FwdIt before_end = first;
         //If we have to expand both sides,
         //we will play if the first new values so
         //calculate the upper bound of new values
         if(do_after){
            std::advance(before_end, s_before);
         }

         //Now we can have two situations: the raw_mem of the
         //beginning divides the old_begin, or the new elements:
         if (s_before <= elemsbefore) {
            //The raw memory divides the old_begin group:
            //
            //If we need two phase construction (do_after)
            //new group is divided in new = new_beg + new_end groups
            //In this phase only new_beg will be inserted
            //
            //Old situation:
            // _________________________________________________
            //| raw_mem | old_begin | old_end |  raw_mem        |
            //|_________|___________|_________|_________________|
            //
            //New situation with do_after(1):
            //This is not definitive situation, the second phase
            //will include
            // _________________________________________________
            //| old_begin | new_beg | old_end |  raw_mem        |
            //|___________|_________|_________|_________________|
            //
            //New situation without do_after:
            // _________________________________________________
            //| old_begin | new | old_end |  raw_mem            |
            //|___________|_____|_________|_____________________|
            //
            //Copy the first part of old_begin to raw_mem
            pointer start_n = old_start + difference_type(s_before); 
            uninitialized_copy(move_it(old_start), move_it(start_n), new_start, static_cast<A&>(*this));
            //The buffer is all constructed until old_end,
            //release destroyer and update size
            old_values_destroyer.release();
            this->m_size = old_size + s_before;
            //Now copy the second part of old_begin overwriting himself
            pointer next = std::copy(move_it(start_n), move_it(pos), old_start);
            if(do_after){
               //Now copy the new_beg elements
               std::copy(first, before_end, next);
            }
            else{
               //Now copy the all the new elements
               pointer move_start = std::copy(first, last, next);
               //Now displace old_end elements
               pointer move_end   = std::copy(move_it(pos), move_it(old_finish), move_start);
               //Destroy remaining moved elements from old_end except if
               //they have trivial destructor after being moved
               difference_type n_destroy = s_before - n;
               if(!base_t::trivial_dctr_after_move)
                  this->destroy_n(move_end, n_destroy);
               this->m_size -= n_destroy;
            }
         }
         else {
            //The raw memory divides the new elements
            //
            //If we need two phase construction (do_after)
            //new group is divided in new = new_beg + new_end groups
            //In this phase only new_beg will be inserted
            //
            //Old situation:
            // _______________________________________________________
            //|   raw_mem     | old_begin | old_end |  raw_mem        |
            //|_______________|___________|_________|_________________|
            //
            //New situation with do_after():
            // ____________________________________________________
            //| old_begin |    new_beg    | old_end |  raw_mem     |
            //|___________|_______________|_________|______________|
            //
            //New situation without do_after:
            // ______________________________________________________
            //| old_begin | new | old_end |  raw_mem                 |
            //|___________|_____|_________|__________________________|
            //
            //First copy whole old_begin and part of new to raw_mem
            FwdIt mid = first;
            size_type n_new_init = difference_type(s_before) - elemsbefore;
            std::advance(mid, n_new_init);
            uninitialized_copy_copy
               (move_it(old_start), move_it(pos), first, mid, new_start, static_cast<A&>(*this));
            //The buffer is all constructed until old_end,
            //release destroyer and update size
            old_values_destroyer.release();
            this->m_size = old_size + s_before;

            if(do_after){
               //Copy new_beg part
               std::copy(mid, before_end, old_start);
            }
            else{
               //Copy all new elements
               pointer move_start = std::copy(mid, last, old_start);
               //Displace old_end
               pointer move_end   = std::copy(move_it(pos), move_it(old_finish), move_start);
               //Destroy remaining moved elements from old_end except if they
               //have trivial destructor after being moved
               difference_type n_destroy = s_before - n;
               if(!base_t::trivial_dctr_after_move)
                  this->destroy_n(move_end, n_destroy);
               this->m_size -= n_destroy;
            }
         }

         //This is only executed if two phase construction is needed
         //This can be executed without exception handling since we
         //have to just copy and append in raw memory and
         //old_values_destroyer has been released in phase 1.
         if(do_after){
            //The raw memory divides the new elements
            //
            //Old situation:
            // ______________________________________________________
            //|   raw_mem    | old_begin |  old_end   |  raw_mem     |
            //|______________|___________|____________|______________|
            //
            //New situation with do_after(1):
            // _______________________________________________________
            //| old_begin   +   new_beg  | new_end |old_end | raw_mem |
            //|__________________________|_________|________|_________|
            //
            //New situation with do_after(2):
            // ______________________________________________________
            //| old_begin      +       new            | old_end |raw |
            //|_______________________________________|_________|____|
            //
            const size_type n_after  = n - s_before;
            const difference_type elemsafter = old_size - elemsbefore;

            //The new_end part is [first + (n - n_after), last)
            std::advance(first, n - n_after);

            //We can have two situations:
            if (elemsafter > difference_type(n_after)){
               //The raw_mem from end will divide displaced old_end
               //
               //Old situation:
               // ______________________________________________________
               //|   raw_mem    | old_begin |  old_end   |  raw_mem     |
               //|______________|___________|____________|______________|
               //
               //New situation with do_after(1):
               // _______________________________________________________
               //| old_begin   +   new_beg  | new_end |old_end | raw_mem |
               //|__________________________|_________|________|_________|
               //
               //First copy the part of old_end raw_mem
               pointer finish_n = old_finish - difference_type(n_after);
               uninitialized_copy(move_it(finish_n), move_it(old_finish)
                                 ,old_finish, static_cast<A&>(*this));
               this->m_size += n_after;
               //Displace the rest of old_end to the new position
               std::copy_backward(move_it(pos), move_it(finish_n), old_finish);
               //Now overwrite with new_end
               std::copy(first, last, pos);
            }
            else {
               //The raw_mem from end will divide new_end part
               //
               //Old situation:
               // _____________________________________________________________
               //|   raw_mem    | old_begin |  old_end   |  raw_mem            |
               //|______________|___________|____________|_____________________|
               //
               //New situation with do_after(2):
               // _____________________________________________________________
               //| old_begin   +   new_beg  |     new_end   |old_end | raw_mem |
               //|__________________________|_______________|________|_________|
               //
               FwdIt mid = first;
               std::advance(mid, elemsafter);
               //First initialize data in raw memory
               uninitialized_copy_copy
                  (mid, last, move_it(pos), move_it(old_finish),
                   old_finish, static_cast<A&>(*this));
               this->m_size += n_after;
               //Now copy the part of new_end over constructed elements
               std::copy(first, mid, pos);
            }
         }
      }
   }

   template <class InIt>
   void priv_range_insert(iterator pos,
                          InIt first, InIt last,
                          std::input_iterator_tag)
   {
      //Insert range before the pos position
      std::copy(std::inserter(*this, pos), first, last);
   }

   template <class InIt>
   void priv_assign_aux(InIt first, InIt last,
                        std::input_iterator_tag)
   {
      //Overwrite all elements we can from [first, last)
      iterator cur = begin();
      for ( ; first != last && cur != end(); ++cur, ++first){
         *cur = *first;
      }
      
      if (first == last){
         //There are no more elements in the sequence, erase remaining
         this->erase(cur, end());
      }
      else{
         //There are more elements in the range, insert the remaining ones
         this->insert(this->end(), first, last);
      }
   }

   template <class FwdIt>
   void priv_assign_aux(FwdIt first, FwdIt last,
                        std::forward_iterator_tag)
   {
      size_type n = std::distance(first, last);
      //Check if we have enough memory or try to expand current memory
      size_type remaining = this->m_capacity - this->m_size;
      bool same_buffer_start;
      std::pair<pointer, bool> ret;
      size_type new_cap;

      if (n <= remaining){
         same_buffer_start = true;
      }
      else{
         //There is not enough memory, allocate a new buffer
         new_cap = this->next_capacity(n);
         ret = this->allocation_command
               (allocate_new | expand_fwd | expand_bwd,
                  this->size() + n, new_cap, new_cap, this->m_start);
         same_buffer_start = ret.second && this->m_start == ret.first;
         if(same_buffer_start){
            this->m_capacity  = new_cap;
         }
      }
      
      if(same_buffer_start){
         if (this->size() >= n){
            //There is memory, but there are more old elements than new ones
            //Overwrite old elements with new ones
            copy_n(first, n, this->m_start);
            //Destroy remaining old elements
            this->destroy_n(this->m_start + n, this->m_size - n);
            this->m_size = n;
         }
         else{
            //There is memory, but there are less old elements than new ones
            //Overwrite old elements with new ones
            FwdIt mid = first;
            std::advance(mid, this->size());
            std::copy(first, mid, this->m_start);
            //Initialize the remaining new elements in the uninitialized memory
            n_uninitialized_copy(mid, last, this->m_start + this->m_size, static_cast<A&>(*this));
            this->m_size = n;
         }
      }
      else if(!ret.second){
         scoped_ptr<T, dealloc_t> scoped_alloc(ret.first, dealloc_t(*this, new_cap));
         n_uninitialized_copy(first, last, ret.first, static_cast<A&>(*this));
         scoped_alloc.release();
         //Destroy and deallocate old buffer
         this->priv_destroy_and_deallocate();
         this->m_start     = ret.first;
         this->m_size      = n;
         this->m_capacity  = new_cap;
      }
      else{
         //Backwards expansion
         //If anything goes wrong, this object will destroy
         //all old objects
         typedef detail::scoped_destructor_n
            <allocator_type> ValueArrayDestructor;
         pointer old_start    = this->m_start;
         size_type old_size   = this->m_size;
         scoped_ptr<value_type, ValueArrayDestructor>
            old_values_destroyer(old_start
                                 ,ValueArrayDestructor(static_cast<A&>(*this)
                                                       ,old_size));
         //If something goes wrong size will be 0
         //but holding the whole buffer
         this->m_size  = 0;
         this->m_start = ret.first;
         this->m_capacity = new_cap;
         
         //Backup old buffer data
         size_type old_offset    = old_start - ret.first;
         size_type first_count   = min_value(n, old_offset);
         FwdIt mid = n_uninitialized_copy_n
            (first, first_count, ret.first, static_cast<A&>(*this));

         if(old_offset > n){
            //All old elements will be destroyed by "old_values_destroyer" 
            this->m_size = n;
         }
         else{
            //We have constructed objects from the new begin until
            //the old end so release the rollback destruction
            old_values_destroyer.release();
            this->m_start  = ret.first;
            this->m_size   = first_count + old_size;
            //Now overwrite the old values
            size_type second_count = min_value(old_size, n - first_count);
            mid = copy_n(mid, second_count, old_start);
            
            //Check if we still have to append elements in the
            //uninitialized end
            if(second_count == old_size){
               n_uninitialized_copy_n(mid, n - first_count - second_count
                                     , old_start + old_size, static_cast<A&>(*this)); 
            }
            else{
               //We have to destroy some old values
               this->destroy_n
                  (old_start + second_count, old_size - second_count);
               this->m_size = n;
            }
            this->m_size = n;                        
         }
      }
   }

   template <class Integer>
   void priv_assign_dispatch(Integer n, Integer val, boost::mpl::true_)
   { this->assign((size_type) n, (T) val); }

   template <class InIt>
   void priv_assign_dispatch(InIt first, InIt last, 
                             boost::mpl::false_)
   { 
      //Dispatch depending on integer/iterator
      typedef typename 
         std::iterator_traits<InIt>::iterator_category ItCat;
      this->priv_assign_aux(first, last, ItCat()); 
   }

   template <class Integer>
   void priv_insert_dispatch( iterator pos, Integer n, 
                              Integer val, boost::mpl::true_) 
   {  this->insert(pos, (size_type)n, (T)val);  }

   template <class InIt>
   void priv_insert_dispatch(iterator pos,   InIt first, 
                             InIt last,      boost::mpl::false_)
   {
      //Dispatch depending on integer/iterator
      typedef typename 
         std::iterator_traits<InIt>::iterator_category ItCat;
      this->priv_range_insert(pos.get_ptr(), first, last, ItCat());
   }

   template <class Integer>
   void priv_initialize_aux(Integer n, Integer value, boost::mpl::true_) 
   {
      this->priv_range_initialize(cvalue_iterator(value, n),
                                  cvalue_iterator(),
                                  cvalue_iterator::iterator_category());
   }

   template <class InIt>
   void priv_initialize_aux(InIt first, InIt last,
                            boost::mpl::false_) 
   {
      //Dispatch depending on integer/iterator
      typedef typename 
         std::iterator_traits<InIt>::iterator_category ItCat;
      this->priv_range_initialize(first, last, ItCat());
   }

   void priv_destroy_and_deallocate()
   {
      //If there is allocated memory, destroy and deallocate
      if(this->m_start != 0){
         this->destroy_n(this->m_start, this->m_size); 
         this->deallocate(this->m_start, this->m_capacity);
      }
   }

   template <class FwdIt>
   pointer priv_reserve_and_copy(size_type n, size_type &cap
                                 ,FwdIt first, FwdIt last)
   {
      //Allocate n element buffer and initialize from range
      pointer result = this->allocation_command(allocate_new, n, n, cap).first;
      scoped_ptr<T, dealloc_t> scoped_alloc(result, dealloc_t(*this, cap));
      n_uninitialized_copy(first, last, result, static_cast<A&>(*this));
      scoped_alloc.release();
      return result;
   }

   void priv_check_range(size_type n) const 
   {
      //If n is out of range, throw an out_of_range exception
      if (n >= size())
         throw std::out_of_range("vector::at");
   }
   /// @endcond
};

template <class T, class A>
inline bool 
operator==(const vector<T, A>& x, const vector<T, A>& y)
{
   //Check first size and each element if needed
   return x.size() == y.size() &&
          std::equal(x.begin(), x.end(), y.begin());
}

template <class T, class A>
inline bool 
operator!=(const vector<T, A>& x, const vector<T, A>& y)
{
   //Check first size and each element if needed
  return x.size() != y.size() ||
         !std::equal(x.begin(), x.end(), y.begin());
}

template <class T, class A>
inline bool 
operator<(const vector<T, A>& x, const vector<T, A>& y)
{
   return std::lexicographical_compare(x.begin(), x.end(), 
                                       y.begin(), y.end());
}

template <class T, class A>
inline void swap(vector<T, A>& x, vector<T, A>& y)
{  x.swap(y);  }

template <class T, class A>
inline void swap(const detail::moved_object<vector<T, A> >& x, vector<T, A>& y)
{  x.get().swap(y);  }

template <class T, class A>
inline void swap(vector<T, A> &x, const detail::moved_object<vector<T, A> >& y)
{  x.swap(y.get());  }

/// @cond
/*!This class is movable*/
template <class T, class A>
struct is_movable<vector<T, A> >
{
   enum {   value = true };
};

//!has_trivial_destructor_after_move<> == true_type
//!specialization for optimizations
template <class T, class A>
struct has_trivial_destructor_after_move<vector<T, A> >
{
   enum {   value = has_trivial_destructor<A>::value  };
};
/// @endcond

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif //   #ifndef  BOOST_INTERPROCESS_VECTOR_HPP

