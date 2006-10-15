//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_ATOMIC_HPP
#define BOOST_INTERPROCESS_DETAIL_ATOMIC_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/cstdint.hpp>

#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

#  include <boost/interprocess/detail/win32_api.hpp>

namespace boost{
namespace interprocess{
namespace detail{

inline boost::uint32_t atomic_add32(volatile boost::uint32_t *mem, boost::uint32_t val)
{  return winapi::interlocked_exchange_add((volatile long*)mem, val);   }

inline void atomic_sub32(volatile boost::uint32_t *mem, boost::uint32_t val)
{  winapi::interlocked_exchange_add((volatile long*)mem, (boost::uint32_t)(-val)); }

inline boost::uint32_t atomic_inc32(volatile boost::uint32_t *mem)
{  return winapi::interlocked_increment((volatile long*)mem)-1;  }

inline boost::uint32_t atomic_dec32(volatile boost::uint32_t *mem)
{  return winapi::interlocked_decrement((volatile long*)mem)+1;  }

inline void atomic_write32(volatile boost::uint32_t *mem, boost::uint32_t val)
{  winapi::interlocked_exchange((volatile long*)mem, val);  }

inline boost::uint32_t atomic_read32(volatile boost::uint32_t *mem)
{  return *mem;   }

inline boost::uint32_t atomic_cas32
   (volatile boost::uint32_t *mem, boost::uint32_t with, boost::uint32_t cmp)
{  return winapi::interlocked_compare_exchange((volatile long*)mem, with, cmp);  }

inline boost::uint32_t atomic_xchg32(volatile boost::uint32_t *mem, boost::uint32_t val)
{  return winapi::interlocked_exchange((volatile long*)mem, val);  }

}  //namespace detail{
}  //namespace interprocess{
}  //namespace boost{

#elif defined(__GNUC__)

#include <stdlib.h>

namespace boost {
namespace interprocess {
namespace detail{

#if (defined(__i386__) || defined(__x86_64__))

static inline boost::uint32_t intel_atomic_add32
   (volatile boost::uint32_t *mem, boost::uint32_t val)
{
   asm volatile ("lock; xaddl %0,%1"
               : "=r"(val), "=m"(*mem) // outputs 
               : "0"(val), "m"(*mem)   // inputs
               : "memory", "cc");
   return val;
}

inline boost::uint32_t atomic_add32(volatile boost::uint32_t *mem, boost::uint32_t val)
{  return intel_atomic_add32(mem, val);   }

inline void atomic_sub32(volatile boost::uint32_t *mem, boost::uint32_t val)
{
   asm volatile ("lock; subl %1, %0"
               :
               : "m" (*(mem)), "r" (val)
               : "memory", "cc");
}

inline boost::uint32_t atomic_inc32(volatile boost::uint32_t *mem)
{  return intel_atomic_add32(mem, 1);  }

inline boost::uint32_t atomic_dec32(volatile boost::uint32_t *mem)
{
   unsigned char prev;

   asm volatile ("lock; decl %1;\n\t"
               "setnz %%al"
               : "=a" (prev)
               : "m" (*(mem))
               : "memory", "cc");
   return prev;
}

inline boost::uint32_t atomic_read32(volatile boost::uint32_t *mem)
{  return *mem;   }

inline boost::uint32_t atomic_cas32
   (volatile boost::uint32_t *mem, boost::uint32_t with, boost::uint32_t cmp)
{
   boost::uint32_t prev;
   asm volatile ("lock; cmpxchgl %1, %2"             
               : "=a" (prev)               
               : "r" (with), "m" (*(mem)), "0"(cmp) 
               : "memory", "cc");
   return prev;
}

inline boost::uint32_t atomic_xchg32(volatile boost::uint32_t *mem, boost::uint32_t val)
{
   boost::uint32_t prev = val;
   asm volatile ("lock; xchgl %0, %1"
               : "=r" (prev)
               : "m" (*(mem)), "0"(prev)
               : "memory");
   return prev;
}

inline void atomic_write32(volatile boost::uint32_t *mem, boost::uint32_t val)
{  atomic_xchg32(mem, val); }

#elif (defined(__PPC__) || defined(__ppc__))

inline boost::uint32_t atomic_add32(volatile boost::uint32_t *mem, boost::uint32_t val)
{
   boost::uint32_t prev, temp;

   asm volatile ("0:\n\t"                 // retry local label     
               "lwarx  %0,0,%2\n\t"       // load prev and reserve 
               "add    %1,%0,%3\n\t"      // temp = prev + delta   
               "stwcx. %1,0,%2\n\t"       // conditionally store   
               "bne-   0b"                // start over if we lost
                                          // the reservation
               //XXX find a cleaner way to define the temp         
               //it's not an output
               : "=&r" (prev), "=&r" (temp)        // output, temp 
               : "b" (mem), "r" (delta)            // inputs       
               : "memory", "cc");                  // clobbered    
   return prev;
}

inline boost::uint32_t atomic_inc32(volatile boost::uint32_t *mem)
{  return atomic_inc32(mem, 1);  }

inline boost::uint32_t atomic_read32(volatile boost::uint32_t *mem)
{  return *mem;   }

inline boost::uint32_t atomic_cas32
   (volatile boost::uint32_t *mem, boost::uint32_t with, boost::uint32_t cmp)
{
   boost::uint32_t prev;
                                                                              
   asm volatile ("0:\n\t"                   // retry local label     
               "lwarx  %0,0,%1\n\t"       // load prev and reserve 
               "cmpw   %0,%3\n\t"         // does it match cmp?    
               "bne-   1f\n\t"            // ...no, bail out       
               "stwcx. %2,0,%1\n\t"       // ...yes, conditionally
                                             store swap            
               "bne-   0b\n\t"            // start over if we lost
                                             the reservation       
               "1:"                       // exit local label      

               : "=&r"(prev)                        // output      
               : "b" (mem), "r" (swap), "r"(cmp)    // inputs      
               : "memory", "cc");                   // clobbered   
   return prev;
}

inline void atomic_sub32(volatile boost::uint32_t *mem, boost::uint32_t val)
{
   boost::uint32_t old_value, new_value;
   do {
      old_value = *mem;
      new_value = old_value - val;
   } while (atomic_cas32(mem, new_value, old_value) != old_value);
}

inline boost::uint32_t atomic_dec32(volatile boost::uint32_t *mem)
{  return atomic_sub32(mem, 1);  }

inline boost::uint32_t atomic_xchg32(volatile boost::uint32_t *mem, boost::uint32_t val)
{
   boost::uint32_t prev;
   do {
      prev = *mem;
   } while (atomic_cas32(mem, val, prev) != prev);
   return prev;
}

inline void atomic_write32(volatile boost::uint32_t *mem, boost::uint32_t val)
{  atomic_xchg32(mem, val); }

#else

#error No atomic operations implemented for this platform, sorry!

#endif

}  //namespace detail{
}  //namespace interprocess{
}  //namespace boost{

#else

#error No atomic operations implemented for this platform, sorry!

#endif

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_DETAIL_ATOMIC_HPP
