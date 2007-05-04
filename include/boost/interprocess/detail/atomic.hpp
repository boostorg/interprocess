//////////////////////////////////////////////////////////////////////////////
//! Copyright 2000-2005 The Apache Software Foundation or its licensors, as
//! applicable.
//!
//! Licensed under the Apache License, Version 2.0 (the "License");
//! you may not use this file except in compliance with the License.
//! You may obtain a copy of the License at
//!
//!     http://www.apache.org/licenses/LICENSE-2.0
//!
//! Unless required by applicable law or agreed to in writing, software
//! distributed under the License is distributed on an "AS IS" BASIS,
//! WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//! See the License for the specific language governing permissions and
//! limitations under the License.
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

//! Atomically add 'val' to an boost::uint32_t
//! "mem": pointer to the object
//! "val": amount to add
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_add32(volatile boost::uint32_t *mem, boost::uint32_t val);

//! Atomically subtract 'val' from an apr_uint32_t
//! "mem": pointer to the object
//! "val": amount to subtract
inline void atomic_sub32(volatile boost::uint32_t *mem, boost::uint32_t val);

//! Atomically increment an apr_uint32_t by 1
//! "mem": pointer to the object
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_inc32(volatile boost::uint32_t *mem);

//! Atomically decrement an boost::uint32_t by 1
//! "mem": pointer to the atomic value
//! Returns false if the value becomes zero on decrement, otherwise true
inline bool atomic_dec32(volatile boost::uint32_t *mem);

//! Atomically read an boost::uint32_t from memory
inline boost::uint32_t atomic_read32(volatile boost::uint32_t *mem);

//! Atomically set an boost::uint32_t in memory
//! "mem": pointer to the object
//! "param": val value that the object will assume
inline void atomic_write32(volatile boost::uint32_t *mem, boost::uint32_t val);

//! Compare an boost::uint32_t's value with "cmp".
//! If they are the same swap the value with "with"
//! "mem": pointer to the value
//! "with": what to swap it with
//! "cmp": the value to compare it to
//! Returns the old value of *mem
inline boost::uint32_t atomic_cas32
   (volatile boost::uint32_t *mem, boost::uint32_t with, boost::uint32_t cmp);

//! Exchange an boost::uint32_t's value with "val".
//! "mem": pointer to the value
//! "val": what to swap it with
//! Returns the old value of *mem
inline boost::uint32_t atomic_xchg32(volatile boost::uint32_t *mem, boost::uint32_t val);

/*
//! Compare the pointer's value with "cmp".
//! If they are the same swap the value with "with"
//! "mem": pointer to the pointer
//! "with": what to swap it with
//! "cmp": the value to compare it to
//! Returns the old value of the pointer
inline void *atomic_casptr(volatile void **mem, void *with, const void *cmp);
*/

#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

#  include <boost/interprocess/detail/win32_api.hpp>

