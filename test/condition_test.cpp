#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/thread/detail/config.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include "condition_test_template.hpp"

using namespace boost::interprocess;

int main ()
{
   return test::do_test_condition<interprocess_condition, interprocess_mutex>() ?
          0 : -1;
}

#include <boost/interprocess/detail/config_end.hpp>
