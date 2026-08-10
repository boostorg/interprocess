//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2012
// (C) Copyright Markus Schoepflin 2007
// (C) Copyright Bryce Lelbach 2010
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_ATOMIC_HPP
#define BOOST_INTERPROCESS_DETAIL_ATOMIC_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/cstdint.hpp>

#if !defined(_AIX)
#define BOOST_INTERPROCESS_DETAIL_PPC_ASM_LABEL(label) label ":\n\t"
#define BOOST_INTERPROCESS_DETAIL_PPC_ASM_JUMP(insn, label, offset) insn " " label "\n\t"
#else
#define BOOST_INTERPROCESS_DETAIL_PPC_ASM_LABEL(label)
#define BOOST_INTERPROCESS_DETAIL_PPC_ASM_JUMP(insn, label, offset) insn " $" offset "\n\t"
#endif

namespace boost{
namespace interprocess{
namespace ipcdetail{

//! Atomically increment an boost::uint32_t by 1
//! "mem": pointer to the object
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_inc32(volatile boost::uint32_t *mem);

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

}  //namespace ipcdetail{
}  //namespace interprocess{
}  //namespace boost{

#if defined (BOOST_INTERPROCESS_WINDOWS)

#include <boost/interprocess/detail/win32_api.hpp>

//All the operations of this header are sequentially consistent. atomic_read32
//and atomic_write32 are implemented with one of these two mappings:
//
// - BOOST_INTERPROCESS_ATOMIC_LOAD32 / BOOST_INTERPROCESS_ATOMIC_STORE32, a
//   load-acquire/store-release pair. On ARMv8 this pair is sequentially
//   consistent (a load-acquire can't be reordered before a previous
//   store-release), so no explicit barrier is needed.
//
// - A plain load followed by BOOST_INTERPROCESS_READ_BARRIER, combined with
//   the full barrier interlocked exchange of atomic_write32. The full barrier
//   of the store is what makes the plain load sequentially consistent.
//
//Note that atomic_read32 must NOT be implemented with an interlocked
//(read-modify-write) operation, as it's also used on read-only mapped regions.
#if defined(__ATOMIC_SEQ_CST)
   //Clang (clang-cl included) and GCC 4.7 and later: the compiler emits the
   //optimal sequentially consistent load/store for the target
   #define BOOST_INTERPROCESS_ATOMIC_LOAD32(mem) __atomic_load_n((mem), __ATOMIC_SEQ_CST)
   #define BOOST_INTERPROCESS_ATOMIC_STORE32(mem, val) __atomic_store_n((mem), (val), __ATOMIC_SEQ_CST)
#elif defined( _MSC_VER )
   #if defined(_M_ARM64EC) || defined(_M_ARM64)
      //ARMv8 has load-acquire/store-release instructions, so no explicit
      //barrier and no interlocked operation are needed
      #include <intrin.h>
      #define BOOST_INTERPROCESS_ATOMIC_LOAD32(mem) \
                  (boost::uint32_t)__ldar32(reinterpret_cast<unsigned __int32 volatile *>(mem))
      #define BOOST_INTERPROCESS_ATOMIC_STORE32(mem, val) \
                  __stlr32(reinterpret_cast<unsigned __int32 volatile *>(mem), (unsigned __int32)(val))
   #elif defined(_M_ARM)
      //ARMv7 is weakly ordered and has no load-acquire instruction, so a real
      //hardware barrier is required after the load
      #include <intrin.h>
      #define BOOST_INTERPROCESS_READ_BARRIER __dmb(_ARM_BARRIER_ISH)
   #else
      //x86/x64 never reorders a load with the loads and stores that follow it,
      //so only the compiler must be kept in place.
      extern "C" void _ReadWriteBarrier(void);
      #pragma intrinsic(_ReadWriteBarrier)

      #define BOOST_INTERPROCESS_READ_BARRIER \
                  BOOST_INTERPROCESS_DISABLE_DEPRECATED_WARNING \
                  _ReadWriteBarrier() \
                  BOOST_INTERPROCESS_RESTORE_WARNING
   #endif
#elif defined(__GNUC__)
   //GCC 4.1 to 4.6, only the legacy __sync builtins are available
   #define BOOST_INTERPROCESS_READ_BARRIER __sync_synchronize()
#else
#  error "Unsupported Compiler for Window"
#endif

