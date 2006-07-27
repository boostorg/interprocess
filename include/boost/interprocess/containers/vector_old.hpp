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

#ifndef BOOST_SHMEM_VECTOR_HPP
#define BOOST_SHMEM_VECTOR_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/config_begin.hpp>
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
#include <boost/detail/no_exceptions_support.hpp>


namespace boost {

namespace interprocess {

namespace detail {

/*!This struct deallocates and allocated memory */
template <class A>
struct vector_alloc_holder : public A
{
   typedef typename A::pointer   pointer;

   //Constructor, does not throw
   vector_alloc_holder(const A &a)
      : A(a), m_start(0), m_finish(0), m_end(0)
   {}

   //Destructor
   ~vector_alloc_holder()
      {  this->priv_deallocate(); }

   pointer m_start;
   pointer m_finish;
   pointer m_end;

 private:
   void priv_deallocate()
   {
      if(!m_start)   return;
      this->deallocate(m_start, m_end - m_start);
      m_end       = m_start;
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
   typedef range_from_ref_iterator<const T, difference_type> range_from_ref_it;

   public:
   //Vector const_iterator
    class const_iterator
   : public boost::iterator<std::random_access_iterator_tag, 
                             value_type,     vec_diff, 
                             vec_cptr,       vec_cref         >
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
    class iterator : public const_iterator
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
         {return this->m_ptr[off];}

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
      : base_t(a)
   {
      if(n){
         //If there is an exception, the base class will
         //deallocate the memory
         this->m_start  = this->allocate(n, 0);
         this->m_end    = this->m_start + n;
         this->m_finish = this->m_start;
         this->priv_uninitialized_fill_n(this->m_start, n, value, *this);
         this->m_finish = this->m_end;
      }
   }

   //Copy constructor
   vector(const vector<T, A>& x) 
      : base_t(x.get_allocator())
   {
      size_type n = x.size();
      if(n){
         this->m_start  = this->allocate(x.size(), 0);
         this->m_end    = this->m_start + x.size();
         this->m_finish = this->m_start;
         this->priv_uninitialized_copy(x.m_start, x.m_finish, this->m_start, *this); 
         this->m_finish = this->m_end;
      }
   }

   //Construct from iterator range
   template <class InIt>
   vector(InIt first, InIt last,
          const allocator_type& a = allocator_type())
      : base_t(a)
   {
      const bool aux_boolean = boost::is_integral<InIt>::value;
      typedef boost::mpl::bool_<aux_boolean> Result;
      this->priv_initialize_aux(first, last, Result());
   }

   //Destructor
  ~vector() 
      {  this->priv_destroy_all();  }

   //Functions: begin, rbegin, end, rend
   iterator begin()      
      { return iterator(this->m_start); }

   iterator end()        
      { return iterator(this->m_finish); }

   const_iterator begin() const
      { return const_iterator(this->m_start); }

   const_iterator end()   const
      { return const_iterator(this->m_finish); }

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
      { return *(this->m_finish - 1); }

   const_reference   back()  const 
      { return *(this->m_finish - 1); }

   //Functions: size, max_size, capacity, empty
   size_type size() const 
      { return size_type(this->m_finish - this->m_start); }

   size_type max_size() const 
      { return allocator_type::max_size(); }

   size_type capacity() const 
      { return size_type(this->m_end - this->m_start); }

   bool empty() const 
      { return this->m_start == this->m_finish;        }

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
      if (capacity() < n){
         //Allocate new size, copy data and free old buffer
         const size_type old_size = size();
         pointer tmp = this->priv_allocate_and_copy(n, this->m_start, this->m_finish);
         this->priv_destroy_and_deallocate();
         this->m_start = tmp;
         this->m_finish = tmp + old_size;
         this->m_end = this->m_start + n;
      }
   }

   //Assignment
   vector<T, A>& operator=(const vector<T, A>& x)
   {
      if (&x != this){
         const size_type xlen = x.size();

         if (xlen > this->capacity()){
            //There is no memory to hold values from x. Allocate a new buffer,
            //copy data from x, and destroy and deallocate previous values
            pointer tmp = this->priv_allocate_and_copy(xlen, x.m_start, x.m_finish);
            this->priv_destroy_and_deallocate();
            this->m_start = tmp;
            this->m_end = this->m_start + xlen;
         }
         else if (size() >= xlen){
            //There is enough memory but current buffer has more objects than x.
            //Copy all objects from x and destroy the remaining objects
            pointer i = std::copy(x.m_start, x.m_finish, this->m_start);
            this->priv_destroy_range(i, this->m_finish);
         }
         else {
            //There is enough memory but current buffer has less objects than x.
            //Copy the first this->size() objects from x and initialize the 
            //rest of the buffer with remaining x objects
            std::copy(x.m_start, x.m_start + size(), this->m_start);
            this->priv_uninitialized_copy(x.m_start + size(), x.m_finish, this->m_finish, *this);
         }
         this->m_finish = this->m_start + xlen;
      }
      return *this;
   }

