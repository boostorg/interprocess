//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2004-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include "util.hpp"
#include "mutex_test_template.hpp"
#include "sharable_mutex_test_template.hpp"
#include "get_process_id_name.hpp"
#include <fstream>
#include <string>
#include <cstdio> //std::remove

using namespace boost::interprocess;

//This wrapper is necessary to have a default constructor
//in generic mutex_test_template functions
class file_lock_lock_test_wrapper
   : public boost::interprocess::file_lock
{
   public:
   file_lock_lock_test_wrapper()
      :  boost::interprocess::file_lock(get_filename().c_str())
   {}
};

//The open_or_create constructor must create the file if it does not exist and
//must open an already existing file without truncating it.
int test_open_or_create()
{
   std::remove(get_filename().c_str());

   //The file does not exist, so the open-only constructor must fail
   try{
      file_lock flock(get_filename().c_str());
      return 1;
   }
   catch(interprocess_exception &){}

   //...but the open_or_create constructor must create it
   {
      file_lock flock(open_or_create, get_filename().c_str());
      scoped_lock<file_lock> sl(flock);
   }
   {
      std::ifstream file(get_filename().c_str());
      if(!file){
         return 1;
      }
   }

   //Write some data to check that opening an existing file does not truncate it
   {
      std::ofstream file(get_filename().c_str());
      if(!file){
         return 1;
      }
      file << "0123456789";
   }
   {
      permissions perm;
      perm.set_unrestricted();
      file_lock flock(open_or_create, get_filename().c_str(), perm);
      scoped_lock<file_lock> sl(flock, try_to_lock);
      if(!sl){
         return 1;
      }
   }
   {
      std::ifstream file(get_filename().c_str());
      std::string contents;
      if(!(file >> contents) || contents != "0123456789"){
         return 1;
      }
   }

   #if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES)
   std::remove(get_filename().c_str());
   {
      file_lock flock(open_or_create, get_wfilename().c_str());
      scoped_lock<file_lock> sl(flock);
   }
   {
      std::ifstream file(get_filename().c_str());
      if(!file){
         return 1;
      }
   }
   #endif

   std::remove(get_filename().c_str());
   return 0;
}

int main ()
{
   if(test_open_or_create()){
      return 1;
   }

   //Destroy and create file
   {
      std::remove(get_filename().c_str());
      std::ofstream file(get_filename().c_str());
      if(!file){
         return 1;
      }
      {
         file_lock flock(get_filename().c_str());
         {
         scoped_lock<file_lock> sl(flock);
         }
         {
         scoped_lock<file_lock> sl(flock, try_to_lock);
         }
         {
         scoped_lock<file_lock> sl(flock, test::ptime_delay_ms(1));
         }
         {
         scoped_lock<file_lock> sl(flock, test::boost_systemclock_delay_ms(1));
         }
         {
         scoped_lock<file_lock> sl(flock, test::std_systemclock_delay_ms(1));
         }
      }
      #if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES)
      file_lock flock(get_wfilename().c_str());
      #endif
   }
   {
      //Now test move semantics
      file_lock mapping(get_filename().c_str());
      file_lock move_ctor(boost::move(mapping));
      file_lock move_assign;
      move_assign = boost::move(move_ctor);
      mapping.swap(move_assign);
   }

   test::test_all_lock<file_lock_lock_test_wrapper>();
   //test::test_all_mutex<file_lock_lock_test_wrapper>();
   //test::test_all_sharable_mutex<file_lock_lock_test_wrapper>();
   std::remove(get_filename().c_str());

   return 0;
}

