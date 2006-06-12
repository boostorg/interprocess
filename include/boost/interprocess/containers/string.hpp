/*
 * Copyright (c) 1997-1999
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
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
// This file comes from SGI's string file. Modified by Ion Gaztañaga 2004-2006
// Renaming, isolating and porting to generic algorithms. Pointer typedef 
// set to allocator::pointer to allow placing it in shared memory.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_STRING_HPP
#define BOOST_INTERPROCESS_STRING_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#include <boost/detail/workaround.hpp>
#include <boost/config.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/version_type.hpp>
#include <boost/interprocess/allocators/allocation_type.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <functional>
#include <string>
#include <stdexcept>      
#include <utility>  
#include <iterator>
#include <memory>
#include <algorithm>
#include <iosfwd>
#include <ios>
#include <locale>
#include <cstddef>
//#include <ctype>
#include <boost/detail/no_exceptions_support.hpp>

// Standard C++ string class.  This class has performance
// characteristics very much like vector<>, meaning, for example, that
// it does not perform reference-count or copy-on-write, and that
// concatenation of two strings is an O(N) operation. 

// There are three reasons why basic_string is not identical to
// vector.  First, basic_string always stores a null character at the
// end; this makes it possible for c_str to be a fast operation.
// Second, the C++ standard requires basic_string to copy elements
// using char_traits<>::assign, char_traits<>::copy, and
// char_traits<>::move.  This means that all of vector<>'s low-level
// operations must be rewritten.  Third, basic_string<> has a lot of
// extra functions in its interface that are convenient but, strictly
// speaking, redundant.

namespace boost { namespace interprocess {

// ------------------------------------------------------------
// Class basic_string_base.  

// basic_string_base is a helper class that makes it it easier to write
// an exception-safe version of basic_string.  The constructor allocates,
// but does not initialize, a block of memory.  The destructor
// deallocates, but does not destroy elements within, a block of
// memory.  The destructor assumes that this->m_start either is the internal buffer, 
// or else points to a block of memory that was allocated using _String_base's 
// allocator and whose size is this->m_storage.
template <class A>
class basic_string_base : private A
{
 public:
   typedef A allocator_type;
   typedef typename A::pointer     pointer;
   typedef typename A::value_type  value_type;
   typedef typename A::size_type   size_type;

   allocator_type get_allocator() const { return *this; }

   basic_string_base(const allocator_type& a)
      : allocator_type(a), 
      m_start(0), 
      m_length(0), 
      m_storage(0)
   {}

   basic_string_base(const allocator_type& a, std::size_t n)
      : allocator_type(a), 
      m_start(0), 
      m_length(0), 
      m_storage(0)
   { if(n)  this->allocate_block(n); }

   ~basic_string_base() 
   { this->deallocate_block(); }

   protected:
   enum {   InternalBufferSize = 16   };
   enum {   MinAllocation = InternalBufferSize*2   };

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
      if (limit_size > InternalBufferSize)
         return this->allocation_command
            (command, limit_size, preferred_size, received_size, reuse, alloc_version());
      received_size = InternalBufferSize;
      return std::pair<pointer, bool>(m_buf_holder.m_buf
                                     ,reuse == pointer(m_buf_holder.m_buf));
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
   {  return get_next_capacity(A::max_size(), this->m_storage, additional_objects);  }

   void deallocate(pointer p, std::size_t n) 
   {  
      if (p && (n > InternalBufferSize))
         allocator_type::deallocate(p, n);
   }

   void construct(pointer p, const value_type &value = value_type())
   {  allocator_type::construct(p, value);   }

   void destroy(pointer p, size_type n)
   {  for(; n--; ++p) allocator_type::destroy(p);  }

   void destroy(pointer p)
   { allocator_type::destroy(p); }

   void allocate_block(std::size_t n)
   { 
      if (n <= this->max_size()) {
         size_type new_cap = this->next_capacity(n);
         m_start  = this->allocation_command(allocate_new, n, new_cap, new_cap).first;
         this->m_length = 0;
         m_storage = new_cap;
      }
      else
         throw_length_error();
   }

   void deallocate_block() 
   {  this->deallocate(m_start, m_storage);  }
      
   std::size_t max_size() const
   {  return A::max_size() - 1; }

   // Helper functions for exception handling.
   void throw_length_error() const
   {  throw(std::length_error("basic_string"));  }

   void throw_out_of_range() const
   {  throw(std::out_of_range("basic_string"));  }

   A & get_alloc()
   {  return *this;  }

   const A & get_alloc() const
   {  return *this;  }

   private:
   union Buffer
   {
      value_type m_buf[InternalBufferSize];
      value_type dummy;
   } m_buf_holder;

   protected:
   pointer     m_start;
	size_type   m_length;	// current length of string
	size_type   m_storage;	// current storage reserved for string

   void swap(basic_string_base& other) 
   {
      detail::do_swap(this->m_start, other.m_start);
      detail::do_swap(this->m_buf_holder, other.m_buf_holder);
      value_type *this_buf  = detail::get_pointer(this->m_start);
      value_type *other_buf = detail::get_pointer(other.m_start);

      if(this_buf == other.m_buf_holder.m_buf){
         this->m_start = this->m_buf_holder.m_buf;
      }
      
      if(other_buf == this->m_buf_holder.m_buf){
         other.m_start = other.m_buf_holder.m_buf;
      }

      detail::do_swap(this->m_length, other.m_length);
      detail::do_swap(this->m_storage, other.m_storage);

      allocator_type & this_al = this->get_alloc(), &other_al = other.get_alloc();
      if(this_al != other_al){
         detail::do_swap(this_al, other_al);
      }
   }
};

// ------------------------------------------------------------
// Class basic_string.  

// Class invariants:
// (1) [start, finish) is a valid range.
// (2) Each iterator in [start, finish) points to a valid object
//     of type value_type.
// (3) *finish is a valid object of type value_type; in particular,
//     it is value_type().
// (4) [finish + 1, end_of_storage) is a valid range.
// (5) Each iterator in [finish + 1, end_of_storage) points to 
//     uninitialized memory.

// Note one important consequence: a string of length n must manage
// a block of memory whose size is at least n + 1.  


template <class CharT, class Traits, class A> 
class basic_string : private basic_string_base<A> 
{
   private:
   typedef basic_string_base<A> base_t;
   enum {   InternalBufferSize = base_t::InternalBufferSize   };

   protected:
   // A helper class to use a char_traits as a function object.

   template <class Tr>
   struct Eq_traits
      : public std::binary_function<typename Tr::char_type,
                                    typename Tr::char_type,
                                    bool>
   {
      bool operator()(const typename Tr::char_type& x,
                      const typename Tr::char_type& y) const
         { return Tr::eq(x, y); }
   };

   template <class Tr>
   struct Not_within_traits
      : public std::unary_function<typename Tr::char_type, bool>
   {
      typedef const typename Tr::char_type* Pointer;
      const Pointer m_first;
      const Pointer m_last;

      Not_within_traits(Pointer f, Pointer l) 
         : m_first(f), m_last(l) {}

      bool operator()(const typename Tr::char_type& x) const 
      {
         return std::find_if(m_first, m_last, 
                        std::bind1st(Eq_traits<Tr>(), x)) == m_last;
      }
   };

 public:
   typedef A                                   allocator_type;
   typedef CharT                                   value_type;
   typedef Traits                                  traits_type;
   typedef typename A::pointer                 pointer;
   typedef typename A::const_pointer           const_pointer;
   typedef typename A::reference               reference;
   typedef typename A::const_reference         const_reference;
   typedef typename A::size_type               size_type;
   typedef typename A::difference_type         difference_type;
   typedef const_pointer                           const_iterator;
   typedef pointer                                 iterator;
	typedef boost::reverse_iterator<iterator>       reverse_iterator;
	typedef boost::reverse_iterator<const_iterator> const_reverse_iterator;
   static const size_type npos;

   private:
   typedef constant_iterator<CharT, difference_type> cvalue_iterator;

   public:                         // Constructor, destructor, assignment.
   allocator_type get_allocator() const 
      { return base_t::get_allocator(); }

   explicit basic_string(const allocator_type& a = allocator_type())
      : base_t(a, InternalBufferSize)
   { this->priv_terminate_string(); }

   struct reserve_t {};

   basic_string(reserve_t, std::size_t n,
               const allocator_type& a = allocator_type())
      : base_t(a, n + 1)
   { this->priv_terminate_string(); }

   basic_string(const basic_string& s) 
      : base_t(s.get_allocator()) 
   { this->priv_range_initialize(s.begin(), s.end()); }

   basic_string(const basic_string& s, size_type pos, size_type n = npos,
               const allocator_type& a = allocator_type()) 
      : base_t(a) 
   {
      if (pos > s.size())
         this->throw_out_of_range();
      else
         this->priv_range_initialize
            (s.begin() + pos, s.begin() + pos + std::min(n, s.size() - pos));
   }

   basic_string(const CharT* s, size_type n,
               const allocator_type& a = allocator_type()) 
      : base_t(a) 
   { this->priv_range_initialize(s, s + n); }

   basic_string(const CharT* s,
                const allocator_type& a = allocator_type())
      : base_t(a) 
   { this->priv_range_initialize(s, s + Traits::length(s)); }

   basic_string(size_type n, CharT c,
                const allocator_type& a = allocator_type())
      : base_t(a)
   {  
      this->priv_range_initialize(cvalue_iterator(c, n),
                                  cvalue_iterator());
   }

   // Check to see if InputIterator is an integer type.  If so, then
   // it can't be an iterator.
   template <class InputIterator>
   basic_string(InputIterator f, InputIterator l,
               const allocator_type& a = allocator_type())
      : base_t(a)
   {
      //Dispatch depending on integer/iterator
      const bool aux_boolean = boost::is_integral<InputIterator>::value;
      typedef boost::mpl::bool_<aux_boolean> Result;
      this->priv_initialize_dispatch(f, l, Result());
   }

   ~basic_string() 
   { this->destroy(this->m_start, this->m_length + 1); }
      
   basic_string& operator=(const basic_string& s)
   {
      if (&s != this) 
         this->assign(s.begin(), s.end());
      return *this;
   }

   basic_string& operator=(const CharT* s) 
   { return this->assign(s, s + Traits::length(s)); }

   basic_string& operator=(CharT c)
   { return this->assign(static_cast<size_type>(1), c); }

 private:                         // Helper functions used by constructors
                                 // and elsewhere.
   void priv_construct_null(pointer p)
   {  this->construct(p, 0);  }

   static CharT priv_null()
   {  return (CharT) 0; }

 private:                        
   // Helper functions used by constructors.  It is a severe error for
   // any of them to be called anywhere except from within constructors.
   void priv_terminate_string() 
   {
      BOOST_TRY{
         this->priv_construct_null(this->m_start + this->m_length);
      }
      BOOST_CATCH(...){
         this->destroy(this->m_start, this->m_length);
         BOOST_RETHROW
      }
      BOOST_CATCH_END
   }

   template <class InputIter>
   void priv_range_initialize(InputIter f, InputIter l,
                              std::input_iterator_tag)
   {
      this->allocate_block(InternalBufferSize);
      this->priv_construct_null(this->m_start + this->m_length);
      BOOST_TRY{
         this->append(f, l);
      }
      BOOST_CATCH(...){
         this->destroy(this->m_start, this->m_length + 1);
         BOOST_RETHROW
      }
      BOOST_CATCH_END
   }

   template <class ForwardIter>
   void priv_range_initialize(ForwardIter f, ForwardIter l, 
                              std::forward_iterator_tag)
   {
      difference_type n = std::distance(f, l);
      this->allocate_block(std::max<difference_type>(n+1, InternalBufferSize));
      priv_uninitialized_copy(f, l, this->m_start, this->get_alloc());
      this->m_length = n;
      this->priv_terminate_string();
   }

   template <class InputIter>
   void priv_range_initialize(InputIter f, InputIter l)
   {
      typedef typename std::iterator_traits<InputIter>::iterator_category Category;
      this->priv_range_initialize(f, l, Category());
   }

   template <class Integer>
   void priv_initialize_dispatch(Integer n, Integer x, boost::mpl::true_)
   {
      this->allocate_block(std::max<difference_type>(n+1, InternalBufferSize));
      priv_uninitialized_fill_n(this->m_start, n, x, this->get_alloc());
      this->m_length = n;
      this->priv_terminate_string();
   }

   template <class InputIter>
   void priv_initialize_dispatch(InputIter f, InputIter l, boost::mpl::false_)
   {  this->priv_range_initialize(f, l);  }
    
   public:                         // Iterators.
   iterator begin()             { return this->m_start; }
   iterator end()               { return this->m_start + this->m_length; }
   const_iterator begin() const { return this->m_start; }
   const_iterator end()   const { return this->m_start + this->m_length; }  

   reverse_iterator rbegin()             
      { return reverse_iterator(this->m_start + this->m_length); }
   reverse_iterator rend()               
      { return reverse_iterator(this->m_start); }
   const_reverse_iterator rbegin() const 
      { return const_reverse_iterator(this->m_start + this->m_length); }
   const_reverse_iterator rend()   const 
      { return const_reverse_iterator(this->m_start); }

 public:                         // Size, capacity, etc.
   size_type size() const     { return this->m_length; }
   size_type length() const   { return this->size(); }

   size_type max_size() const { return base_t::max_size(); }

   void resize(size_type n, CharT c)
   {
      if (n <= size())
         this->erase(this->begin() + n, this->end());
      else
         this->append(n - this->size(), c);
   }

   void resize(size_type n) { resize(n, this->priv_null()); }

   // Change the string's capacity so that it is large enough to hold
   //  at least priv_res_arg elements, plus the terminating null.
   void reserve(size_type res_arg)
   {
      if (res_arg > this->max_size())
         this->throw_length_error();

      if (this->capacity() < res_arg){
         size_type n = std::max(res_arg, this->size()) + 1;
         size_type new_cap = this->next_capacity(n);
         pointer new_start = this->allocation_command
            (allocate_new, n, new_cap, new_cap).first;
         size_type new_length = 0;

         BOOST_TRY{
            new_length += priv_uninitialized_copy
               (this->m_start, this->m_start + this->m_length, new_start, this->get_alloc());
            this->priv_construct_null(new_start + new_length);
         }
         BOOST_CATCH(...){
            this->destroy(new_start, new_length);
            this->deallocate(new_start, new_cap);
            BOOST_RETHROW
         }
         BOOST_CATCH_END

         this->destroy(this->m_start, this->m_length + 1);
         this->deallocate_block();
         this->m_start   = new_start;
         this->m_length  = new_length;
         this->m_storage = new_cap;
      }
   }

   size_type capacity() const
   { return this->m_storage - 1; }

   void clear()
   {
      if (!empty()) {
         Traits::assign(*this->m_start, this->priv_null());
         this->destroy(this->m_start+1, this->m_length);
         this->m_length = 0;
      }
   } 

   bool empty() const { return !this->m_length; }

   public:                         // Element access.

   const_reference operator[](size_type n) const
      { return *(this->m_start + n); }
   reference operator[](size_type n)
      { return *(this->m_start + n); }

   const_reference at(size_type n) const {
      if (n >= size())
      this->throw_out_of_range();
      return *(this->m_start + n);
   }

   reference at(size_type n) {
      if (n >= size())
      this->throw_out_of_range();
      return *(this->m_start + n);
   }

   public:                         // Append, operator+=, push_back.

   basic_string& operator+=(const basic_string& s)
   {  return this->append(s); }

   basic_string& operator+=(const CharT* s)
   {  return this->append(s); }

   basic_string& operator+=(CharT c)
   {  this->push_back(c); return *this;   }

   basic_string& append(const basic_string& s) 
   {  return this->append(s.begin(), s.end());  }

   basic_string& append(const basic_string& s, size_type pos, size_type n)
   {
      if (pos > s.size())
      this->throw_out_of_range();
      return this->append(s.begin() + pos,
                          s.begin() + pos + std::min(n, s.size() - pos));
   }

   basic_string& append(const CharT* s, size_type n) 
   {  return this->append(s, s + n);  }

   basic_string& append(const CharT* s) 
   {  return this->append(s, s + Traits::length(s));  }

   basic_string& append(size_type n, CharT c)
   {  return this->append(cvalue_iterator(c, n), cvalue_iterator()); }

   template <class InputIter>
   basic_string& append(InputIter first, InputIter last)
   {  this->insert(this->end(), first, last);   return *this;  }

   void push_back(CharT c)
   {
      if ((this->m_length + 1) < this->m_storage){
         this->priv_construct_null(this->m_start + (this->m_length + 1));
         Traits::assign(this->m_start[this->m_length], c);
         ++this->m_length;
      }
      else{
         //No enough memory, insert a new object at the end
         this->append((size_type)1, c);
      }
   }

   void pop_back()
   {
      Traits::assign(this->m_start[this->m_length-1], this->priv_null());
      this->destroy(this->m_start + this->m_length);
      --this->m_length;
   }

   private: // Helper functions for append.
 
   template<class FwdIt, class Count, class Alloc> inline
   void priv_uninitialized_fill_n(FwdIt first,  Count count, 
                                  const CharT val, Alloc& al)
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
	      BOOST_RETHROW
	   }
      BOOST_CATCH_END
   }

   template<class InpIt, class FwdIt, class Alloc> inline
   size_type priv_uninitialized_copy(InpIt first,   InpIt last, 
                                     FwdIt dest,    Alloc& al)
   {
      //Save initial destination position
	   FwdIt dest_init = dest;
      size_type constructed = 0;

	   BOOST_TRY{
         //Try to build objects
	      for (; first != last; ++dest, ++first, ++constructed){
		      al.construct(dest, *first);
         }
      }
	   BOOST_CATCH(...){
         //Call destructors
	      for (; constructed--; ++dest_init){
		      al.destroy(dest_init);
         }
	      BOOST_RETHROW;
	   }
      BOOST_CATCH_END
	   return (constructed);
   }

   public:                         // Assign

   basic_string& assign(const basic_string& s) 
   { return this->assign(s.begin(), s.end()); }

   basic_string& assign(const basic_string& s, 
                        size_type pos, size_type n) {
      if (pos > s.size())
      this->throw_out_of_range();
      return this->assign(s.begin() + pos, 
                          s.begin() + pos + std::min(n, s.size() - pos));
   }

   basic_string& assign(const CharT* s, size_type n)
   {  return this->assign(s, s + n);   }

   basic_string& assign(const CharT* s)
   { return this->assign(s, s + Traits::length(s)); }

   basic_string& assign(size_type n, CharT c)
   {  return this->assign(cvalue_iterator(c, n), cvalue_iterator()); }

   // Check to see if InputIterator is an integer type.  If so, then
   // it can't be an iterator.
   template <class InputIter>
   basic_string& assign(InputIter first, InputIter last) 
   {
      //Dispatch depending on integer/iterator
      const bool aux_boolean = boost::is_integral<InputIter>::value;
      typedef boost::mpl::bool_<aux_boolean> Result;
      return this->priv_assign_dispatch(first, last, Result());
   }

   basic_string& assign(const CharT* f, const CharT* l)
   {
      const std::ptrdiff_t n = l - f;
      if (static_cast<size_type>(n) <= size()) {
         Traits::copy(detail::get_pointer(this->m_start), f, n);
         this->erase(this->m_start + n, this->m_start + this->m_length);
      }
      else {
         Traits::copy(detail::get_pointer(this->m_start), f, size());
         this->append(f + size(), l);
      }
      return *this;
   }

   private:                        // Helper functions for assign.

   template <class Integer>
   basic_string& priv_assign_dispatch(Integer n, Integer x, boost::mpl::true_) 
   {  return this->assign((size_type) n, (CharT) x);   }

   template <class InputIter>
   basic_string& priv_assign_dispatch(InputIter f, InputIter l,
                                      boost::mpl::false_)
   {
      size_type cur = 0;
      CharT *ptr = detail::get_pointer(this->m_start);
      while (f != l && cur != this->m_length) {
         Traits::assign(*ptr, *f);
         ++f;
         ++cur;
         ++ptr;
      }
      if (f == l)
         this->erase(this->m_start + cur, this->m_start + this->m_length);
      else
         this->append(f, l);
      return *this;
   }

   public:                      // Insert

   basic_string& insert(size_type pos, const basic_string& s) 
   {
      if (pos > size())
         this->throw_out_of_range();
      if (this->size() > this->max_size() - s.size())
         this->throw_length_error();
      this->insert(this->m_start + pos, s.begin(), s.end());
      return *this;
   }

   basic_string& insert(size_type pos, const basic_string& s,
                        size_type beg, size_type n) 
   {
      if (pos > this->size() || beg > s.size())
         this->throw_out_of_range();
      size_type len = std::min(n, s.size() - beg);
      if (this->size() > this->max_size() - len)
         this->throw_length_error();
      const CharT *beg_ptr = detail::get_pointer(s.begin()) + beg;
      const CharT *end_ptr = beg_ptr + len;
      this->insert(this->m_start + pos, beg_ptr, end_ptr);
      return *this;
   }

   basic_string& insert(size_type pos, const CharT* s, size_type n) 
   {
      if (pos > this->size())
         this->throw_out_of_range();
      if (this->size() > this->max_size() - n)
         this->throw_length_error();
      this->insert(this->m_start + pos, s, s + n);
      return *this;
   }

   basic_string& insert(size_type pos, const CharT* s) 
   {
      if (pos > size())
         this->throw_out_of_range();
      size_type len = Traits::length(s);
      if (this->size() > this->max_size() - len)
         this->throw_length_error();
      this->insert(this->m_start + pos, s, s + len);
      return *this;
   }
      
   basic_string& insert(size_type pos, size_type n, CharT c) 
   {
      if (pos > this->size())
         this->throw_out_of_range();
      if (this->size() > this->max_size() - n)
         this->throw_length_error();
      this->insert(this->m_start + pos, n, c);
      return *this;
   }

   iterator insert(iterator position, CharT c) 
   {
      size_type new_offset = position - this->m_start + 1;
      this->insert(position, cvalue_iterator(c, 1),
                             cvalue_iterator());
      return this->m_start + new_offset;
   }

   void insert(iterator position, std::size_t n, CharT c)
   {
      this->insert(position, cvalue_iterator(c, n),
                             cvalue_iterator());
   }

   // Check to see if InputIterator is an integer type.  If so, then
   // it can't be an iterator.
   template <class InputIter>
   void insert(iterator p, InputIter first, InputIter last) 
   {
      //Dispatch depending on integer/iterator
      const bool aux_boolean = boost::is_integral<InputIter>::value;
      typedef boost::mpl::bool_<aux_boolean> Result;
      this->priv_insert_dispatch(p, first, last, Result());
   }

   private:                        // Helper functions for insert.

   template <class InputIter>
   void priv_insert(iterator p, InputIter first, InputIter last, std::input_iterator_tag)
   {
      for ( ; first != last; ++first, ++p) {
         p = this->insert(p, *first);
      }
   }

   template <class ForwardIter>
   void priv_insert(iterator position, ForwardIter first, 
                    ForwardIter last,  std::forward_iterator_tag)
   {
      if (first != last) {
         size_type n = std::distance(first, last);
         size_type remaining = this->m_storage - this->m_length - 1;
         if (remaining >= n){
            const size_type elems_after =
               this->m_length - (position - this->m_start);
            size_type old_length = this->m_length;
            if (elems_after >= n) {
               pointer pointer_past_last = this->m_start + this->m_length + 1;
               priv_uninitialized_copy(this->m_start + (this->m_length - n + 1),
                                       pointer_past_last,
                                       pointer_past_last, *this);
               this->m_length += n;
               Traits::move(detail::get_pointer(position + n),
                            detail::get_pointer(position),
                            (elems_after - n) + 1);
               this->priv_copy(first, last, position);
            }
            else {
               ForwardIter mid = first;
               std::advance(mid, elems_after + 1);
               priv_uninitialized_copy(mid, last, this->m_start + this->m_length + 1, *this);
               this->m_length += n - elems_after;
               BOOST_TRY{
                  priv_uninitialized_copy
                     (position, this->m_start + old_length + 1, 
                      this->m_start + this->m_length, *this);
                  this->m_length += elems_after;
               }
               BOOST_CATCH(...){
                  this->destroy(this->m_start + (old_length + 1), this->m_length - (old_length + 1));
                  this->m_length = old_length;
                  BOOST_RETHROW
               }
               BOOST_CATCH_END
               this->priv_copy(first, mid, position);
            }
         }
         else {
            const size_type old_size = this->size();        
            size_type new_cap  = this->next_capacity(n);
            pointer new_start =
               this->allocation_command
                  (allocate_new, old_size + n + 1, new_cap, new_cap).first;
            size_type new_length = 0;
            BOOST_TRY{
               new_length += priv_uninitialized_copy
                              (this->m_start, position, new_start, *this);
               new_length += priv_uninitialized_copy
                              (first, last, new_start + new_length, *this);
               new_length += priv_uninitialized_copy
                              (position, this->m_start + this->m_length, 
                               new_start + new_length, *this);
               this->priv_construct_null(new_start + new_length);
            }
            BOOST_CATCH(...){
               this->destroy(new_start, new_length);
               this->deallocate(new_start, new_cap);
               BOOST_RETHROW
            }
            BOOST_CATCH_END
            this->destroy(this->m_start, this->m_length + 1);
            this->deallocate_block();
            this->m_start   = new_start;
            this->m_length  = new_length;
            this->m_storage = new_cap; 
         }
      }
   }

   template <class Integer>
   void priv_insert_dispatch(iterator p, Integer n, Integer x,
                           boost::mpl::true_) 
   {  insert(p, (size_type) n, (CharT) x);   }

   template <class InputIter>
   void priv_insert_dispatch(iterator p, InputIter first, InputIter last,
                           boost::mpl::false_) 
   {
      typedef typename std::iterator_traits<InputIter>::iterator_category Category;
      priv_insert(p, first, last, Category());
   }

   template <class InputIterator>
   void priv_copy(InputIterator first, InputIterator last, iterator result)
   {
      for ( ; first != last; ++first, ++result)
         Traits::assign(*result, *first);
   }

   void priv_copy(const CharT* first, const CharT* last, CharT* result) 
   {  Traits::copy(result, first, last - first);  }

   public:                         // Erase.

   basic_string& erase(size_type pos = 0, size_type n = npos) 
   {
      if (pos > size())
         this->throw_out_of_range();
      erase(this->m_start + pos, this->m_start + pos + std::min(n, size() - pos));
      return *this;
   }  

   iterator erase(iterator position) 
   {
      // The move includes the terminating null.
      Traits::move(detail::get_pointer(position), 
                   detail::get_pointer(position + 1), 
                   this->m_length - (position - this->m_start));
      this->destroy(this->m_start + this->m_length);
      --this->m_length;
      return position;
   }

   iterator erase(iterator first, iterator last)
   {
      if (first != last) { // The move includes the terminating null.
         size_type num_erased = last - first;
         Traits::move(detail::get_pointer(first), 
                      detail::get_pointer(last), 
                      (this->m_length + 1)-(last - this->m_start));
         size_type new_length = this->m_length - num_erased;
         this->destroy(this->m_start + new_length + 1, num_erased);
         this->m_length = new_length;
      }
      return first;
   }

   public:                      // Replace.  (Conceptually equivalent
                                // to erase followed by insert.)
   basic_string& replace(size_type pos, size_type n, 
                         const basic_string& s) 
   {
      if (pos > size())
         this->throw_out_of_range();
      const size_type len = std::min(n, size() - pos);
      if (this->size() - len >= this->max_size() - s.size())
         this->throw_length_error();
      return this->replace(this->m_start + pos, this->m_start + pos + len, 
                     s.begin(), s.end());
   }

   basic_string& replace(size_type pos1, size_type n1,
                         const basic_string& s,
                         size_type pos2, size_type n2) 
   {
      if (pos1 > size() || pos2 > s.size())
         this->throw_out_of_range();
      const size_type len1 = std::min(n1, size() - pos1);
      const size_type len2 = std::min(n2, s.size() - pos2);
      if (this->size() - len1 >= this->max_size() - len2)
         this->throw_length_error();
      return this->replace(this->m_start + pos1, this->m_start + pos1 + len1,
                     s.m_start + pos2, s.m_start + pos2 + len2);
   }

   basic_string& replace(size_type pos, size_type n1,
                        const CharT* s, size_type n2) 
   {
      if (pos > size())
         this->throw_out_of_range();
      const size_type len = std::min(n1, size() - pos);
      if (n2 > this->max_size() || size() - len >= this->max_size() - n2)
         this->throw_length_error();
      return this->replace(this->m_start + pos, this->m_start + pos + len,
                     s, s + n2);
   }

   basic_string& replace(size_type pos, size_type n1,
                        const CharT* s) 
   {
      if (pos > size())
         this->throw_out_of_range();
      const size_type len = std::min(n1, size() - pos);
      const size_type n2 = Traits::length(s);
      if (n2 > this->max_size() || size() - len >= this->max_size() - n2)
         this->throw_length_error();
      return this->replace(this->m_start + pos, this->m_start + pos + len,
                     s, s + Traits::length(s));
   }

   basic_string& replace(size_type pos, size_type n1,
                        size_type n2, CharT c) 
   {
      if (pos > size())
         this->throw_out_of_range();
      const size_type len = std::min(n1, size() - pos);
      if (n2 > this->max_size() || size() - len >= this->max_size() - n2)
         this->throw_length_error();
      return this->replace(this->m_start + pos, this->m_start + pos + len, n2, c);
   }

   basic_string& replace(iterator first, iterator last, 
                        const basic_string& s) 
   { return this->replace(first, last, s.begin(), s.end()); }

   basic_string& replace(iterator first, iterator last,
                        const CharT* s, size_type n) 
   { return this->replace(first, last, s, s + n); }

   basic_string& replace(iterator first, iterator last,
                        const CharT* s) 
   {  return this->replace(first, last, s, s + Traits::length(s));   }

   basic_string& replace(iterator first, iterator last, 
                         size_type n, CharT c)
   {
      const size_type len = static_cast<size_type>(last - first);
      if (len >= n) {
         Traits::assign(detail::get_pointer(first), n, c);
         erase(first + n, last);
      }
      else {
         Traits::assign(detail::get_pointer(first), len, c);
         insert(last, n - len, c);
      }
      return *this;
   }

   // Check to see if InputIterator is an integer type.  If so, then
   // it can't be an iterator.
   template <class InputIter>
   basic_string& replace(iterator first, iterator last,
                        InputIter f, InputIter l) 
   {
      //Dispatch depending on integer/iterator
      const bool aux_boolean = boost::is_integral<iterator>::value;
      typedef boost::mpl::bool_<aux_boolean> Result;
      return this->priv_replace_dispatch(first, last, f, l,  Result());
   }

 private:                        // Helper functions for replace.

   template <class Integer>
   basic_string& priv_replace_dispatch(iterator first, iterator last,
                                       Integer n, Integer x,
                                       boost::mpl::true_) 
   {  return this->replace(first, last, (size_type) n, (CharT) x);   }

   template <class InputIter>
   basic_string& priv_replace_dispatch(iterator first, iterator last,
                                       InputIter f, InputIter l,
                                       boost::mpl::false_) 
   {
      typedef typename std::iterator_traits<InputIter>::iterator_category Category;
      return this->priv_replace(first, last, f, l, Category());
   }


   template <class InputIter>
   basic_string& priv_replace(iterator first, iterator last,
                              InputIter f, InputIter l, std::input_iterator_tag)
   {
      for ( ; first != last && f != l; ++first, ++f)
         Traits::assign(*first, *f);

      if (f == l)
         this->erase(first, last);
      else
         this->insert(last, f, l);
      return *this;
   }

   template <class ForwardIter>
   basic_string& priv_replace(iterator first, iterator last,
                              ForwardIter f, ForwardIter l, 
                              std::forward_iterator_tag)
   {
      difference_type n = std::distance(f, l);
      const difference_type len = last - first;
      if (len >= n) {
         this->priv_copy(f, l, first);
        this->erase(first + n, last);
      }
      else {
         ForwardIter m = f;
         std::advance(m, len);
         this->priv_copy(f, m, first);
         this->insert(last, m, l);
      }
      return *this;
   }

   public:                         // Other modifier member functions.

   size_type copy(CharT* s, size_type n, size_type pos = 0) const 
   {
      if (pos > size())
         this->throw_out_of_range();
      const size_type len = std::min(n, size() - pos);
      Traits::copy(s, detail::get_pointer(this->m_start + pos), len);
      return len;
   }

   void swap(basic_string& s) 
   {  base_t::swap(s);  }

 public:                         // Conversion to C string.

   const CharT* c_str() const 
   {  return detail::get_pointer(this->m_start); }

   const CharT* data()  const 
   {  return detail::get_pointer(this->m_start); }

 public:                         // find.

   size_type find(const basic_string& s, size_type pos = 0) const 
   { return find(s.c_str(), pos, s.size()); }

   size_type find(const CharT* s, size_type pos = 0) const 
   { return find(s, pos, Traits::length(s)); }

   size_type find(const CharT* s, size_type pos, size_type n) const
   {
      if (pos + n > size())
         return npos;
      else {
         pointer finish = this->m_start + this->m_length;
         const const_iterator result =
            std::search(detail::get_pointer(this->m_start + pos), 
                   detail::get_pointer(finish),
                   s, s + n, Eq_traits<Traits>());
         return result != finish ? result - begin() : npos;
      }
   }

   size_type find(CharT c, size_type pos = 0) const
   {
      if (pos >= size())
         return npos;
      else {
         pointer finish = this->m_start + this->m_length;
         const const_iterator result =
            std::find_if(this->m_start + pos, finish,
                  std::bind2nd(Eq_traits<Traits>(), c));
         return result != finish ? result - begin() : npos;
      }
   }

 public:                         // rfind.

   size_type rfind(const basic_string& s, size_type pos = npos) const 
      { return rfind(s.c_str(), pos, s.size()); }

   size_type rfind(const CharT* s, size_type pos = npos) const 
      { return rfind(s, pos, Traits::length(s)); }

   size_type rfind(const CharT* s, size_type pos, size_type n) const
   {
      const std::size_t len = size();

      if (n > len)
         return npos;
      else if (n == 0)
         return std::min(len, pos);
      else {
         const const_iterator last = begin() + std::min(len - n, pos) + n;
         const const_iterator result = find_end(begin(), last,
                                                s, s + n,
                                                Eq_traits<Traits>());
         return result != last ? result - begin() : npos;
      }
   }

   size_type rfind(CharT c, size_type pos = npos) const
   {
      const size_type len = size();

      if (len < 1)
         return npos;
      else {
         const const_iterator last = begin() + std::min(len - 1, pos) + 1;
         const_reverse_iterator rresult =
            std::find_if(const_reverse_iterator(last), rend(),
                  std::bind2nd(Eq_traits<Traits>(), c));
         return rresult != rend() ? (rresult.base() - 1) - begin() : npos;
      }
   }

 public:                         // find_first_of

   size_type find_first_of(const basic_string& s, size_type pos = 0) const 
      { return find_first_of(s.c_str(), pos, s.size()); }

   size_type find_first_of(const CharT* s, size_type pos = 0) const 
      { return find_first_of(s, pos, Traits::length(s)); }

   size_type find_first_of(const CharT* s, size_type pos, 
                           size_type n) const
   {
      if (pos >= size())
         return npos;
      else {
         pointer finish = this->m_start + this->m_length;
         const_iterator result = std::find_first_of(this->m_start + pos, finish,
                                                    s, s + n,
                                                    Eq_traits<Traits>());
         return result != finish ? result - begin() : npos;
      }
   }

  size_type find_first_of(CharT c, size_type pos = 0) const 
    { return find(c, pos); }

 public:                         // find_last_of

   size_type find_last_of(const basic_string& s,
                           size_type pos = npos) const
      { return find_last_of(s.c_str(), pos, s.size()); }

   size_type find_last_of(const CharT* s, size_type pos = npos) const 
      { return find_last_of(s, pos, Traits::length(s)); }

   size_type find_last_of(const CharT* s, size_type pos, size_type n) const
   {
      const size_type len = size();

      if (len < 1)
         return npos;
      else {
         const const_iterator last = this->m_start + std::min(len - 1, pos) + 1;
         const const_reverse_iterator rresult =
            std::find_first_of(const_reverse_iterator(last), rend(),
                               s, s + n,
                               Eq_traits<Traits>());
         return rresult != rend() ? (rresult.base() - 1) - this->m_start : npos;
      }
   }

   size_type find_last_of(CharT c, size_type pos = npos) const 
      {  return rfind(c, pos);   }

 public:                         // find_first_not_of

   size_type find_first_not_of(const basic_string& s, 
                              size_type pos = 0) const 
      { return find_first_not_of(s.c_str(), pos, s.size()); }

   size_type find_first_not_of(const CharT* s, size_type pos = 0) const 
      { return find_first_not_of(s, pos, Traits::length(s)); }

   size_type find_first_not_of(const CharT* s, size_type pos,
                              size_type n) const
   {
      if (pos > size())
         return npos;
      else {
         pointer finish = this->m_start + this->m_length;
         const_iterator result = std::find_if(this->m_start + pos, finish,
                                    Not_within_traits<Traits>(s, s + n));
         return result != finish ? result - this->m_start : npos;
      }
   }

   size_type find_first_not_of(CharT c, size_type pos = 0) const
   {
      if (pos > size())
         return npos;
      else {
         pointer finish = this->m_start + this->m_length;
         const_iterator result
            = std::find_if(this->m_start + pos, finish,
                     std::not1(std::bind2nd(Eq_traits<Traits>(), c)));
         return result != finish ? result - begin() : npos;
      }
   }

 public:                         // find_last_not_of

   size_type find_last_not_of(const basic_string& s, 
                              size_type pos = npos) const
      { return find_last_not_of(s.c_str(), pos, s.size()); }

   size_type find_last_not_of(const CharT* s, size_type pos = npos) const
      { return find_last_not_of(s, pos, Traits::length(s)); }

   size_type find_last_not_of(const CharT* s, size_type pos, size_type n) const
   {
      const size_type len = size();

      if (len < 1)
         return npos;
      else {
         const const_iterator last = begin() + std::min(len - 1, pos) + 1;
         const const_reverse_iterator rresult =
            std::find_if(const_reverse_iterator(last), rend(),
                    Not_within_traits<Traits>(s, s + n));
         return rresult != rend() ? (rresult.base() - 1) - begin() : npos;
      }
   }

   size_type find_last_not_of(CharT c, size_type pos = npos) const
   {
      const size_type len = size();

      if (len < 1)
         return npos;
      else {
         const const_iterator last = begin() + std::min(len - 1, pos) + 1;
         const_reverse_iterator rresult =
            std::find_if(const_reverse_iterator(last), rend(),
                  std::not1(std::bind2nd(Eq_traits<Traits>(), c)));
         return rresult != rend() ? (rresult.base() - 1) - begin() : npos;
      }
   }

 public:                         // Substring.

   basic_string substr(size_type pos = 0, size_type n = npos) const 
   {
      if (pos > size())
         this->throw_out_of_range();
      return basic_string(this->m_start + pos, 
                          this->m_start + pos + std::min(n, size() - pos), this->get_alloc());
   }

 public:                         // Compare

   int compare(const basic_string& s) const 
   { return this->s_compare(this->m_start, this->m_start + this->m_length, s.m_start, s.m_start + s.m_length); }

   int compare(size_type pos1, size_type n1, const basic_string& s) const 
   {
      if (pos1 > size())
         this->throw_out_of_range();
      return this->s_compare(this->m_start + pos1, 
                        this->m_start + pos1 + std::min(n1, size() - pos1),
                        s.m_start, s.m_start + s.m_length);
   }
      
   int compare(size_type pos1, size_type n1,
               const basic_string& s,
               size_type pos2, size_type n2) const {
      if (pos1 > size() || pos2 > s.size())
      this->throw_out_of_range();
      return this->s_compare(this->m_start + pos1, 
                        this->m_start + pos1 + std::min(n1, size() - pos1),
                        s.m_start + pos2, 
                        s.m_start + pos2 + std::min(n2, size() - pos2));
   }

   int compare(const CharT* s) const 
   {  return this->s_compare(this->m_start, this->m_start + this->m_length, s, s + Traits::length(s));   }

   int compare(size_type pos1, size_type n1, const CharT* s) const 
   {
      if (pos1 > size())
         this->throw_out_of_range();
      return this->s_compare(this->m_start + pos1, 
                        this->m_start + pos1 + std::min(n1, size() - pos1),
                        s, s + Traits::length(s));
   }

  int compare(size_type pos1, size_type n1, const CharT* s,
              size_type n2) const 
   {
      if (pos1 > size())
         this->throw_out_of_range();
      return this->s_compare(this->m_start + pos1, 
                        this->m_start + pos1 + std::min(n1, size() - pos1),
                        s, s + n2);
   }

public:                        // Helper function for compare.
   static int s_compare(const_pointer f1, const_pointer l1,
                        const_pointer f2, const_pointer l2) 
   {
      const std::ptrdiff_t n1 = l1 - f1;
      const std::ptrdiff_t n2 = l2 - f2;
      const int cmp = Traits::compare(detail::get_pointer(f1), 
                                      detail::get_pointer(f2), 
                                      std::min(n1, n2));
      return cmp != 0 ? cmp : (n1 < n2 ? -1 : (n1 > n2 ? 1 : 0));
   }
};

template <class CharT, class Traits, class A> 
const typename basic_string<CharT,Traits,A>::size_type 
basic_string<CharT,Traits,A>::npos 
  = (typename basic_string<CharT,Traits,A>::size_type) -1;

// ------------------------------------------------------------
// Non-member functions.

// Operator+

template <class CharT, class Traits, class A>
inline basic_string<CharT,Traits,A>
operator+(const basic_string<CharT,Traits,A>& x,
          const basic_string<CharT,Traits,A>& y)
{
   typedef basic_string<CharT,Traits,A> str_t;
   typedef typename str_t::reserve_t reserve_t;
   reserve_t reserve;
   str_t result(reserve, x.size() + y.size(), x.get_allocator());
   result.append(x);
   result.append(y);
   return result;
}

template <class CharT, class Traits, class A>
inline basic_string<CharT,Traits,A>
operator+(const CharT* s, const basic_string<CharT,Traits,A>& y) 
{
   typedef basic_string<CharT,Traits,A> str_t;
   typedef typename str_t::reserve_t reserve_t;
   reserve_t reserve;
   const std::size_t n = Traits::length(s);
   str_t result(reserve, n + y.size());
   result.append(s, s + n);
   result.append(y);
   return result;
}

template <class CharT, class Traits, class A>
inline basic_string<CharT,Traits,A>
operator+(CharT c, const basic_string<CharT,Traits,A>& y) 
{
   typedef basic_string<CharT,Traits,A> str_t;
   typedef typename str_t::reserve_t reserve_t;
   reserve_t reserve;
   str_t result(reserve, 1 + y.size());
   result.push_back(c);
   result.append(y);
   return result;
}

template <class CharT, class Traits, class A>
inline basic_string<CharT,Traits,A>
operator+(const basic_string<CharT,Traits,A>& x, const CharT* s) 
{
   typedef basic_string<CharT,Traits,A> str_t;
   typedef typename str_t::reserve_t reserve_t;
   reserve_t reserve;
   const std::size_t n = Traits::length(s);
   str_t result(reserve, x.size() + n, x.get_allocator());
   result.append(x);
   result.append(s, s + n);
   return result;
}

template <class CharT, class Traits, class A>
inline basic_string<CharT,Traits,A>
operator+(const basic_string<CharT,Traits,A>& x, const CharT c) 
{
  typedef basic_string<CharT,Traits,A> str_t;
  typedef typename str_t::reserve_t reserve_t;
   reserve_t reserve;
   str_t result(reserve, x.size() + 1, x.get_allocator());
   result.append(x);
   result.push_back(c);
   return result;
}

// Operator== and operator!=

template <class CharT, class Traits, class A>
inline bool
operator==(const basic_string<CharT,Traits,A>& x,
           const basic_string<CharT,Traits,A>& y) 
{
   return x.size() == y.size() &&
          Traits::compare(x.data(), y.data(), x.size()) == 0;
}

template <class CharT, class Traits, class A>
inline bool
operator==(const CharT* s, const basic_string<CharT,Traits,A>& y) 
{
   std::size_t n = Traits::length(s);
   return n == y.size() && Traits::compare(s, y.data(), n) == 0;
}

template <class CharT, class Traits, class A>
inline bool
operator==(const basic_string<CharT,Traits,A>& x, const CharT* s) 
{
   std::size_t n = Traits::length(s);
   return x.size() == n && Traits::compare(x.data(), s, n) == 0;
}

template <class CharT, class Traits, class A>
inline bool
operator!=(const basic_string<CharT,Traits,A>& x,
           const basic_string<CharT,Traits,A>& y) 
   {  return !(x == y);  }

template <class CharT, class Traits, class A>
inline bool
operator!=(const CharT* s, const basic_string<CharT,Traits,A>& y) 
   {  return !(s == y); }

template <class CharT, class Traits, class A>
inline bool
operator!=(const basic_string<CharT,Traits,A>& x, const CharT* s) 
   {  return !(x == s);   }


// Operator< (and also >, <=, and >=).

template <class CharT, class Traits, class A>
inline bool
operator<(const basic_string<CharT,Traits,A>& x,
          const basic_string<CharT,Traits,A>& y) 
{
   return basic_string<CharT,Traits,A>
      ::s_compare(x.begin(), x.end(), y.begin(), y.end()) < 0;
}

template <class CharT, class Traits, class A>
inline bool
operator<(const CharT* s, const basic_string<CharT,Traits,A>& y) 
{
   std::size_t n = Traits::length(s);
   return basic_string<CharT,Traits,A>
          ::s_compare(s, s + n, y.begin(), y.end()) < 0;
}

template <class CharT, class Traits, class A>
inline bool
operator<(const basic_string<CharT,Traits,A>& x,
          const CharT* s) 
{
   std::size_t n = Traits::length(s);
   return basic_string<CharT,Traits,A>
      ::s_compare(x.begin(), x.end(), s, s + n) < 0;
}

template <class CharT, class Traits, class A>
inline bool
operator>(const basic_string<CharT,Traits,A>& x,
          const basic_string<CharT,Traits,A>& y) {
   return y < x;
}

template <class CharT, class Traits, class A>
inline bool
operator>(const CharT* s, const basic_string<CharT,Traits,A>& y) {
   return y < s;
}

template <class CharT, class Traits, class A>
inline bool
operator>(const basic_string<CharT,Traits,A>& x, const CharT* s) 
{
   return s < x;
}

template <class CharT, class Traits, class A>
inline bool
operator<=(const basic_string<CharT,Traits,A>& x,
           const basic_string<CharT,Traits,A>& y) 
{
  return !(y < x);
}

template <class CharT, class Traits, class A>
inline bool
operator<=(const CharT* s, const basic_string<CharT,Traits,A>& y) 
   {  return !(y < s);  }

template <class CharT, class Traits, class A>
inline bool
operator<=(const basic_string<CharT,Traits,A>& x, const CharT* s) 
   {  return !(s < x);  }

template <class CharT, class Traits, class A>
inline bool
operator>=(const basic_string<CharT,Traits,A>& x,
           const basic_string<CharT,Traits,A>& y) 
   {  return !(x < y);  }

template <class CharT, class Traits, class A>
inline bool
operator>=(const CharT* s, const basic_string<CharT,Traits,A>& y) 
   {  return !(s < y);  }

template <class CharT, class Traits, class A>
inline bool
operator>=(const basic_string<CharT,Traits,A>& x, const CharT* s) 
   {  return !(x < s);  }

// Swap.

template <class CharT, class Traits, class A>
inline void swap(basic_string<CharT,Traits,A>& x,
                 basic_string<CharT,Traits,A>& y) 
   {  x.swap(y);  }

// I/O.  

template <class CharT, class Traits>
inline bool
sgi_string_fill(std::basic_ostream<CharT, Traits>& os,
                  std::basic_streambuf<CharT, Traits>* buf,
                  std::size_t n)
{
   CharT f = os.fill();
   std::size_t i;
   bool ok = true;

   for (i = 0; i < n; i++)
      ok = ok && !Traits::eq_int_type(buf->sputc(f), Traits::eof());
   return ok;
}

template <class CharT, class Traits, class A>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, 
           const basic_string<CharT,Traits,A>& s)
{
   typename std::basic_ostream<CharT, Traits>::sentry sentry(os);
   bool ok = false;

   if (sentry) {
      ok = true;
      std::size_t n = s.size();
      std::size_t pad_len = 0;
      const bool left = (os.flags() & std::ios::left) != 0;
      const std::size_t w = os.width(0);
      std::basic_streambuf<CharT, Traits>* buf = os.rdbuf();

      if (w != 0 && n < w)
         pad_len = w - n;
       
      if (!left)
         ok = sgi_string_fill(os, buf, pad_len);    

      ok = ok && 
            buf->sputn(s.data(), std::streamsize(n)) == std::streamsize(n);

      if (left)
         ok = ok && sgi_string_fill(os, buf, pad_len);
   }

   if (!ok)
      os.setstate(std::ios_base::failbit);

   return os;
}

template <class CharT, class Traits, class A>
std::basic_istream<CharT, Traits>& 
operator>>(std::basic_istream<CharT, Traits>& is,
           basic_string<CharT,Traits,A>& s)
{
   typename std::basic_istream<CharT, Traits>::sentry sentry(is);

   if (sentry) {
      std::basic_streambuf<CharT, Traits>* buf = is.rdbuf();
      const std::ctype<CharT>& ctype = std::use_facet<std::ctype<CharT> >(is.getloc());

      s.clear();
      std::size_t n = is.width(0);
      if (n == 0)
         n = static_cast<std::size_t>(-1);
      else
         s.reserve(n);

      while (n-- > 0) {
         typename Traits::int_type c1 = buf->sbumpc();

         if (Traits::eq_int_type(c1, Traits::eof())) {
            is.setstate(std::ios_base::eofbit);
            break;
         }
         else {
            CharT c = Traits::to_char_type(c1);

            if (ctype.is(std::ctype<CharT>::space, c)) {
               if (Traits::eq_int_type(buf->sputbackc(c), Traits::eof()))
                  is.setstate(std::ios_base::failbit);
               break;
            }
            else
               s.push_back(c);
         }
      }
      
      // If we have read no characters, then set failbit.
      if (s.size() == 0)
         is.setstate(std::ios_base::failbit);
   }
   else
      is.setstate(std::ios_base::failbit);

   return is;
}

template <class CharT, class Traits, class A>    
std::basic_istream<CharT, Traits>& 
getline(std::istream& is,
        basic_string<CharT,Traits,A>& s,
        CharT delim)
{
   std::size_t nread = 0;
   typename std::basic_istream<CharT, Traits>::sentry sentry(is, true);
   if (sentry) {
      std::basic_streambuf<CharT, Traits>* buf = is.rdbuf();
      s.clear();

      int c1;
      while (nread < s.max_size()) {
         int c1 = buf->sbumpc();
         if (Traits::eq_int_type(c1, Traits::eof())) {
            is.setstate(std::ios_base::eofbit);
            break;
         }
         else {
            ++nread;
            CharT c = Traits::to_char_type(c1);
            if (!Traits::eq(c, delim)) 
               s.push_back(c);
            else
               break;              // Character is extracted but not appended.
         }
      }
   }
   if (nread == 0 || nread >= s.max_size())
      is.setstate(std::ios_base::failbit);

   return is;
}

template <class CharT, class Traits, class A>    
inline std::basic_istream<CharT, Traits>& 
getline(std::basic_istream<CharT, Traits>& is,
        basic_string<CharT,Traits,A>& s)
{
   return getline(is, s, '\n');
}

template <class Ch, class A>
inline std::size_t hash_value(basic_string<Ch, std::char_traits<Ch>, A> const& v)
{
   return hash_range(v.begin(), v.end());
}

typedef basic_string<char, std::char_traits<char>, std::allocator<char> > string;

}} //namespace boost {  namespace interprocess

#include <boost/interprocess/detail/config_end.hpp>

#endif // BOOST_INTERPROCESS_STRING_HPP
