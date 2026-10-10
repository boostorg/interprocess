//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2026. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/detail/char_wchar_holder.hpp>
#include <boost/core/lightweight_test.hpp>

#include <cstring>
#include <cwchar>

using boost::interprocess::char_wchar_holder;

void test_copy_construct()
{
   {  //narrow
      char_wchar_holder a("narrow");
      char_wchar_holder b(a);
      BOOST_TEST(b.getn() != 0);
      BOOST_TEST(b.getn() != a.getn());
      BOOST_TEST(0 == std::strcmp(b.getn(), "narrow"));
      BOOST_TEST(b.getw() == 0);
   }
   {  //wide
      char_wchar_holder a(L"wide");
      char_wchar_holder b(a);
      BOOST_TEST(b.getw() != 0);
      BOOST_TEST(b.getw() != a.getw());
      BOOST_TEST(0 == std::wcscmp(b.getw(), L"wide"));
      BOOST_TEST(b.getn() == 0);
   }
   {  //empty
      char_wchar_holder a;
      char_wchar_holder b(a);
      BOOST_TEST(b.getn() == 0);
      BOOST_TEST(b.getw() == 0);
   }
}

void test_copy_assign()
{
   {  //narrow over wide
      char_wchar_holder a("narrow");
      char_wchar_holder b(L"wide");
      b = a;
      BOOST_TEST(b.getn() != a.getn());
      BOOST_TEST(0 == std::strcmp(b.getn(), "narrow"));
      BOOST_TEST(b.getw() == 0);
   }
   {  //wide over narrow
      char_wchar_holder a(L"wide");
      char_wchar_holder b("narrow");
      b = a;
      BOOST_TEST(b.getw() != a.getw());
      BOOST_TEST(0 == std::wcscmp(b.getw(), L"wide"));
      BOOST_TEST(b.getn() == 0);
   }
   {  //empty source
      char_wchar_holder a;
      char_wchar_holder b("narrow");
      b = a;
      BOOST_TEST(b.getn() == 0);
      BOOST_TEST(b.getw() == 0);
   }
   {  //self assignment
      char_wchar_holder a("narrow");
      char_wchar_holder &ra = a;
      a = ra;
      BOOST_TEST(0 == std::strcmp(a.getn(), "narrow"));
      char_wchar_holder w(L"wide");
      char_wchar_holder &rw = w;
      w = rw;
      BOOST_TEST(0 == std::wcscmp(w.getw(), L"wide"));
   }
}

void test_assign_own_buffer()
{
   {  //narrow
      char_wchar_holder a("narrow");
      a = a.getn();
      BOOST_TEST(0 == std::strcmp(a.getn(), "narrow"));
   }
   {  //wide
      char_wchar_holder a(L"wide");
      a = a.getw();
      BOOST_TEST(0 == std::wcscmp(a.getw(), L"wide"));
   }
}

void test_swap()
{
   char_wchar_holder a("narrow");
   char_wchar_holder b(L"wide");
   a.swap(b);
   BOOST_TEST(0 == std::wcscmp(a.getw(), L"wide"));
   BOOST_TEST(0 == std::strcmp(b.getn(), "narrow"));
}

int main()
{
   test_copy_construct();
   test_copy_assign();
   test_assign_own_buffer();
   test_swap();
   return boost::report_errors();
}
