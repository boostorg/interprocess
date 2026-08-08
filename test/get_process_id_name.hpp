//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2004-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_GET_PROCESS_ID_NAME_HPP
#define BOOST_INTERPROCESS_GET_PROCESS_ID_NAME_HPP

#include <boost/config.hpp>
#include <string>    //std::string
#include <sstream>   //std::stringstream
#include <ctime>     //std::time
#include <boost/interprocess/detail/os_thread_functions.hpp>
#include <boost/interprocess/detail/os_file_functions.hpp>

namespace boost{
namespace interprocess{
namespace test{

//Process ids get reused by the OS, so on a machine that has accumulated
//leftover files/resources from previous (e.g. crashed) test runs, a name
//based only on the process id can collide with one of those leftovers.
//Mixing in the process start time makes new names collision-free against
//any such leftovers without needing to clean them up first.
inline long get_process_unique_stamp()
{
   static const long stamp = static_cast<long>(std::time(0));
   return stamp;
}

inline void get_process_id_name(std::string &str)
{
   //Create a short name since some OSes have a limit on the length
   //of names for shared resources like shared memory (e.g. 31 chars MacOs)
   std::stringstream sstr;
   sstr << "bip" << boost::interprocess::ipcdetail::get_current_process_id()
        << get_process_unique_stamp() << std::ends;
   str = sstr.str().c_str();
}

inline void get_process_id_ptr_name(std::string &str, const void *ptr)
{
   std::stringstream sstr;
   sstr << "process_" << boost::interprocess::ipcdetail::get_current_process_id()
        << "_" << get_process_unique_stamp() << "_" << ptr << std::ends;
   str = sstr.str().c_str();
}

inline const char *get_process_id_name()
{
   static bool done = false;
   static std::string str;
   if(!done)
      get_process_id_name(str);
   return str.c_str();
}

inline const char *get_process_id_ptr_name(void *ptr)
{
   static bool done = false;
   static std::string str;
   if(!done)
      get_process_id_ptr_name(str, ptr);
   return str.c_str();
}

inline const char *add_to_process_id_name(const char *name)
{
   static bool done = false;
   static std::string str;
   if(!done){
      get_process_id_name(str);
      str += name;
   }
   return str.c_str();
}

inline const char *add_to_process_id_ptr_name(const char *name, void *ptr)
{
   static bool done = false;
   static std::string str;
   if(!done){
      get_process_id_ptr_name(str, ptr);
      str += name;
   }
   return str.c_str();
}

}  //namespace test{

inline std::string get_filename()
{
   std::string ret (ipcdetail::get_temporary_path());
   ret += "/";
   ret += test::get_process_id_name();
   return ret;
}

#ifdef BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES

namespace test {

inline void get_process_id_wname(std::wstring &str)
{
   std::wstringstream sstr;
   sstr << L"bip" << boost::interprocess::ipcdetail::get_current_process_id()
        << get_process_unique_stamp() << std::ends;
   str = sstr.str().c_str();
}

inline const wchar_t *get_process_id_wname()
{
   static bool done = false;
   static std::wstring str;
   if(!done)
      get_process_id_wname(str);

   return str.c_str();
}

inline const wchar_t *add_to_process_id_name(const wchar_t *name)
{
   static bool done = false;
   static std::wstring str;
   if(!done){
      get_process_id_wname(str);
      str += name;
   }
   return str.c_str();
}

}  //namespace test {

inline std::wstring get_wfilename()
{
   std::wstring ret (ipcdetail::get_temporary_wpath());
   ret += L"/";
   ret += test::get_process_id_wname();
   return ret;
}

#endif

namespace test {

inline const char *get_argv_2(char *argv[])
{  return argv[2]; }

}  //namespace test {

}  //namespace interprocess{
}  //namespace boost{

#endif //#ifndef BOOST_INTERPROCESS_GET_PROCESS_ID_NAME_HPP