   void assign(size_type n, const value_type& val = T())
   {
      this->assign(range_from_ref_it(val, n)
                  ,range_from_ref_it());
/*
      if (n > this->capacity()){
         //There is not enough memory, create a temporal vector and swap
         vector<T, A> tmp(n, val, static_cast<const allocator_type&>(*this));
         tmp.swap(*this);
      }
      else if (n > size()){
         //There is enough memory, but we have less objects. Assign current 
         //objects and initialize the rest.
         std::fill(this->m_start, this->m_finish, val);
         size_t rest = n - size();
         this->priv_uninitialized_fill_n(this->m_finish, rest, val, *this);
         this->m_finish += rest;
      }
      else{
         //There is enough memory, but we have more objects. Assign the first 
         //n objects and destroy the rest
         std::fill_n(this->m_start, n, val);
         this->erase(iterator(this->m_start + n), iterator(this->m_finish));
      }*/
   }

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
      if (this->m_finish != this->m_end){
         //There is more memory, just construct a new object at the end
         this->construct(this->m_finish, x);
         ++this->m_finish;
      }
      else{
         //No enough memory, insert a new object at the end
         this->insert(this->end(), static_cast<size_type>(1), x);
      }
   }
   
   void swap(vector<T, A>& x) 
   {
      allocator_type &this_al = *this, &other_al = x;
      //Just swap pointers
      detail::do_swap(this->m_start, x.m_start);
      detail::do_swap(this->m_finish, x.m_finish);
      detail::do_swap(this->m_end, x.m_end);

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

   void insert (iterator position, size_type n, const T& x)
   {
      this->insert(position
                  ,range_from_ref_it(x, n)
                  ,range_from_ref_it());
/*
      if (n != 0){
         if (size_type(this->m_end - this->m_finish) >= n){
            //There is enough memory
            const size_type elems_after = this->end() - position;
            pointer old_finish = this->m_finish;

            if (elems_after > n){
               //New elements can be just copied.
               //Move to uninitialized memory last objects
               this->priv_uninitialized_copy(this->m_finish - n,  this->m_finish, 
                                             this->m_finish,      *this);
               this->m_finish += n;
               //Copy previous to last objects to the initialized end
               std::copy_backward(position.get_ptr(), old_finish - n, 
                                  old_finish);
               //Insert new objects in the position
               std::fill(position.get_ptr(), position.get_ptr() + n, x);
            }
            else {
               //The new elements don't fit in the [position, end()) range. Copy
               //to the beginning of the unallocated zone the last new elements.
               this->priv_uninitialized_fill_n(this->m_finish, n - elems_after, x, *this);
               this->m_finish += n - elems_after;
               //Copy old [position, end()) elements to the uninitialized memory
               this->priv_uninitialized_copy(position.get_ptr(),  old_finish, 
                                             this->m_finish,      *this);
               this->m_finish += elems_after;
               //Insert first new elements in position
               std::fill(position.get_ptr(), old_finish, x);
            }
         }
         else {
            //There is not enough memory, allocate a new buffer
            size_type old_size   = size();        
            size_type len        = old_size + ((old_size/2 > n) ? old_size/2 : n);
            pointer new_start    = this->allocate(len, 0);

            scoped_ptr<T, dealloc_t> scoped_alloc(new_start, dealloc_t(*this, len));
            pointer new_finish   = new_start;
            //Initialize [begin(), position) old buffer to the start of the new buffer
            new_finish = this->priv_uninitialized_copy(this->m_start, position.get_ptr(), 
                                                       new_start, *this);
            //Initialize new n objects, starting from previous point
            this->priv_uninitialized_fill_n(new_finish, n, x, *this);
            new_finish = new_finish + n;
            //Initialize from the rest of the old buffer, starting from previous point
            new_finish = this->priv_uninitialized_copy(position.get_ptr(), this->m_finish, 
                                                       new_finish,         *this);
            //New data is in the new buffer, release scoped deallocation
            scoped_alloc.release();
            //Destroy and deallocate old elements
            this->priv_destroy_and_deallocate();
            this->m_start = new_start;
            this->m_finish = new_finish;
            this->m_end = new_start + len;
         }
      }*/
   }

   void pop_back() 
   {
      //Destroy last element
      --this->m_finish;
      this->destroy(this->m_finish);
   }

   // Return type of erase(const_iterator):
   // http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2005/n1753.html#130

   iterator erase(const_iterator position) 
   {
      if (position + 1 != end()){
         //If not the last element, copy left [position, end()) elements
         std::copy((position + 1).get_ptr(), this->m_finish, position.get_ptr());
      }
      --this->m_finish;
      //Destroy last element
      this->destroy(this->m_finish);
      return iterator(position.get_ptr());
   }

   iterator erase(const_iterator first, const_iterator last) 
   {
      //Overwrite [last, end()) elements to first
      pointer i = std::copy(last.get_ptr(), this->m_finish, first.get_ptr());
      //Destroy remaining objects
      this->priv_destroy_range(i, this->m_finish);
      this->m_finish = this->m_finish - (last - first);
      return iterator(first.get_ptr());
   }

   void resize(size_type new_size, const T& x = T()) 
   {
      if (new_size < size()) 
         //Destroy last elements
         this->erase(iterator(this->m_start + new_size), iterator(this->m_finish));
      else
         //Insert new elements at the end
         this->insert(iterator(this->m_finish), new_size - size(), x);
   }

   void clear() 
      {  this->priv_destroy_all();  }

 private:

   void priv_destroy_all()
   {
      if(!this->m_start)
         return;

      pointer start  = this->m_start;

      for(;start != this->m_finish; ++start) 
         this->destroy(start);

      this->m_finish = this->m_start;
   }

   template <class InIt>
   void priv_range_initialize(InIt first,  
                              InIt last, std::input_iterator_tag)
   {
      //Just insert all elements at the end
      std::copy(first, last, std::back_inserter(*this));
   }

   template <class FwdIt>
   void priv_range_initialize(FwdIt first,
                           FwdIt last, std::forward_iterator_tag)
   {
      //Calculate distance, allocate and copy
      size_type n = 0;
      n = std::distance(first, last);
      this->m_start = this->allocate(n, 0);

      scoped_ptr<T, dealloc_t> scoped_alloc(this->m_start, dealloc_t(*this, n));
      this->m_end = this->m_start + n;
      this->m_finish = this->priv_uninitialized_copy(first, last, this->m_start, *this);
      scoped_alloc.release();
   }

   template <class FwdIt>
   void priv_range_insert(iterator position,     FwdIt first, 
                        FwdIt last, std::forward_iterator_tag)
   {
      if (first != last){
         size_type n = 0;
         n = std::distance(first, last);
         if (size_type(this->m_end - this->m_finish) >= n){
            //There is enough memory
            const size_type elems_after = this->m_finish - position.get_ptr();
            pointer old_finish = this->m_finish;
            if (elems_after > n){
               //New elements can be just copied.
               //Move to unitialized memory last objects
               this->priv_uninitialized_copy(this->m_finish - n,  this->m_finish, 
                                             this->m_finish,      *this);
               this->m_finish += n;
               //Copy previous to last objects to the initialized end
               std::copy_backward(position.get_ptr(), old_finish - n, old_finish);
               //Insert new objects in the position
               std::copy(first, last, position.get_ptr());
            }
            else {
               //The new elements don't fit in the [position, end()) range. Copy
               //to the beginning of the unalocated zone the last new elements.
               FwdIt mid = first;
               std::advance(mid, elems_after);
               this->priv_uninitialized_copy(mid, last, this->m_finish, *this);
               this->m_finish += n - elems_after;
               //Copy old [position, end()) elements to the unitialized memory
               this->priv_uninitialized_copy(position.get_ptr(),  old_finish, 
                                             this->m_finish,            *this);
               this->m_finish += elems_after;
               //Copy first new elements in position
               std::copy(first, mid, position.get_ptr());
            }
         }
         else {
            //There is not enough memory, allocate a new buffer
            const size_type old_size = size();
            const size_type len = old_size + ((old_size/2 > n) ? old_size/2 : n);
            pointer new_start = this->allocate(len, 0);
            scoped_ptr<T, dealloc_t> scoped_alloc(new_start, dealloc_t(*this, len));
            pointer new_finish = new_start;
            BOOST_TRY{
               //Initialize with [begin(), position) old buffer 
               //the start of the new buffer
               new_finish = this->priv_uninitialized_copy
                              (this->m_start, position.get_ptr(), new_start, *this);
               //Initialize new objects, starting from previous point
               new_finish = this->priv_uninitialized_copy
                              (first, last, new_finish, *this);
               //Initialize from the rest of the old buffer, 
               //starting from previous point
               new_finish = this->priv_uninitialized_copy
                              (position.get_ptr(), this->m_finish, new_finish, *this);
            }
            BOOST_CATCH(...){
               this->priv_destroy_range(new_start, new_finish);
               this->deallocate(new_start, len);
               BOOST_RETHROW
            }
            BOOST_CATCH_END
            scoped_alloc.release();
            //Destroy and deallocate old elements
            this->priv_destroy_and_deallocate();
            this->m_start = new_start;
            this->m_finish = new_finish;
            this->m_end = new_start + len;
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
         //There are no more elements in the secuence, erase remaining
         this->erase(cur, end());
      }
      else{
         //There are more elements in the range, insert the remaining ones
         this->insert(end(), first, last);
      }
   }

   template <class FwdIt>
   void priv_assign_aux(FwdIt first, FwdIt last,
                     std::forward_iterator_tag)
   {
      size_type len = 0;
      len = std::distance(first, last);

      if (len > capacity()){
         //There is no enough memory, allocate and copy new buffer
         pointer tmp = this->priv_allocate_and_copy(len, first, last);
         //Destroy and deallocate old buffer
         this->priv_destroy_and_deallocate();
         this->m_start = tmp;
         this->m_end = this->m_finish = this->m_start + len;
      }
      else if (size() >= len){
         //There is memory, but there are more old elements than new ones
         //Overwrite old elements with new ones
         pointer new_finish = std::copy(first, last, this->m_start);
         //Destroy remaining old elements
         this->priv_destroy_range(new_finish, this->m_finish);
         this->m_finish = new_finish;
      }
      else {
         //There is memory, but there are less old elements than new ones
         //Overwrite old elements with new ones
         FwdIt mid = first;
         std::advance(mid, size());
         std::copy(first, mid, this->m_start);
         //Initialize the remaining new elements in the unitialized memory
         this->m_finish = this->priv_uninitialized_copy(mid, last, this->m_finish, *this);
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
   void priv_insert_dispatch(iterator pos,         InIt first, 
                             InIt last,  boost::mpl::false_)
   {
      //Dispatch depending on integer/iterator
      typedef typename 
         std::iterator_traits<InIt>::iterator_category ItCat;
      this->priv_range_insert(pos, first, last, ItCat());
   }

   void priv_destroy_range(pointer p, pointer p2)
      { for(;p != p2; ++p) this->destroy(p); }

   template <class Integer>
   void priv_initialize_aux(Integer n, Integer value, boost::mpl::true_) 
   {
      //Allocate and initialize integer vector buffer
      this->m_start  = this->allocate(n, 0);
      scoped_ptr<T, dealloc_t> scoped_alloc(this->m_start, dealloc_t(*this, n));
      this->m_end    = this->m_start + n; 
      this->m_finish = this->m_start;
      this->priv_uninitialized_fill_n(this->m_start, n, value, *this);
      this->m_finish = this->m_start + n;
      scoped_alloc.release();
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
         this->priv_destroy_range(this->m_start, this->m_finish); 
         this->deallocate(this->m_start, this->m_end - this->m_start);
      }
   }

   template <class FwdIt>
   pointer priv_allocate_and_copy(size_type n, FwdIt first, 
                                               FwdIt last)
   {
      //Allocate n element buffer and initialize from range
      pointer result = this->allocate(n, 0);
      scoped_ptr<T, dealloc_t> scoped_alloc(result, dealloc_t(*this, n));
      this->priv_uninitialized_copy(first, last, result, *this);
      scoped_alloc.release();
      return result;
   }

   void priv_check_range(size_type n) const 
   {
      //If n is out of range, throw an out_of_range exception
      if (n >= size())
         throw std::out_of_range("vector::at");
   }

   template<class InIt, class FwdIt, class alloc> inline
   FwdIt priv_uninitialized_copy(InIt first,   InIt last, 
                                 FwdIt dest,    alloc& al)
   {
      //Save initial destination position
       FwdIt dest_init = dest;

       BOOST_TRY{
         //Try to build objects
          for (; first != last; ++dest, ++first){
              al.construct(dest, *first);
         }
      }
       BOOST_CATCH(...){
         //Call destructors
          for (; dest_init != dest; ++dest_init){
              al.destroy(dest_init);
         }
          BOOST_RETHROW;
       }
      BOOST_CATCH_END
       return (dest);
   }

   template<class FwdIt, class Count, class Alloc> inline
   void priv_uninitialized_fill_n(FwdIt first,  Count count, 
                                  const T& val, Alloc& al)
   {
      //Save initial position
       FwdIt init = first;

       BOOST_TRY{
         //Construct objects
          for (; count--; ++first){
              al.construct(first, val);
         }
      }
       BOOST_CATCH(...){
         //Call destructors
          for (; init != first; ++init){
              al.destroy(init);
         }
          BOOST_RETHROW;
       }
      BOOST_CATCH_END
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

#endif //   #ifndef  BOOST_SHMEM_VECTOR_HPP

