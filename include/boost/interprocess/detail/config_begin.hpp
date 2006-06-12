#ifdef _MSC_VER
   #pragma warning (push)
   //
   //'function' : resolved overload was found by argument-dependent lookup
   //A function found by argument-dependent lookup (Koenig lookup) was eventually 
   //chosen by overload resolution.
   //
   //In Visual C++ .NET and earlier compilers, a different function would have 
   //been called. To pick the original function, use an explicitly qualified name.
   //
   #pragma warning (disable : 4675)
   #pragma warning (disable : 4996)
   #pragma warning (disable : 4503)
   #pragma warning (disable : 4284) // odd return type for operator->
   #pragma warning (disable : 4244) // possible loss of data
#endif

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>

#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))
#  pragma warn -8026 // shut up warning "cannot inline function because ..."
#  pragma warn -8027 // shut up warning "cannot inline function because ..."
#endif
