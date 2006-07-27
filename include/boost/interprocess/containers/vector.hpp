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
// See http://www.boost.org/libs/interprocess/ for documentation.
//
//////////////////////////////////////////////////////////////////////////////
//
// This file comes from SGI's stl_vector.h file. Modified by Ion Gaztañaga.
// Renaming, isolating and porting to generic algorithms. Pointer typedef 
// set to allocator::pointer to allow placing it in shared memory.
//
///////////////////////////////////////////////////////////////////////////////

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


namespace boost {

namespace interprocess {

namespace detail {

/*!This struct deallocates and allocated memory */
template <class A>
struct vector_alloc_holder : public A
{
   typedef typename A::pointer   pointer;
   typedef typename A::size_type size_type;

   //Constructor, does not throw
   vector_alloc_holder(const A &a)
      : A(a), m_start(0), m_size(0), m_capacity(0)
   {}

   //Destructor
   ~vector_alloc_holder()
   {  this->priv_deallocate(); }

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

   private:
   void priv_deallocate()
   {
      if(!m_start)   return;
      this->deallocate(m_start, m_capacity);
      m_capacity     = 0;
   }
};

}  //namespace detail {


//vector class
template <class T, class A>
class vector : private detail::vector_alloc_holder<A>
{
   typedef vector<T, A>                   self_t;
   typedef detail::vector_alloc_holder<A> base_t;
   typedef detail::scoped_array_deallocator<A> dealloc_t;

   public:
   //STL container typedefs
   typedef T                              value_type;
   typedef A                              allocator_type;
   typedef typename A::pointer            pointer;
   typedef typename A::const_pointer      const_pointer;
   typedef typename A::reference          reference;
   typedef typename A::const_reference    const_reference;
   typedef typename A::size_type          size_type;
   typedef typename A::difference_type    difference_type;
   //This shouldn't be needed but VC6.0 needs this
   typedef typename A::pointer            vec_ptr;
   typedef typename A::const_pointer      vec_cptr;
   typedef typename A::reference          vec_ref;
   typedef typename A::const_reference    vec_cref;
   typedef typename A::difference_type    vec_diff;

   private:
   typedef constant_iterator<T, difference_type> cvalue_iterator;

