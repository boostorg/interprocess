//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_PTIME_TO_TIMESPEC_HPP
#define BOOST_INTERPROCESS_DETAIL_PTIME_TO_TIMESPEC_HPP

#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <time.h>

namespace boost {

namespace interprocess {

namespace detail {

inline timespec ptime_to_timespec (const boost::posix_time::ptime &tm)
{
   const boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
   boost::posix_time::time_duration duration (tm - epoch);
   timespec ts;
   ts.tv_sec  = duration.total_seconds();
   ts.tv_nsec = duration.total_nanoseconds() % 1000000000;
   return ts;
}

}  //namespace detail {

}  //namespace interprocess {

}  //namespace boost {

#endif   //ifndef BOOST_INTERPROCESS_DETAIL_PTIME_TO_TIMESPEC_HPP