namespace boost{
namespace interprocess{
namespace detail{

//! Atomically add 'val' to an boost::uint32_t
//! "mem": pointer to the object
//! "val": amount to add
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_add32(volatile boost::uint32_t *mem, boost::uint32_t val)
{  return winapi::interlocked_exchange_add((volatile long*)mem, val);   }

//! Atomically subtract 'val' from an apr_uint32_t
//! "mem": pointer to the object
//! "val": amount to subtract
inline void atomic_sub32(volatile boost::uint32_t *mem, boost::uint32_t val)
{  winapi::interlocked_exchange_add((volatile long*)mem, (boost::uint32_t)(-val)); }

//! Atomically increment an apr_uint32_t by 1
//! "mem": pointer to the object
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_inc32(volatile boost::uint32_t *mem)
{  return winapi::interlocked_increment((volatile long*)mem)-1;  }

//! Atomically decrement an boost::uint32_t by 1
//! "mem": pointer to the atomic value
//! Returns false if the value becomes zero on decrement, otherwise true
inline bool atomic_dec32(volatile boost::uint32_t *mem)
{  return winapi::interlocked_decrement((volatile long*)mem) != 0;  }

//! Atomically read an boost::uint32_t from memory
inline boost::uint32_t atomic_read32(volatile boost::uint32_t *mem)
{  return *mem;   }

//! Atomically set an boost::uint32_t in memory
//! "mem": pointer to the object
//! "param": val value that the object will assume
inline void atomic_write32(volatile boost::uint32_t *mem, boost::uint32_t val)
{  winapi::interlocked_exchange((volatile long*)mem, val);  }

//! Compare an boost::uint32_t's value with "cmp".
//! If they are the same swap the value with "with"
//! "mem": pointer to the value
//! "with": what to swap it with
//! "cmp": the value to compare it to
//! Returns the old value of *mem
inline boost::uint32_t atomic_cas32
   (volatile boost::uint32_t *mem, boost::uint32_t with, boost::uint32_t cmp)
{  return winapi::interlocked_compare_exchange((volatile long*)mem, with, cmp);  }

//! Exchange an boost::uint32_t's value with "val".
//! "mem": pointer to the value
//! "val": what to swap it with
//! Returns the old value of *mem
inline boost::uint32_t atomic_xchg32(volatile boost::uint32_t *mem, boost::uint32_t val)
{  return winapi::interlocked_exchange((volatile long*)mem, val);  }
/*
//! Compare the pointer's value with "cmp".
//! If they are the same swap the value with "with"
//! "mem": pointer to the pointer
//! "with": what to swap it with
//! "cmp": the value to compare it to
//! Returns the old value of the pointer
inline void *atomic_casptr(volatile void **mem, void *with, const void *cmp);
{  return winapi::interlocked_compare_exchange_pointer(mem, with, cmp); }
*/
}  //namespace detail{
}  //namespace interprocess{
}  //namespace boost{

#elif defined(__GNUC__)

#include <stdlib.h>

