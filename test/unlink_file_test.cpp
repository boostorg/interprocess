//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2026. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

//Tests that ipcdetail::delete_file (implemented in Windows with
//winapi::unlink_file) offers POSIX unlink-like semantics:
//
//- Deleting the name must succeed even if the file is in use
//  (open handles or mapped regions).
//- The deleted name must be immediately reusable to create a new file.
//- Already opened handles and mapped regions of the deleted file must
//  remain usable and independent from files created later with the
//  same name.

#include <boost/interprocess/detail/os_file_functions.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cstddef>   //std::size_t
#include <cstring>   //std::memcpy, std::memcmp
#include <string>
#include "get_process_id_name.hpp"

using namespace boost::interprocess;

namespace {

template<class CharT>
bool file_exists(const CharT *name)
{
   file_handle_t h = ipcdetail::open_existing_file(name, read_only);
   if(h == ipcdetail::invalid_file())
      return false;
   ipcdetail::close_file(h);
   return true;
}

void closed_file_test(const char *filename)
{
   file_handle_t h = ipcdetail::create_new_file(filename, read_write);
   BOOST_TEST(h != ipcdetail::invalid_file());
   BOOST_TEST(ipcdetail::write_file(h, "unlink", 6u));
   BOOST_TEST(ipcdetail::close_file(h));
   BOOST_TEST(file_exists(filename));

   BOOST_TEST(ipcdetail::delete_file(filename));
   BOOST_TEST(!file_exists(filename));

   //Deleting a non-existing file must fail
   BOOST_TEST(!ipcdetail::delete_file(filename));
}

void in_use_file_test(const char *filename)
{
   file_handle_t h = ipcdetail::create_new_file(filename, read_write);
   BOOST_TEST(h != ipcdetail::invalid_file());

   //Deleting the name must succeed while the handle is open
   BOOST_TEST(ipcdetail::delete_file(filename));
   BOOST_TEST(!file_exists(filename));

   //The name must be immediately reusable
   file_handle_t h2 = ipcdetail::create_new_file(filename, read_write);
   BOOST_TEST(h2 != ipcdetail::invalid_file());

   //The unlinked file must still be usable through the old handle...
   BOOST_TEST(ipcdetail::write_file(h, "still alive", 11u));
   offset_t size = 0;
   BOOST_TEST(ipcdetail::get_file_size(h, size));
   BOOST_TEST_EQ(size, offset_t(11));

   //...and it must be independent from the new file with the old name
   offset_t size2 = 1;
   BOOST_TEST(ipcdetail::get_file_size(h2, size2));
   BOOST_TEST_EQ(size2, offset_t(0));

   BOOST_TEST(ipcdetail::close_file(h));
   BOOST_TEST(ipcdetail::close_file(h2));
   BOOST_TEST(ipcdetail::delete_file(filename));
}

void mapped_file_test(const char *filename)
{
   const std::size_t FileSize = 4096u;
   {
      file_handle_t h = ipcdetail::create_new_file(filename, read_write);
      BOOST_TEST(h != ipcdetail::invalid_file());
      BOOST_TEST(ipcdetail::truncate_file(h, FileSize));
      BOOST_TEST(ipcdetail::close_file(h));
   }
   {
      file_mapping mapping(filename, read_write);
      mapped_region region(mapping, read_write);
      BOOST_TEST_EQ(region.get_size(), FileSize);
      std::memcpy(region.get_address(), "mapped", 6u);

      //Deleting the name must succeed while the mapping is open
      BOOST_TEST(file_mapping::remove(filename));
      BOOST_TEST(!file_exists(filename));

      //The name must be immediately reusable
      file_handle_t h2 = ipcdetail::create_new_file(filename, read_write);
      BOOST_TEST(h2 != ipcdetail::invalid_file());
      BOOST_TEST(ipcdetail::close_file(h2));
      BOOST_TEST(ipcdetail::delete_file(filename));

      //The mapped region must remain intact
      BOOST_TEST(0 == std::memcmp(region.get_address(), "mapped", 6u));
   }
}

#ifdef BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES
void wide_char_test(const wchar_t *filename)
{
   file_handle_t h = ipcdetail::create_new_file(filename, read_write);
   BOOST_TEST(h != ipcdetail::invalid_file());

   //Deleting the name must succeed while the handle is open
   BOOST_TEST(ipcdetail::delete_file(filename));
   BOOST_TEST(!file_exists(filename));

   BOOST_TEST(ipcdetail::close_file(h));
   BOOST_TEST(!ipcdetail::delete_file(filename));
}
#endif

}  //anonymous namespace

int main()
{
   BOOST_INTERPROCESS_TRY{
      const std::string filename = get_filename() + "_unlink_test";
      //Remove any leftover from a previous crashed run (process ids are reused)
      ipcdetail::delete_file(filename.c_str());
      closed_file_test(filename.c_str());
      in_use_file_test(filename.c_str());
      mapped_file_test(filename.c_str());
      #ifdef BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES
      const std::wstring wfilename = get_wfilename() + L"_unlink_test";
      ipcdetail::delete_file(wfilename.c_str());
      wide_char_test(wfilename.c_str());
      #endif
   }
   BOOST_INTERPROCESS_CATCH(...){
      BOOST_ERROR("Unexpected exception thrown");
   } BOOST_INTERPROCESS_CATCH_END

   return ::boost::report_errors();
}
