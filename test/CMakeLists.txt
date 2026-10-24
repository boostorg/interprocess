include(BoostTest OPTIONAL RESULT_VARIABLE HAVE_BOOST_TEST)

if(NOT HAVE_BOOST_TEST)
  return()
endif()

#The tests are multithreaded, as in Jamfile.v2 (<threading>multi)
find_package(Threads REQUIRED)

#Boost.Interprocess plus the libraries the tests include directly. util.hpp
#uses Boost.Chrono in header-only mode, but the target is still needed for the
#include directories
set(BOOST_TEST_LINK_LIBRARIES Boost::interprocess Boost::chrono Boost::container_hash
    Boost::core Boost::date_time Boost::unordered Threads::Threads)

#Every .cpp file in this directory is a test, as in Jamfile.v2
file(GLOB tests RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)

foreach(test IN LISTS tests)
  boost_test(SOURCES ${test})
endforeach()
