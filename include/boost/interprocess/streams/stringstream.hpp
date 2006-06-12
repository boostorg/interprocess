/*
 * Copyright (c) 1998
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
// This file comes from SGI's sstream file. Modified by Ion Gaztañaga 2005.
// Changed internal SGI string to a generic, templatized string. Added efficient
// internal buffer get/set/swap functions, so that we can obtain/establish the
// internal buffer without any reallocation or copy. Kill those temporaries!
///////////////////////////////////////////////////////////////////////////////

/*!\file
   This file defines basic_stringbuf, basic_istringstream,
   basic_ostringstream, and basic_stringstreamclasses.  These classes
   represent streamsbufs and streams whose sources or destinations are
   STL-like strings that can be swapped with external strings to avoid 
   unnecessary allocations/copies.
*/

#ifndef BOOST_INTERPROCESS_STRINGSTREAM_HPP
#define BOOST_INTERPROCESS_STRINGSTREAM_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <iosfwd>
#include <ios>
#include <istream>
#include <ostream>
#include <string>    // char traits            
#include <cstddef>   // ptrdiff_t
#include <boost/interprocess/interprocess_fwd.hpp>
#include <assert.h>

namespace boost {  namespace interprocess {

/*!A streambuf class that controls the transmission of elements to and from
   a basic_istringstream, basic_ostringstream or basic_stringstream. 
   It holds a character string specified by CharString template parameter
   as its formatting buffer. The string must have contiguous storage, like 
   std::string, boost::interprocess::string or boost::interprocess::basic_string*/
template <class CharString, class CharTraits>
class basic_stringbuf
   : public std::basic_streambuf<typename CharString::value_type, CharTraits>
{
   public:
   typedef CharString                        string_type;
   typedef typename CharString::value_type   char_type;
   typedef typename CharTraits::int_type     int_type;
   typedef typename CharTraits::pos_type     pos_type;
   typedef typename CharTraits::off_type     off_type;
   typedef CharTraits                        traits_type;

   private:
   typedef std::basic_streambuf<char_type, traits_type> base_t;

   basic_stringbuf(const basic_stringbuf&);
   basic_stringbuf & operator =(const basic_stringbuf&);

   public:
   /*!Constructor. Throws if string_type default constructor throws.*/
   explicit basic_stringbuf(std::ios_base::openmode mode
                              = std::ios_base::in | std::ios_base::out)
      :  base_t(), m_mode(mode)
      {  this->set_pointers();   }

   /*!Constructor. Throws if string_type(const VectorParameter &param) throws.*/
   template<class VectorParameter>
   explicit basic_stringbuf(const VectorParameter &param,
                            std::ios_base::openmode mode
                                 = std::ios_base::in | std::ios_base::out)
      :  base_t(), m_mode(mode), m_string(param)
      {  this->set_pointers();   }

   virtual ~basic_stringbuf(){}

   public:

   /*!Swaps the underlying string with the passed string. 
      This function resets the read/write position in the stream.
      Does not throw.*/
   void swap_string(string_type &vect)
      {  m_string.swap(vect);   this->set_pointers();   }

   /*!Returns a const reference to the internal string.
      Does not throw.*/
   const string_type &string() const { return m_string; }

   /*!Calls resize() method of the internal string.
      Resets the stream to the first position.
      Throws if the internals string's resize throws.*/
   void resize(typename string_type::size_type size) 
      { m_string.resize(size); this->set_pointers();   }

   private:
   void set_pointers()
   {
      // The initial read position is the beginning of the string.
      if(m_mode & std::ios_base::in)
         this->setg(&m_string[0], &m_string[0], &m_string[m_string.size()]);

      // The initial write position is the beginning of the string.
      if(m_mode & std::ios_base::out)
         this->setp(&m_string[0], &m_string[m_string.size()]);
   }

   protected:
   virtual int_type underflow()
   {
      // Precondition: gptr() >= egptr(). Returns a character, if available.
      return this->gptr() != this->egptr()
         ? CharTraits::to_int_type(*this->gptr())
         : CharTraits::eof();
   }

   virtual int_type uflow()
   {
      // Precondition: gptr() >= egptr().
      if(this->gptr() != this->egptr()) {
         int_type c = CharTraits::to_int_type(*this->gptr());
         this->gbump(1);
         return c;
      }
      else
         return CharTraits::eof();
   }

   virtual int_type pbackfail(int_type c = CharTraits::eof())
   {
      if(this->gptr() != this->eback()) {
         if(!CharTraits::eq_int_type(c, CharTraits::eof())) {
            if(CharTraits::eq(CharTraits::to_char_type(c), this->gptr()[-1])) {
               this->gbump(-1);
               return c;
            }
            else if(m_mode & std::ios_base::out) {
               this->gbump(-1);
               *this->gptr() = c;
               return c;
            }
            else
               return CharTraits::eof();
         }
         else {
            this->gbump(-1);
            return CharTraits::not_eof(c);
         }
      }
      else
         return CharTraits::eof();
   }

   virtual int_type overflow(int_type c = CharTraits::eof())
   {
      if(m_mode & std::ios_base::out) {
         if(!CharTraits::eq_int_type(c, CharTraits::eof())) {
            if(!(m_mode & std::ios_base::in)) {
               if(this->pptr() != this->epptr()) {
                  *this->pptr() = CharTraits::to_char_type(c);
                  this->pbump(1);
                  return c;
               }
               else
                  return CharTraits::eof();
            }
            else {
               // We're not using a special append buffer, just the string itself.
               if(this->pptr() == this->epptr()) {
                  std::ptrdiff_t offset = this->gptr() - this->eback();
                  m_string.push_back(CharTraits::to_char_type(c));
                  this->setg(&m_string[0], &m_string[offset], &m_string[m_string.size()]);
                  this->setp(&m_string[0], &m_string[m_string.size()]);
                  this->pbump(static_cast<int>(m_string.size()));
                  return c;
               }
               else {
                  *this->pptr() = CharTraits::to_char_type(c);
                  this->pbump(1);
                  return c;
               }
            }
         }
         else  // c is EOF, so we don't have to do anything
            return CharTraits::not_eof(c);
      }
      else     // Overflow always fails if it's read-only.
         return CharTraits::eof();
   }

   virtual std::streamsize xsputn(const char_type* s, std::streamsize n)
   {
      std::streamsize nwritten = 0;

      if((m_mode & std::ios_base::out) && n > 0) {
         // If the put pointer is somewhere in the middle of the
         // string, then overwrite instead of append.
         assert(this->pbase() == &m_string[0]);
//         if(this->pbase() == &m_string[0]) {
            std::streamsize avail = static_cast<std::streamsize>(
               &m_string[m_string.size()] - this->pptr());
            if(avail > n) {
               CharTraits::copy(this->pptr(), s, n);
               this->pbump(n);
               return n;
            }
            else if(avail){
               CharTraits::copy(this->pptr(), s, avail);
               nwritten += avail;
               n -= avail;
               s += avail;
            }
//         }

         // At this point we know we're appending.
         if(m_mode & std::ios_base::in) {
            std::ptrdiff_t get_offset = this->gptr() - this->eback();
            m_string.insert(m_string.end(), s, s + n);
            this->setg(&m_string[0], &m_string[get_offset], &m_string[m_string.size()]);
            this->setp(&m_string[0], &m_string[m_string.size()]);
            this->pbump(static_cast<int>(m_string.size()));
         }
         else {
            m_string.insert(m_string.end(), s, s + n);
         }
         nwritten += n;
      }
      return nwritten;
   }

   virtual std::streamsize xsputnc(char_type c, std::streamsize n)
   {
      std::streamsize nwritten = 0;

      if((m_mode & std::ios_base::out) && n > 0) {
         // If the put pointer is somewhere in the middle of the string,
         // then overwrite instead of append.
         assert(this->pbase() == &m_string[0]);
//         if(this->pbase() == &m_string[0]) {
            std::streamsize avail = static_cast<std::streamsize>
                     (&m_string[m_string.size()] - this->pptr());
            if(avail > n) {
               CharTraits::assign(this->pptr(), n, c);
               this->pbump(n);
               return n;
            }
            else if(avail){
               CharTraits::assign(this->pptr(), avail, c);
               nwritten += avail;
               n -= avail;
            }
//         }

         // At this point we know we're appending.
         if(this->m_mode & std::ios_base::in) {
            std::streamsize  get_offset = static_cast<std::streamsize> 
               (this->gptr() - this->eback());
            m_string.insert(m_string.end(), n, c);
            this->setg(&m_string[0], &m_string[get_offset], &m_string[m_string.size()]);
            this->setp(&m_string[0], &m_string[m_string.size()]);
            this->pbump(static_cast<int>(m_string.size()));
         }
         else {
            m_string.insert(m_string.end(), n, c);
         }
         nwritten += n;
      }

      return nwritten;
   }

   virtual base_t* setbuf(char_type* buf, std::streamsize n)
   {
      // According to the C++ standard the effects of setbuf are implementation
      // defined, except that setbuf(0, 0) has no effect.  In this implementation,
      // setbuf(<anything>, n), for n > 0, calls resize(n) on the underlying
      // string.
      if(n > 0) {
//         bool do_get_area = false;
//         bool do_put_area = false;
         std::ptrdiff_t offg = 0;
         std::ptrdiff_t offp = 0;

         assert(this->pbase() == &m_string[0]);
         //if(this->pbase() == &m_string[0]) {
            //do_put_area = true;
            offp = this->pptr() - this->pbase();
         //}

         assert(this->eback() == &m_string[0]);
         //if(this->eback() == &m_string[0]) {
            //do_get_area = true;
            offg = this->gptr() - this->eback();
         //}

         m_string.resize(n);

         //if(do_get_area) {
            this->setg(&m_string[0], &m_string[offg], &m_string[m_string.size()]);
         //}

         //if(do_put_area) {
            this->setp(&m_string[0], &m_string[m_string.size()]);
            this->pbump(static_cast<int>(offp));
         //}
      }
      return this;
   }

   virtual pos_type seekoff(off_type off, std::ios_base::seekdir dir,
                              std::ios_base::openmode mode 
                                 = std::ios_base::in | std::ios_base::out)
   {
      bool in  = false;
      bool out = false;
         
      const std::ios_base::openmode inout = 
         std::ios_base::in | std::ios_base::out;

      if((mode & inout) == inout) {
         if(dir == std::ios_base::beg || dir == std::ios_base::end)
            in = out = true;
      }
      else if(mode & std::ios_base::in)
         in = true;
      else if(mode & std::ios_base::out)
         out = true;

      if(!in && !out)
         return pos_type(off_type(-1));
      else if((in  && (!(m_mode & std::ios_base::in) || this->gptr() == 0)) ||
               (out && (!(m_mode & std::ios_base::out) || this->pptr() == 0)))
         return pos_type(off_type(-1));

      std::streamoff newoff;
      switch(dir) {
         case std::ios_base::beg:
            newoff = 0;
            break;
         case std::ios_base::end:
            newoff = static_cast<std::streamoff>(m_string.size());
            break;
         case std::ios_base::cur:
            newoff = in ? static_cast<std::streamoff>(this->gptr() - this->eback()) 
                        : static_cast<std::streamoff>(this->pptr() - this->pbase());
            break;
         default:
            return pos_type(off_type(-1));
      }

      off += newoff;

      if(in) {
         std::ptrdiff_t n = this->egptr() - this->eback();

         if(off < 0 || off > n)
            return pos_type(off_type(-1));
         else
            this->setg(this->eback(), this->eback() + off, this->eback() + n);
      }

      if(out) {
         std::ptrdiff_t n = this->epptr() - this->pbase();

         if(off < 0 || off > n)
            return pos_type(off_type(-1));
         else {
            this->setp(this->pbase(), this->pbase() + n);
            this->pbump(off);
         }
      }
      return pos_type(off);
   }

   virtual pos_type seekpos(pos_type pos, std::ios_base::openmode mode 
                                 = std::ios_base::in | std::ios_base::out)
   {
      bool in  = (mode & std::ios_base::in) != 0;
      bool out = (mode & std::ios_base::out) != 0;

      if((in  && (!(m_mode & std::ios_base::in) || this->gptr() == 0)) ||
            (out && (!(m_mode & std::ios_base::out) || this->pptr() == 0)))
         return pos_type(off_type(-1));

      const off_type n = pos - pos_type(off_type(0));

      if(in) {
         if(n < 0 || n > this->egptr() - this->eback())
            return pos_type(off_type(-1));
         this->setg(this->eback(), this->eback() + n, this->egptr());
      }

      if(out) {
         if(n < 0 || n > off_type(m_string.size()))
            return pos_type(off_type(-1));
         this->setp(&m_string[0], &m_string[m_string.size()]);
         this->pbump(n);
      }
      return pos;
   }

   private:
   std::ios_base::openmode m_mode;
   string_type             m_string;
};

/*!A basic_istream class that holds a character string specified by CharString
   template parameter as its formatting buffer. The string must have
   contiguous storage, like std::string, boost::interprocess::string or
   boost::interprocess::basic_string*/
template <class CharString, class CharTraits>
class basic_istringstream
: public std::basic_istream<typename CharString::value_type, CharTraits>
{
   public:
   typedef CharString                                                   string_type;
   typedef typename std::basic_ios
      <typename CharString::value_type, CharTraits>::char_type          char_type;
   typedef typename std::basic_ios<char_type, CharTraits>::int_type     int_type;
   typedef typename std::basic_ios<char_type, CharTraits>::pos_type     pos_type;
   typedef typename std::basic_ios<char_type, CharTraits>::off_type     off_type;
   typedef typename std::basic_ios<char_type, CharTraits>::traits_type  traits_type;

   private:
   typedef std::basic_ios<char_type, CharTraits>                basic_ios_t;
   typedef std::basic_istream<char_type, CharTraits>            base_t;

   public:
   /*!Constructor. Throws if string_type default constructor throws.*/
   basic_istringstream(std::ios_base::openmode mode = std::ios_base::in)
      :  basic_ios_t(), base_t(0), m_buf(mode | std::ios_base::in)
      {  basic_ios_t::init(&m_buf); }

   /*!Constructor. Throws if string_type(const VectorParameter &param) throws.*/
   template<class VectorParameter>
   basic_istringstream(const VectorParameter &param,
                       std::ios_base::openmode mode = std::ios_base::in)
      :  basic_ios_t(), base_t(0), 
         m_buf(param, mode | std::ios_base::in)
      {  basic_ios_t::init(&m_buf); }

   ~basic_istringstream(){};

   public:
   /*!Returns the address of the stored stream buffer.*/
   basic_stringbuf<CharString, CharTraits>* rdbuf() const
      { return const_cast<basic_stringbuf<CharString, CharTraits>*>(&m_buf); }

   /*!Swaps the underlying string with the passed string. 
      This function resets the read position in the stream.
      Does not throw.*/
   void swap_string(string_type &vect)
      {  m_buf.swap_string(vect);   }

   /*!Returns a const reference to the internal string.
      Does not throw.*/
   const string_type &string() const 
      {  return m_buf.string();   }

   /*!Calls resize() method of the internal string.
      Resets the stream to the first position.
      Throws if the internals string's resize throws.*/
   void resize(typename string_type::size_type size) 
      {  m_buf.resize(size);   }

   private:
   basic_stringbuf<CharString, CharTraits> m_buf;
};

/*!A basic_ostream class that holds a character string specified by CharString
   template parameter as its formatting buffer. The string must have
   contiguous storage, like std::string, boost::interprocess::string or
   boost::interprocess::basic_string*/
template <class CharString, class CharTraits>
class basic_ostringstream
   : public std::basic_ostream<typename CharString::value_type, CharTraits>
{
   public:
   typedef CharString                                                   string_type;
   typedef typename std::basic_ios
      <typename CharString::value_type, CharTraits>::char_type          char_type;
   typedef typename std::basic_ios<char_type, CharTraits>::int_type     int_type;
   typedef typename std::basic_ios<char_type, CharTraits>::pos_type     pos_type;
   typedef typename std::basic_ios<char_type, CharTraits>::off_type     off_type;
   typedef typename std::basic_ios<char_type, CharTraits>::traits_type  traits_type;

   private:
   typedef std::basic_ios<char_type, CharTraits>      basic_ios_t;
   typedef std::basic_ostream<char_type, CharTraits>  base_t;

   public:
   /*!Constructor. Throws if string_type default constructor throws.*/
   basic_ostringstream(std::ios_base::openmode mode = std::ios_base::out)
      :  basic_ios_t(), base_t(0), m_buf(mode | std::ios_base::out)
      {  basic_ios_t::init(&m_buf); }

   /*!Constructor. Throws if string_type(const VectorParameter &param) throws.*/
   template<class VectorParameter>
   basic_ostringstream(const VectorParameter &param,
                        std::ios_base::openmode mode = std::ios_base::out)
      :  basic_ios_t(), base_t(0), m_buf(param, mode | std::ios_base::out)
      {  basic_ios_t::init(&m_buf); }

   ~basic_ostringstream(){}

   public:
   /*!Returns the address of the stored stream buffer.*/
   basic_stringbuf<CharString, CharTraits>* rdbuf() const
      { return const_cast<basic_stringbuf<CharString, CharTraits>*>(&m_buf); }

   /*!Swaps the underlying string with the passed string. 
      This function resets the write position in the stream.
      Does not throw.*/
   void swap_string(string_type &vect)
      {  m_buf.swap_string(vect);   }

   /*!Returns a const reference to the internal string.
      Does not throw.*/
   const string_type &string() const 
      {  return m_buf.string();   }

   /*!Calls resize() method of the internal string.
      Resets the stream to the first position.
      Throws if the internals string's resize throws.*/
   void resize(typename string_type::size_type size) 
      {  m_buf.resize(size);   }

   private:
   basic_stringbuf<CharString, CharTraits> m_buf;
};


/*!A basic_iostream class that holds a character string specified by CharString
   template parameter as its formatting buffer. The string must have
   contiguous storage, like std::string, boost::interprocess::string or
   boost::interprocess::basic_string*/
template <class CharString, class CharTraits>
class basic_stringstream
: public std::basic_iostream<typename CharString::value_type, CharTraits>

{
   public:
   typedef CharString                                                   string_type;
   typedef typename std::basic_ios
      <typename CharString::value_type, CharTraits>::char_type          char_type;
   typedef typename std::basic_ios<char_type, CharTraits>::int_type     int_type;
   typedef typename std::basic_ios<char_type, CharTraits>::pos_type     pos_type;
   typedef typename std::basic_ios<char_type, CharTraits>::off_type     off_type;
   typedef typename std::basic_ios<char_type, CharTraits>::traits_type  traits_type;

   private:
   typedef std::basic_ios<char_type, CharTraits>                 basic_ios_t;
   typedef std::basic_iostream<char_type, CharTraits>            base_t;

   public:
   /*!Constructor. Throws if string_type default constructor throws.*/
   basic_stringstream(std::ios_base::openmode mode 
                      = std::ios_base::in | std::ios_base::out)
      :  basic_ios_t(), base_t(0), m_buf(mode)
      {  basic_ios_t::init(&m_buf); }

   /*!Constructor. Throws if string_type(const VectorParameter &param) throws.*/
   template<class VectorParameter>
   basic_stringstream(const VectorParameter &param, std::ios_base::openmode mode
                      = std::ios_base::in | std::ios_base::out)
      :  basic_ios_t(), base_t(0), m_buf(param, mode)
      {  basic_ios_t::init(&m_buf); }

   ~basic_stringstream(){}

   public:
   //Returns the address of the stored stream buffer.
   basic_stringbuf<CharString, CharTraits>* rdbuf() const
      { return const_cast<basic_stringbuf<CharString, CharTraits>*>(&m_buf); }

   /*!Swaps the underlying string with the passed string. 
      This function resets the read/write position in the stream.
      Does not throw.*/
   void swap_string(string_type &vect)
      {  m_buf.swap_string(vect);   }

   /*!Returns a const reference to the internal string.
      Does not throw.*/
   const string_type &string() const 
      {  return m_buf.string();   }

   /*!Calls resize() method of the internal string.
      Resets the stream to the first position.
      Throws if the internals string's resize throws.*/
   void resize(typename string_type::size_type size) 
      {  m_buf.resize(size);   }

   private:
   basic_stringbuf<CharString, CharTraits> m_buf;
};

//Some typedefs to simplify usage
/*
typedef basic_stringbuf<std::string<char> >        stringbuf;
typedef basic_stringstream<std::string<char> >     stringstream;
typedef basic_istringstream<std::string<char> >    istringstream;
typedef basic_ostringstream<std::string<char> >    ostringstream;

typedef basic_stringbuf<std::string<wchar_t> >     wstringbuf;
typedef basic_stringstream<std::string<wchar_t> >  wstringstream;
typedef basic_istringstream<std::string<wchar_t> > wistringstream;
typedef basic_ostringstream<std::string<wchar_t> > wostringstream;
*/
}} //namespace boost {  namespace interprocess {

#include <boost/interprocess/detail/config_end.hpp>

#endif /* BOOST_INTERPROCESS_STRINGSTREAM_HPP */
