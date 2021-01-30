//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2004-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/sync/named_semaphore.hpp>
#include <boost/interprocess/detail/interprocess_tester.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "named_creation_template.hpp"
#include "mutex_test_template.hpp"
#include "get_process_id_name.hpp"
#include <exception>

using namespace boost::interprocess;

static const std::size_t RecSemCount   = 100;

//This wrapper is necessary to plug this class
//in lock tests
class lock_test_wrapper
   : public named_semaphore
{
   public:

   template <class CharT>
   lock_test_wrapper(create_only_t, const CharT *name, unsigned int count = 1)
      :  named_semaphore(create_only, name, count)
   {}

   template <class CharT>
   lock_test_wrapper(open_only_t, const CharT *name)
      :  named_semaphore(open_only, name)
   {}

   template <class CharT>
   lock_test_wrapper(open_or_create_t, const CharT *name, unsigned int count = 1)
      :  named_semaphore(open_or_create, name, count)
   {}

   ~lock_test_wrapper()
   {}

   void lock()
   {  this->wait();  }

   bool try_lock()
   {  return this->try_wait();  }

   bool timed_lock(const boost::posix_time::ptime &pt)
   {  return this->timed_wait(pt);  }

   void unlock()
   {  this->post();  }
};

//This wrapper is necessary to plug this class
//in recursive tests
class recursive_test_wrapper
   :  public lock_test_wrapper
{
   public:
   recursive_test_wrapper(create_only_t, const char *name)
      :  lock_test_wrapper(create_only, name, RecSemCount)
   {}

   recursive_test_wrapper(open_only_t, const char *name)
      :  lock_test_wrapper(open_only, name)
   {}

   recursive_test_wrapper(open_or_create_t, const char *name)
      :  lock_test_wrapper(open_or_create, name, RecSemCount)
   {}
};

bool test_named_semaphore_specific()
{
   named_semaphore::remove(test::get_process_id_name());
   //Test persistance
   {
      named_semaphore sem(create_only, test::get_process_id_name(), 3);
   }
   {
      named_semaphore sem(open_only, test::get_process_id_name());
      BOOST_INTERPROCESS_CHECK(sem.try_wait() == true);
      BOOST_INTERPROCESS_CHECK(sem.try_wait() == true);
      BOOST_INTERPROCESS_CHECK(sem.try_wait() == true);
      BOOST_INTERPROCESS_CHECK(sem.try_wait() == false);
      sem.post();
   }
   {
      named_semaphore sem(open_only, test::get_process_id_name());
      BOOST_INTERPROCESS_CHECK(sem.try_wait() == true);
      BOOST_INTERPROCESS_CHECK(sem.try_wait() == false);
   }

   named_semaphore::remove(test::get_process_id_name());
   return true;
}

int main ()
{
   int ret = 0;
   try{
      test::test_named_creation< test::named_sync_creation_test_wrapper<lock_test_wrapper> >();
      #if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES)
      test::test_named_creation< test::named_sync_creation_test_wrapper_w<lock_test_wrapper> >();
      #endif //defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES)

      test::test_all_lock< test::named_sync_wrapper<lock_test_wrapper> >();
      test::test_all_mutex<test::named_sync_wrapper<lock_test_wrapper> >();
      test::test_all_recursive_lock<test::named_sync_wrapper<recursive_test_wrapper> >();
      test_named_semaphore_specific();
   }
   catch(std::exception &ex){
      std::cout << ex.what() << std::endl;
      ret = 1;
   }
   named_semaphore::remove(test::get_process_id_name());
   return ret;
}