namespace boost{
namespace interprocess{
namespace ipcdetail{

//! Atomically decrement an boost::uint32_t by 1
//! "mem": pointer to the atomic value
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_dec32(volatile boost::uint32_t *mem)
{  return (boost::uint32_t)winapi::interlocked_decrement(reinterpret_cast<volatile long*>(mem)) + 1;  }

//! Atomically increment an apr_uint32_t by 1
//! "mem": pointer to the object
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_inc32(volatile boost::uint32_t *mem)
{  return (boost::uint32_t)winapi::interlocked_increment(reinterpret_cast<volatile long*>(mem))-1;  }

//! Atomically read an boost::uint32_t from memory
inline boost::uint32_t atomic_read32(volatile boost::uint32_t *mem)
{
   #if defined(BOOST_INTERPROCESS_ATOMIC_LOAD32)
   return BOOST_INTERPROCESS_ATOMIC_LOAD32(mem);
   #else
   const boost::uint32_t val = *mem;
   BOOST_INTERPROCESS_READ_BARRIER;
   return val;
   #endif
}

//! Atomically set an boost::uint32_t in memory
//! "mem": pointer to the object
//! "param": val value that the object will assume
inline void atomic_write32(volatile boost::uint32_t *mem, boost::uint32_t val)
{
   #if defined(BOOST_INTERPROCESS_ATOMIC_STORE32)
   BOOST_INTERPROCESS_ATOMIC_STORE32(mem, val);
   #else
   winapi::interlocked_exchange(reinterpret_cast<volatile long*>(mem), (long)val);
   #endif
}

//! Compare an boost::uint32_t's value with "cmp".
//! If they are the same swap the value with "with"
//! "mem": pointer to the value
//! "with": what to swap it with
//! "cmp": the value to compare it to
//! Returns the old value of *mem
inline boost::uint32_t atomic_cas32
   (volatile boost::uint32_t *mem, boost::uint32_t with, boost::uint32_t cmp)
{  return (boost::uint32_t)winapi::interlocked_compare_exchange(reinterpret_cast<volatile long*>(mem), (long)with, (long)cmp);  }

}  //namespace ipcdetail{
}  //namespace interprocess{
}  //namespace boost{

#elif defined(__GNUC__) && ( __GNUC__ * 100 + __GNUC_MINOR__ >= 401 )

namespace boost {
namespace interprocess {
namespace ipcdetail{

//! Atomically add 'val' to an boost::uint32_t
//! "mem": pointer to the object
//! "val": amount to add
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_add32
   (volatile boost::uint32_t *mem, boost::uint32_t val)
{  return __sync_fetch_and_add(const_cast<boost::uint32_t *>(mem), val);   }

//! Atomically increment an apr_uint32_t by 1
//! "mem": pointer to the object
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_inc32(volatile boost::uint32_t *mem)
{  return atomic_add32(mem, 1);  }

//! Atomically decrement an boost::uint32_t by 1
//! "mem": pointer to the atomic value
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_dec32(volatile boost::uint32_t *mem)
{  return atomic_add32(mem, (boost::uint32_t)-1);   }

//! Compare an boost::uint32_t's value with "cmp".
//! If they are the same swap the value with "with"
//! "mem": pointer to the value
//! "with" what to swap it with
//! "cmp": the value to compare it to
//! Returns the old value of *mem
inline boost::uint32_t atomic_cas32
   (volatile boost::uint32_t *mem, boost::uint32_t with, boost::uint32_t cmp)
{  return __sync_val_compare_and_swap(const_cast<boost::uint32_t *>(mem), cmp, with);   }

//! Atomically read an boost::uint32_t from memory
//! Note: this must NOT be a read-modify-write operation, as atomic_read32
//! is also used on read-only mapped regions
inline boost::uint32_t atomic_read32(volatile boost::uint32_t *mem)
{
   #if defined(__ATOMIC_SEQ_CST)
   //GCC 4.7 and later: a real atomic load, the compiler emits the optimal
   //sequentially consistent load for the target
   return __atomic_load_n(mem, __ATOMIC_SEQ_CST);
   #else
   const boost::uint32_t old_val = *mem; __sync_synchronize(); return old_val;
   #endif
}

//! Atomically set an boost::uint32_t in memory
//! "mem": pointer to the object
//! "param": val value that the object will assume
inline void atomic_write32(volatile boost::uint32_t *mem, boost::uint32_t val)
{
   #if defined(__ATOMIC_SEQ_CST)
   __atomic_store_n(mem, val, __ATOMIC_SEQ_CST);
   #else
   //The trailing fence is what gives StoreLoad ordering, needed to make
   //this store sequentially consistent
   __sync_synchronize(); *mem = val; __sync_synchronize();
   #endif
}

}  //namespace ipcdetail{
}  //namespace interprocess{
}  //namespace boost{

#else

#error No atomic operations implemented for this platform, sorry!

#endif

namespace boost{
namespace interprocess{
namespace ipcdetail{

inline bool atomic_add_unless32
   (volatile boost::uint32_t *mem, boost::uint32_t value, boost::uint32_t unless_this)
{
   boost::uint32_t old, c(atomic_read32(mem));
   while(c != unless_this && (old = atomic_cas32(mem, c + value, c)) != c){
      c = old;
   }
   return c != unless_this;
}

}  //namespace ipcdetail
}  //namespace interprocess
}  //namespace boost


#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_DETAIL_ATOMIC_HPP
