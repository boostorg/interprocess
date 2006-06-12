//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_PTR_WRKRND_HPP
#define BOOST_INTERPROCESS_PTR_WRKRND_HPP

#include <boost/interprocess/detail/config_begin.hpp>

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/iterator.hpp>
#include <boost/iterator/reverse_iterator.hpp>

#ifndef BOOST_INTERPROCESS_NO_DEPENDENT_TYPES_IN_TEMPLATE_VALUE_PARAMETERS
#if defined (BOOST_NO_DEPENDENT_TYPES_IN_TEMPLATE_VALUE_PARAMETERS) || BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))
#define BOOST_INTERPROCESS_NO_DEPENDENT_TYPES_IN_TEMPLATE_VALUE_PARAMETERS
#endif
#endif

#if BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
// old Dinkumware
#  include <boost/compatibility/cpp_c_headers/cstddef>
#else
#  include <cstddef>
#endif

#if !((defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)) && \
      ((defined(_POSIX_VERSION) && _POSIX_VERSION >= 200112L) || \
        defined(_XOPEN_VERSION) && _XOPEN_VERSION >= 500)
#define BOOST_INTERPROCESS_USE_PTHREAD_BARRIER
#endif

namespace boost {

namespace interprocess {

namespace workaround
{
//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*//
//                                              //
//    We want generally const_shm_ptr to inherit//
//    from iterator class but for void this     //
//    doesn't work, so we must inherit from     //
//    other class.                              //
//                                              //
//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*//

//Empty class
struct empty_type{};

template<class T>
struct random_it 
: public boost::iterator<std::random_access_iterator_tag, 
                         T, std::ptrdiff_t, T*, T&> 
{
   typedef const T*           const_pointer;
   typedef const T&           const_reference;
};

template<> struct random_it<void>
{
   typedef void *             pointer;
   typedef const void *       const_pointer;
   typedef empty_type&        reference;
   typedef const empty_type&  const_reference;
   typedef void               value_type;
   typedef empty_type         difference_type;
   typedef empty_type         iterator_category;
};

template<> struct random_it<const void>
{
   typedef const void *       pointer;
   typedef const void *       const_pointer;
   typedef const empty_type & reference;
   typedef const empty_type & const_reference;
   typedef const void         value_type;
   typedef empty_type         difference_type;
   typedef empty_type         iterator_category;
};

template<> struct random_it<volatile void>
{
   typedef volatile void *       pointer;
   typedef const volatile void * const_pointer;
   typedef empty_type&           reference;
   typedef const empty_type&     const_reference;
   typedef volatile void         value_type;
   typedef empty_type            difference_type;
   typedef empty_type            iterator_category;
};

template<> struct random_it<const volatile void>
{
   typedef const volatile void *    pointer;
   typedef const volatile void *    const_pointer;
   typedef const empty_type &       reference;
   typedef const empty_type &       const_reference;
   typedef const volatile void      value_type;
   typedef empty_type               difference_type;
   typedef empty_type               iterator_category;
};

}  //namespace workaround

}  //namespace interprocess {

}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_PTR_WRKRND_HPP
