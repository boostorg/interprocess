#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>

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

   //warning C4275: non dll-interface class 'x' used as base for
   //dll-interface class 'Y'
   #pragma warning (disable : 4275)
   //warning C4251: 'x' : class 'y' needs to have dll-interface to
   //be used by clients of class 'z'
   #pragma warning (disable : 4251)
   #pragma warning (disable : 4675)
   #pragma warning (disable : 4996)
   #pragma warning (disable : 4503)
   #pragma warning (disable : 4284) // odd return type for operator->
   #pragma warning (disable : 4244) // possible loss of data
   #pragma warning (disable : 4521) ////Disable "multiple copy constructors specified"
   #pragma warning (disable : 4522)
   #pragma warning (disable : 4146)
   #pragma warning (disable : 4267) //conversion from 'X' to 'Y', possible loss of data
#endif
