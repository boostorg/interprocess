//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2004-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_GET_COMPILER_NAME_HPP
#define BOOST_GET_COMPILER_NAME_HPP

#include <boost/config.hpp>
#include <string>
#include <algorithm>

namespace boost{
namespace interprocess{
namespace test{

inline void get_compiler_name(std::string &str)
{
   str = BOOST_COMPILER;
   std::replace(str.begin(), str.end(), ' ', '_');
   std::replace(str.begin(), str.end(), '.', '_');
}

inline const char *get_compiler_name()
{
   static std::string str;
   get_compiler_name(str);
   return str.c_str();
}

inline const char *add_to_compiler_name(const char *name)
{
   static std::string str;
   get_compiler_name(str);
   str += name;
   return str.c_str();
}

}  //namespace test{
}  //namespace interprocess{
}  //namespace boost{

#endif //#ifndef BOOST_GET_COMPILER_NAME_HPP