   public:
   //Vector const_iterator
    class const_iterator
      : public boost::iterator<std::random_access_iterator_tag
                              ,value_type
                              ,vec_diff
                              ,vec_cptr
                              ,vec_cref>
    {
      private:
      vec_ptr get_ptr() const    {  return   m_ptr;  }

      protected:
      vec_ptr m_ptr;
        explicit const_iterator(vec_ptr ptr)  : m_ptr(ptr){}

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

   //Vector iterator
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

   //Reverse iterators
   typedef typename boost::reverse_iterator<iterator>   
      reverse_iterator;
    typedef typename boost::reverse_iterator<const_iterator>                 
      const_reverse_iterator;

   public:
   //Constructor
   explicit vector(const A& a = A())
      : base_t(a)
   {}

   //Construct and fill with n equal objects
   vector(size_type n, const T& value = T(),
          const allocator_type& a = allocator_type()) 
      :  base_t(a)
   {  this->assign(n, value); }

   //Copy constructor
   vector(const vector<T, A>& x) 
      :  base_t(x.get_allocator())
   {  *this = x;  }

   //Construct from iterator range
   template <class InIt>
   vector(InIt first, InIt last, const allocator_type& a = allocator_type())
      :  base_t(a)
   {  this->assign(first, last); }

   //Destructor
   ~vector() 
   {  this->priv_destroy_all();  }

   //Functions: begin, rbegin, end, rend
   iterator begin()      
   { return iterator(this->m_start); }

   iterator end()        
   { return iterator(this->m_start + this->m_size); }

   const_iterator begin() const
   { return const_iterator(this->m_start); }

   const_iterator end()   const
   { return const_iterator(this->m_start + this->m_size); }

   reverse_iterator rbegin()     
   { return reverse_iterator(this->end());      }

   reverse_iterator rend()       
   { return reverse_iterator(this->begin());       }

   const_reverse_iterator rbegin()const
   { return const_reverse_iterator(this->end());}

   const_reverse_iterator rend()  const
   { return const_reverse_iterator(this->begin()); }

   //Functions: front, back
   reference         front()       
   { return *this->m_start; }

   const_reference   front() const 
   { return *this->m_start; }

   reference         back()        
   { return this->m_start[this->m_size - 1]; }

   const_reference   back()  const 
   { return this->m_start[this->m_size - 1]; }

   //Functions: size, max_size, capacity, empty
   size_type size() const 
   { return this->m_size; }

   size_type max_size() const 
   { return allocator_type::max_size(); }

   size_type capacity() const 
   { return this->m_capacity; }

   bool empty() const 
   { return !this->m_size; }

   //Functions: operator [], at
   reference       operator[](size_type n)         
   { return this->m_start[n]; }

   const_reference operator[](size_type n) const   
   { return this->m_start[n]; }

   reference at(size_type n)
   { this->priv_check_range(n); return this->m_start[n]; }

   const_reference at(size_type n) const
   { this->priv_check_range(n); return this->m_start[n]; }

   //Functions: get_allocator, reserve
   allocator_type get_allocator() const 
   { return allocator_type(*this);  }

   void reserve(size_type n) 
   {
      if (this->capacity() < n){
         //Allocate new size, copy data and free old buffer
         const size_type old_size = this->size();
         size_type new_cap;
         pointer tmp = this->priv_reserve_and_copy
            (n, new_cap, this->m_start, this->m_start + this->m_size);
         this->priv_destroy_and_deallocate();
         this->m_start     = tmp;
         this->m_size      = old_size;
         this->m_capacity  = new_cap;
      }
   }

   //Assignment
   vector<T, A>& operator=(const vector<T, A>& x)
   {
      if (&x != this){
         this->assign(x.m_start, x.m_start + x.m_size);
      }
      return *this;
   }

   void assign(size_type n, const value_type& val = T())
   {  this->assign(cvalue_iterator(val, n), cvalue_iterator());   }

   template <class InIt>
   void assign(InIt first, InIt last) 
   {
      //Dispatch depending on integer/iterator
      const bool aux_boolean = boost::is_integral<InIt>::value;
      typedef boost::mpl::bool_<aux_boolean> Result;
      this->priv_assign_dispatch(first, last, Result());
   }

   void push_back(const T& x = T()) 
   {
      if (this->m_size < this->m_capacity){
         //There is more memory, just construct a new object at the end
         this->construct(this->m_start + this->m_size, x);
         ++this->m_size;
      }

      this->insert(this->end(), static_cast<size_type>(1), x);
   }
   
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

   iterator insert(iterator position, const T& x = T()) 
   {
      //Just call more general insert(pos, size, value) and return iterator
      size_type n = position - begin();
      this->insert(position, (size_type)1, x);
      return iterator(this->m_start + n);
   }

   template <class InIt>
   void insert(iterator pos, InIt first, InIt last)
   {
      //Dispatch depending on integer/iterator
      const bool aux_boolean = boost::is_integral<InIt>::value;
      typedef boost::mpl::bool_<aux_boolean> Result;
      this->priv_insert_dispatch(pos, first, last, Result());
   }

   void insert (iterator p, size_type n, const T& x)
   {  this->insert(p, cvalue_iterator(x, n), cvalue_iterator()); }

   void pop_back() 
   {
      //Destroy last element
      --this->m_size;
      this->destroy(this->m_start + this->m_size);
   }

   // Return type of erase(const_iterator):
   // http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2005/n1753.html#130

   iterator erase(const_iterator position) 
   {
      size_type p_off = position.get_ptr() - this->m_start;
      if (p_off + 1 != this->m_size){
         //If not the last element, copy left [position, end()) elements
         copy_n(this->m_start + (p_off + 1), this->m_size - (p_off + 1), this->m_start + p_off);
      }
      --this->m_size;
      //Destroy last element
      this->destroy(this->m_start + this->m_size);
      return iterator(position.get_ptr());
   }

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
      copy_n(last.get_ptr(), rem, first.get_ptr());
      //Destroy remaining objects
      this->priv_destroy_n(this->m_start + destroy_off, to_destroy);
      this->m_size -= n;
      return iterator(first.get_ptr());
   }

   void resize(size_type new_size, const T& x = T()) 
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

   void clear() 
   {  this->priv_destroy_all();  }

   private:

   void priv_destroy_all()
   {
      if(!this->m_start)
         return;

      pointer start  = this->m_start;
      for(; this->m_size--; ++start) 
         this->destroy(start);

      this->m_size = 0;
   }

