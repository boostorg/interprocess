//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <cstddef>
#include <cstdio>

using namespace boost::interprocess;
typedef list<int, allocator<int, managed_mapped_file::segment_manager> > 
   MyList;

int main ()
{
   const char *FileName       = "file_mapping";
   const std::size_t FileSize = 1000;
   std::remove(FileName);
   try{
      managed_mapped_file mfile_memory(create_only, FileName, FileSize);
      MyList * mylist = mfile_memory.construct<MyList>("MyList")
                           (mfile_memory.get_segment_manager());

      //Obtain handle, that identifies the list in the buffer
      managed_mapped_file::handle_t list_handle = mfile_memory.get_handle_from_address(mylist);

      //Fill list until there is no more room in the file
      try{
         while(1) {
            mylist->insert(mylist->begin(), 0);
         }
      }
      catch(const bad_alloc &){
         //mapped file is full
      }
      //Let's obtain the size of the list
      std::size_t old_size = mylist->size();

      //To make the list bigger, let's increase the mapped file
      //in FileSize bytes more.
      //mfile_memory.grow(FileSize);

      //If mapping address has changed, the old pointer is invalid,
      //so use previously obtained handle to find the new pointer.
      mylist = static_cast<MyList *>
                  (mfile_memory.get_address_from_handle(list_handle));
      
      //Fill list until there is no more room in the file
      try{
         while(1) {
            mylist->insert(mylist->begin(), 0);
         }
      }
      catch(const bad_alloc &){
         //mapped file is full
      }

      //Let's obtain the new size of the list      
      std::size_t new_size = mylist->size();

      assert(new_size > old_size);

      //Destroy list
      mfile_memory.destroy_ptr(mylist);
   }
   catch(...){
      std::remove(FileName);
      throw;
   }
   std::remove(FileName);
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
