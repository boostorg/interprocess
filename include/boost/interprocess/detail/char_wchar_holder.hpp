//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2020-2021. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_CHAR_WCHAR_HOLDER_HPP
#define BOOST_INTERPROCESS_DETAIL_CHAR_WCHAR_HOLDER_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <cwchar>
#include <cstring>

namespace boost {
namespace interprocess {

class char_wchar_holder
{
   public:
   char_wchar_holder()
      : m_str(), m_is_wide()
   {
      m_str.n = 0;
   }

   char_wchar_holder(const char *nstr)
      : m_str(), m_is_wide()
   {
      m_str.n = priv_dup(nstr);
   }

   char_wchar_holder(const wchar_t *wstr)
      : m_str(), m_is_wide(true)
   {
      m_str.w = priv_dup(wstr);
   }

   char_wchar_holder(const char_wchar_holder &other)
      : m_str(), m_is_wide(other.m_is_wide)
   {
      if(m_is_wide)
         m_str.w = other.m_str.w ? priv_dup(other.m_str.w) : 0;
      else
         m_str.n = other.m_str.n ? priv_dup(other.m_str.n) : 0;
   }

   char_wchar_holder& operator=(const char *nstr)
   {
      //Copy before freeing: nstr might point to our own buffer
      char *tmp = priv_dup(nstr);
      this->delete_mem();
      m_str.n = tmp;
      m_is_wide = false;
      return *this;
   }

   char_wchar_holder& operator=(const wchar_t *wstr)
   {
      //Copy before freeing: wstr might point to our own buffer
      wchar_t *tmp = priv_dup(wstr);
      this->delete_mem();
      m_str.w = tmp;
      m_is_wide = true;
      return *this;
   }

   char_wchar_holder& operator=(const char_wchar_holder &other)
   {
      if(this != &other){
         if(other.m_is_wide){
            wchar_t *tmp = other.m_str.w ? priv_dup(other.m_str.w) : 0;
            this->delete_mem();
            m_str.w = tmp;
         }
         else{
            char *tmp = other.m_str.n ? priv_dup(other.m_str.n) : 0;
            this->delete_mem();
            m_str.n = tmp;
         }
         m_is_wide = other.m_is_wide;
      }
      return *this;
   }

   ~char_wchar_holder()
   {
      this->delete_mem();
   }

   wchar_t *getw() const
   {  return m_is_wide ? m_str.w : 0; }

   char *getn() const
   {  return !m_is_wide ? m_str.n : 0; }

   void swap(char_wchar_holder& other)
   {
      char_wchar tmp;
      std::memcpy(&tmp, &m_str, sizeof(char_wchar));
      std::memcpy(&m_str, &other.m_str, sizeof(char_wchar));
      std::memcpy(&other.m_str, &tmp, sizeof(char_wchar));
      //
      bool b_tmp(m_is_wide);
      m_is_wide = other.m_is_wide;
      other.m_is_wide = b_tmp;
   }

   private:

   static char *priv_dup(const char *nstr)
   {
      char *p = new char [std::strlen(nstr)+1];
      std::strcpy(p, nstr);
      return p;
   }

   static wchar_t *priv_dup(const wchar_t *wstr)
   {
      wchar_t *p = new wchar_t [std::wcslen(wstr)+1];
      std::wcscpy(p, wstr);
      return p;
   }

   void delete_mem()
   {
      if(m_is_wide)
         delete [] m_str.w;
      else
         delete [] m_str.n;
   }

   union char_wchar
   {
      char    *n;
      wchar_t *w;
   } m_str;
   bool m_is_wide;
};

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_DETAIL_CHAR_WCHAR_HOLDER_HPP