   template <class FwdIt>
   void priv_range_insert(const pointer &pos,     FwdIt first, 
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
         if (n < remaining){
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
         (const pointer &pos, FwdIt first, FwdIt last, size_type n)
   {
      //There is enough memory
      pointer old_finish = this->m_start + this->m_size;
      const size_type elems_after = old_finish - pos;

      if (elems_after > n){
         //New elements can be just copied.
         //Move to uninitialized memory last objects
         n_uninitialized_copy(old_finish - n,  old_finish, 
                              old_finish,      static_cast<A&>(*this));
         this->m_size += n;
         //Copy previous to last objects to the initialized end
         std::copy_backward(pos, old_finish - n, old_finish);
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
         n_uninitialized_copy(pos,  old_finish, 
                              this->m_start + this->m_size, static_cast<A&>(*this));
         this->m_size += elems_after;
         //Copy first new elements in pos
         std::copy(first, mid, pos);
      }
   }

   template <class FwdIt>
   void priv_range_insert_new_allocation
      (pointer new_start, size_type new_cap, const pointer &pos, FwdIt first, FwdIt last)
   {
      //Anti-exception rollback
      scoped_ptr<T, dealloc_t> scoped_alloc(new_start, dealloc_t(*this, new_cap));
      pointer new_finish = new_start;
      BOOST_TRY{
         //Initialize with [begin(), pos) old buffer 
         //the start of the new buffer
         new_finish += n_uninitialized_copy
                        (this->m_start, pos, new_start, static_cast<A&>(*this));
         //Initialize new objects, starting from previous point
         new_finish += n_uninitialized_copy
                        (first, last, new_finish, static_cast<A&>(*this));
         //Initialize from the rest of the old buffer, 
         //starting from previous point
         new_finish += n_uninitialized_copy
            (pos, this->m_start + this->m_size, new_finish, static_cast<A&>(*this));
      }
      BOOST_CATCH(...){
         this->priv_destroy_n(new_start, new_finish - new_start);
         BOOST_RETHROW
      }
      BOOST_CATCH_END
      scoped_alloc.release();
      //Destroy and deallocate old elements
      this->priv_destroy_and_deallocate();
      this->m_start     = new_start;
      this->m_size      = new_finish - new_start;
      this->m_capacity  = new_cap;
   }

   template <class FwdIt>
   void priv_range_insert_expand_backwards
         (pointer new_start, size_type new_capacity,
          const pointer &pos, FwdIt first, FwdIt last, size_type n)
   {
      typedef detail::scoped_destructor_n
         <allocator_type> ValueArrayDestructor;

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
            (old_start, pos, first, last, new_start, static_cast<A&>(*this));
         scoped_ptr<value_type, ValueArrayDestructor>
            new_values_destroyer(new_start
                                ,ValueArrayDestructor(static_cast<A&>(*this)
                                                     ,elemsbefore));
         //Now initialize the rest of memory with the last old values
         uninitialized_copy(pos, old_finish
                           ,new_start + elemsbefore + n, static_cast<A&>(*this));
         //All new elements correctly constructed, avoid new element destruction
         new_values_destroyer.release();
         //Old values destroyed automatically with "old_values_destroyer"
         //when "old_values_destroyer" goes out of scope.
         this->m_size = old_size + n;
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
            (old_start, pos, first, last, new_start, static_cast<A&>(*this));
         scoped_ptr<value_type, ValueArrayDestructor>
            new_values_destroyer(new_start
                              ,ValueArrayDestructor(static_cast<A&>(*this)
                                                   ,elemsbefore));
         size_type raw_gap = s_before - (elemsbefore + n);
         //Now initialize the rest of s_before memory with the
         //first of elements after new values
         uninitialized_copy(pos, pos + raw_gap
                           ,new_start + elemsbefore + n, static_cast<A&>(*this));
         //All new elements correctly constructed, avoid new element destruction
         new_values_destroyer.release();
         //All new elements correctly constructed, avoid old element destruction
         old_values_destroyer.release();
         //Update size since we have a contiguous buffer
         this->m_size = old_size + s_before;
         //Now copy remaining last objects in the old buffer begin
         pointer to_destroy = std::copy(pos + raw_gap, old_finish, old_start);
         //Now destroy redundant elements
         size_type n_destroy =  old_finish - to_destroy;
         this->priv_destroy_n(to_destroy, n_destroy);
         this->m_size -= n_destroy;
      }
      else{
         //Check if we have to do the insertion in two phases
         //since maybe s_before is not enough and
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
            uninitialized_copy(old_start, start_n, new_start, static_cast<A&>(*this));
            //The buffer is all constructed until old_end,
            //release destroyer and update size
            old_values_destroyer.release();
            this->m_size = old_size + s_before;
            //Now copy the second part of old_begin overwriting himself
            pointer next = std::copy(start_n, pos, old_start);
            if(do_after){
               //Now copy the new_beg elements
               std::copy(first, before_end, next);
            }
            else{
               //Now copy the all the new elements
               pointer move_start = std::copy(first, last, next);
               //Now displace old_end elements
               pointer move_end   = std::copy(pos, old_finish, move_start);
               //Destroy remaining moved elements from old_end
               difference_type n_destroy = s_before - n;
               this->priv_destroy_n(move_end, n_destroy);
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
               (old_start, pos, first, mid, new_start, static_cast<A&>(*this));
            //The buffer is all constructed until old_end,
            //release destroyer and update size
            old_values_destroyer.release();
            this->m_size = old_size + s_before;

            if(do_after){
               //Copy new_beg part
//               std::copy(mid, before_end, old_start);
               std::copy(mid, before_end, old_start);
            }
            else{
               //Copy all new elements
               pointer move_start = std::copy(mid, last, old_start);
               //Displace old_end
               pointer move_end   = std::copy(pos, old_finish, move_start);
               //Destroy remaining moved elements from old_end
               difference_type n_destroy = s_before - n;
               this->priv_destroy_n(move_end, n_destroy);
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
               uninitialized_copy(finish_n, old_finish
                                 ,old_finish, static_cast<A&>(*this));
               this->m_size += n_after;
               //Displace the rest of old_end to the new position
               std::copy_backward(pos, finish_n, old_finish);
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
                  (mid, last, pos, old_finish,
                   old_finish, static_cast<A&>(*this));
               this->m_size += n_after;
               //Now copy the part of new_end over constructed elements
               std::copy(first, mid, pos);
            }
         }
      }
/*
      const difference_type elemsbefore = pos - this->m_start;

      typedef detail::scoped_destructor_n
         <allocator_type> ValueArrayDestructor;

      pointer old_start  = this->m_start;
      pointer old_finish = this->m_start + this->m_size;
      size_type old_size = this->m_size;
      //Update the vector buffer information
      this->m_start      = new_start;
      this->m_capacity   = new_capacity;

      //If anything goes wrong, this object will destroy
      //all old objects
      scoped_ptr<value_type, ValueArrayDestructor>
         old_values_destroyer(old_start
                             ,ValueArrayDestructor(static_cast<A&>(*this)
                                                  ,old_size));
      //If something goes wrong size will be 0
      this->m_size = 0;

      //This is the number of objects expanded backwards
      size_type s_before = (size_type)(old_start - this->m_start);

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
         this->priv_uninitialized_copy_copy
            (old_start, pos, first, last, new_start, static_cast<A&>(*this));
         scoped_ptr<value_type, ValueArrayDestructor>
            new_values_destroyer(new_start
                                ,ValueArrayDestructor(static_cast<A&>(*this)
                                                     ,elemsbefore));
         //Now initialize the rest of memory with the last old values
         uninitialized_copy(pos, old_finish
                           ,new_start + elemsbefore + n, static_cast<A&>(*this));
         //All new elements correctly constructed, avoid new element destruction
         new_values_destroyer.release();
         //Old values destroyed automatically with "old_values_destroyer"
         //when "old_values_destroyer" goes out of scope.
         this->m_size = old_size + n;
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
         this->priv_uninitialized_copy_copy
            (old_start, pos, first, last, new_start, static_cast<A&>(*this));
         scoped_ptr<value_type, ValueArrayDestructor>
            new_values_destroyer(new_start
                              ,ValueArrayDestructor(static_cast<A&>(*this)
                                                   ,elemsbefore));
         size_type raw_gap = s_before - (elemsbefore + n);
         //Now initialize the rest of s_before memory with the
         //first of elements after new values
         uninitialized_copy(pos, pos + raw_gap
                           ,new_start + elemsbefore + n, static_cast<A&>(*this));
         //All new elements correctly constructed, avoid new element destruction
         new_values_destroyer.release();
         //All new elements correctly constructed, avoid old element destruction
         old_values_destroyer.release();
         //Update size since we have a contiguous buffer
         this->m_size = old_size + s_before;
         //Now copy remaining last objects in the old buffer begin
         pointer to_destroy = std::copy(pos + raw_gap, old_finish, old_start);
         //Now destroy redundant elements
         size_type n_destroy =  old_finish - to_destroy;
         this->priv_destroy_n(to_destroy, n_destroy);
         this->m_size -= n_destroy;
      }
      else{
         //Check if we have to do the insertion in two phases
         //since maybe s_before is not enough and
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
         if (difference_type(s_before) <= elemsbefore) {
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
            uninitialized_copy(old_start, start_n, new_start, static_cast<A&>(*this));
            //The buffer is all constructed until old_end,
            //release destroyer and update size
            old_values_destroyer.release();
            this->m_size = old_size + s_before;
            //Now copy the second part of old_begin overwriting himself
            pointer next = std::copy(start_n, pos, old_start);
            if(do_after){
               //Now copy the new_beg elements
               std::copy(first, before_end, next);
            }
            else{
               //Now copy the all the new elements
               pointer move_start = std::copy(first, last, next);
               //Now displace old_end elements
               pointer move_end   = std::copy(pos, old_finish, move_start);
               //Destroy remaining moved elements from old_end
               difference_type n_destroy = s_before - n;
               this->priv_destroy_n(move_end, n_destroy);
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
            uninitialized_copy_n_copy_n
               (old_start, elemsbefore, first, mid, new_start, static_cast<A&>(*this));
            //The buffer is all constructed until old_end,
            //release destroyer and update size
            old_values_destroyer.release();
            this->m_size = old_size + s_before;

            if(do_after){
               //Copy new_beg part
//               std::copy(mid, before_end, old_start);
               std::copy(mid, before_end, old_start);
            }
            else{
               //Copy all new elements
               pointer move_start = std::copy(mid, last, old_start);
               //Displace old_end
               pointer move_end   = std::copy(pos, old_finish, move_start);
               //Destroy remaining moved elements from old_end
               difference_type n_destroy = s_before - n;
               this->priv_destroy_n(move_end, n_destroy);
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
               uninitialized_copy(finish_n, old_finish
                                 ,old_finish, static_cast<A&>(*this));
               this->m_size += n_after;
               //Displace the rest of old_end to the new position
               std::copy_backward(pos, finish_n, old_finish);
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
               this->priv_uninitialized_copy_copy
                  (mid, last, pos, old_finish,
                   old_finish, static_cast<A&>(*this));
               this->m_size += n_after;
               //Now copy the part of new_end over constructed elements
               std::copy(first, mid, pos);
            }
         }
      }*/
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

      if (n < remaining){
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
            this->priv_destroy_n(this->m_start + n, this->m_size - n);
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
               this->priv_destroy_n
                  (old_start + second_count, old_size - second_count);
               this->m_size = n;
            }
            this->m_size = n;                        
         }
      }
/*
      size_type len = std::distance(first, last);
      //Check if we have enough memory or try to expand current memory
      if (this->m_capacity >= len || this->expand_by(len - this->m_capacity)){

      }
      else {
         //There is no enough memory, allocate and copy new buffer
         size_type new_cap;
         pointer tmp = this->priv_allocate_and_copy(len, new_cap, first, last);
         //Destroy and deallocate old buffer
         this->priv_destroy_and_deallocate();
         this->m_start = tmp;
         this->m_size = len;
         this->m_capacity = new_cap;
      }*/
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

   void priv_destroy_n(pointer p, size_type n)
   {  for(; n--; ++p) this->destroy(p); }

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
         this->priv_destroy_n(this->m_start, this->m_size); 
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

/*
   template <class FwdIt>
   pointer priv_allocate_and_copy(size_type n, size_type &cap
                                 ,FwdIt first, FwdIt last)
   {
      //Allocate n element buffer and initialize from range
      pointer result = this->allocation_command(allocate_new, n, n, cap).first;
      scoped_ptr<T, dealloc_t> scoped_alloc(result, dealloc_t(*this, cap));
      n_uninitialized_copy(first, last, result, static_cast<A&>(*this));
      scoped_alloc.release();
      return result;
   }
*/
   void priv_check_range(size_type n) const 
   {
      //If n is out of range, throw an out_of_range exception
      if (n >= size())
         throw std::out_of_range("vector::at");
   }
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
{
   x.swap(y);
}


}  //namespace interprocess {

}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif //   #ifndef  BOOST_INTERPROCESS_VECTOR_HPP