namespace boost {
namespace interprocess {
namespace detail{

#if (defined(__i386__) || defined(__x86_64__))

static boost::uint32_t inline intel_atomic_add32
   (volatile boost::uint32_t *mem, boost::uint32_t val)
{
   asm volatile ("lock; xaddl %0,%1"
               : "=r"(val), "=m"(*mem) /* outputs */
               : "0"(val), "m"(*mem)   /* inputs */
               : "memory", "cc");
   return val;
}

//! Atomically add 'val' to an boost::uint32_t
//! "mem": pointer to the object
//! "val": amount to add
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_add32
   (volatile boost::uint32_t *mem, boost::uint32_t val)
{  return intel_atomic_add32(mem, val);   }

//! Atomically subtract 'val' from an apr_uint32_t
//! "mem": pointer to the object
//! "val": amount to subtract
inline void atomic_sub32(volatile boost::uint32_t *mem, boost::uint32_t val)
{
   asm volatile ("lock; subl %1, %0"
               :
               : "m" (*(mem)), "r" (val)
               : "memory", "cc");
}

//! Atomically increment an apr_uint32_t by 1
//! "mem": pointer to the object
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_inc32(volatile boost::uint32_t *mem)
{  return intel_atomic_add32(mem, 1);  }

//! Atomically decrement an boost::uint32_t by 1
//! "mem": pointer to the atomic value
//! Returns false if the value becomes zero on decrement, otherwise true
inline bool atomic_dec32(volatile boost::uint32_t *mem)
{
   unsigned char prev;

   asm volatile ("lock; decl %1;\n\t"
               "setnz %%al"
               : "=a" (prev)
               : "m" (*(mem))
               : "memory", "cc");
   return prev != 0;
}

//! Atomically read an boost::uint32_t from memory
inline boost::uint32_t atomic_read32(volatile boost::uint32_t *mem)
{  return *mem;   }

//! Compare an boost::uint32_t's value with "cmp".
//! If they are the same swap the value with "with"
//! "mem": pointer to the value
//! "with" what to swap it with
//! "cmp": the value to compare it to
//! Returns the old value of *mem
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

//! Exchange an boost::uint32_t's value with "val".
//! "mem": pointer to the value
//! "val": what to swap it with
//! Returns the old value of *mem
inline boost::uint32_t atomic_xchg32(volatile boost::uint32_t *mem, boost::uint32_t val)
{
   boost::uint32_t prev = val;

   asm volatile ("lock; xchgl %0, %1"
               : "=r" (prev)
               : "m" (*(mem)), "0"(prev)
               : "memory");
   return prev;
}

//! Atomically set an boost::uint32_t in memory
//! "mem": pointer to the object
//! "param": val value that the object will assume
inline void atomic_write32(volatile boost::uint32_t *mem, boost::uint32_t val)
{  *mem = val; }

/*
//! Compare the pointer's value with "cmp".
//! If they are the same swap the value with "with"
//! "mem": pointer to the pointer
//! "with": what to swap it with
//! "cmp": the value to compare it to
//! Returns the old value of the pointer
inline void *atomic_casptr(volatile void **mem, void *with, const void *cmp);
{
}
*/
#elif (defined(__PPC__) || defined(__ppc__))

//! Atomically add 'val' to an boost::uint32_t
//! "mem": pointer to the object
//! "val": amount to add
//! Returns the old value pointed to by mem
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

//! Compare an boost::uint32_t's value with "cmp".
//! If they are the same swap the value with "with"
//! "mem": pointer to the value
//! "with" what to swap it with
//! "cmp": the value to compare it to
//! Returns the old value of *mem
inline boost::uint32_t atomic_cas32
   (volatile boost::uint32_t *mem, boost::uint32_t with, boost::uint32_t cmp)
{
   boost::uint32_t prev;
                                                                              
   asm volatile ("0:\n\t"                 // retry local label     
               "lwarx  %0,0,%1\n\t"       // load prev and reserve 
               "cmpw   %0,%3\n\t"         // does it match cmp?    
               "bne-   1f\n\t"            // ...no, bail out       
               "stwcx. %2,0,%1\n\t"       // ...yes, conditionally
                                          //   store swap            
               "bne-   0b\n\t"            // start over if we lost
                                          //   the reservation       
               "1:"                       // exit local label      

               : "=&r"(prev)                        // output      
               : "b" (mem), "r" (swap), "r"(cmp)    // inputs      
               : "memory", "cc");                   // clobbered   
   return prev;
}

//! Atomically subtract 'val' from an apr_uint32_t
//! "mem": pointer to the object
//! "val": amount to subtract
inline void atomic_sub32(volatile boost::uint32_t *mem, boost::uint32_t val)
{
   boost::uint32_t old_value, new_value;
   
   do {
      old_value = *mem;
      new_value = old_value - val;
   } while (atomic_cas32(mem, new_value, old_value) != old_value);
}

//! Atomically increment an apr_uint32_t by 1
//! "mem": pointer to the object
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_inc32(volatile boost::uint32_t *mem)
{  return atomic_add32(mem, 1);  }

//! Atomically decrement an boost::uint32_t by 1
//! "mem": pointer to the atomic value
//! Returns false if the value becomes zero on decrement, otherwise true
inline bool atomic_dec32(volatile boost::uint32_t *mem)
{
    boost::uint32_t old_value, new_value;
    
    do {
        old_value = *mem;
        new_value = old_value - 1;
    } while (atomic_cas32(mem, new_value, old_value) != old_value);
    return old_value != 1;
}

//! Atomically read an boost::uint32_t from memory
inline boost::uint32_t atomic_read32(volatile boost::uint32_t *mem)
{  return *mem;   }

//! Exchange an boost::uint32_t's value with "val".
//! "mem": pointer to the value
//! "val": what to swap it with
//! Returns the old value of *mem
inline boost::uint32_t atomic_xchg32(volatile boost::uint32_t *mem, boost::uint32_t val)
{
   boost::uint32_t prev;
   do {
      prev = *mem;
   } while (atomic_cas32(mem, val, prev) != prev);
   return prev;
}

//! Atomically set an boost::uint32_t in memory
//! "mem": pointer to the object
//! "param": val value that the object will assume
inline void atomic_write32(volatile boost::uint32_t *mem, boost::uint32_t val)
{  atomic_xchg32(mem, val); }

/*
//! Compare the pointer's value with "cmp".
//! If they are the same swap the value with "with"
//! "mem": pointer to the pointer
//! "with": what to swap it with
//! "cmp": the value to compare it to
//! Returns the old value of the pointer
inline void *atomic_casptr(volatile void **mem, void *with, const void *cmp);
{
}
*/
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
