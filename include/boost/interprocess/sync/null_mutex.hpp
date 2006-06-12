//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_NULL_MUTEX_HPP
#define BOOST_INTERPROCESS_NULL_MUTEX_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>


/*!\file
   Describes null_mutex classes
*/

namespace boost {

namespace posix_time
{  class ptime;   }

namespace interprocess {

/*!Wraps a interprocess_mutex that does nothing */
class null_mutex
{
   null_mutex(const null_mutex&);
   null_mutex &operator= (const null_mutex&);

   public:

   /*!Empty constructor */
   null_mutex(){}

   /*!Empty destructor */
  ~null_mutex(){}

   /*!Does nothing */
   void lock(){}

   /*!Returns true */
   bool try_lock(){  return true;   }

   /*!Returns true */
   bool timed_lock(const boost::posix_time::ptime &abs_time){  return true;   }

   /*!Does nothing */
   void unlock(){}
   
   #if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))
   private:
   char dummy; // BCB would by default use 8 B for empty structure
   #endif
};

}  //namespace interprocess {

}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_NULL_MUTEX_HPP
