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


namespace boost{
namespace interprocess{
namespace ipcdetail{

//! Atomically add "val" to an boost::uint32_t
//! "mem": pointer to the object
//! "val": value to add
//! Returns the old value pointed to by mem
//!
//! The addition is modulo 2^32, so subtracting is adding the two's complement
//! and no overflow check is performed
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_add32(volatile boost::uint32_t *mem, boost::uint32_t val);

//! Same as atomic_add32, but with relaxed semantics: only the addition itself
//! is atomic, nothing is ordered around it. Enough when the value is a plain
//! count that publishes nothing, like taking a new reference to an object that
//! the caller already owns a reference to.
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_add32_relaxed(volatile boost::uint32_t *mem, boost::uint32_t val);

//! Same as atomic_add32, but with release semantics: everything done before it
//! is visible to whoever acquires the value afterwards. This is what handing
//! work over needs, like posting to a semaphore.
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_add32_release(volatile boost::uint32_t *mem, boost::uint32_t val);

//! Atomically subtract "val" from an boost::uint32_t
//! "mem": pointer to the atomic value
//! "val": value to subtract
//! Returns the old value pointed to by mem
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_sub32(volatile boost::uint32_t *mem, boost::uint32_t val);

//! Same as atomic_sub32, but with relaxed semantics: only the subtraction
//! itself is atomic, nothing is ordered around it.
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_sub32_relaxed(volatile boost::uint32_t *mem, boost::uint32_t val);

//! Same as atomic_sub32, but with release semantics: everything done before it
//! is visible to whoever acquires the value afterwards. This is what dropping
//! a reference needs, so that the thread destroying the object sees all the
//! work of the threads that released their references before.
//!
//! Note that a reference count also needs the destroying thread to acquire
//! what the others released. As this header has no standalone fence, the last
//! subtraction, the one that sees a previous value of 1, must be followed by an
//! acquire operation on the same variable, or use atomic_sub32 instead.
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_sub32_release(volatile boost::uint32_t *mem, boost::uint32_t val);

//! Atomically read an boost::uint32_t from memory
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_read32(volatile boost::uint32_t *mem);

//! Atomically read an boost::uint32_t from memory with acquire semantics
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_read32_acquire(volatile boost::uint32_t *mem);

//! Atomically set an boost::uint32_t in memory
//! "mem": pointer to the object
//! "param": val value that the object will assume
BOOST_INTERPROCESS_FORCEINLINE void atomic_write32(volatile boost::uint32_t *mem, boost::uint32_t val);

//! Atomically set an boost::uint32_t in memory with release semantics
//! "mem": pointer to the object
//! "param": val value that the object will assume
BOOST_INTERPROCESS_FORCEINLINE void atomic_write32_release(volatile boost::uint32_t *mem, boost::uint32_t val);

//! Compare an boost::uint32_t's value with "cmp".
//! If they are the same swap the value with "with"
//! "mem": pointer to the value
//! "with": what to swap it with
//! "cmp": the value to compare it to
//! Returns the old value of *mem
//!
//! This is a strong compare and swap
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_cas32
   (volatile boost::uint32_t *mem, boost::uint32_t with, boost::uint32_t cmp);

//! Same as atomic_cas32, but the operation is ordered with acquire semantics
//! when the swap succeeds. The failure case is only guaranteed to be relaxed
//! and callers must not rely on any ordering there.
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_cas32_acquire
   (volatile boost::uint32_t *mem, boost::uint32_t with, boost::uint32_t cmp);

//! Same as atomic_cas32, but the operation is ordered with release semantics
//! when the swap succeeds. A failed compare is only guaranteed to be relaxed
//! and callers must not rely on any ordering there.
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_cas32_release
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
   //optimal load/store for the target and the requested memory order
   #define BOOST_INTERPROCESS_ATOMIC_LOAD32(mem)           __atomic_load_n((mem), __ATOMIC_SEQ_CST)
   #define BOOST_INTERPROCESS_ATOMIC_LOAD32_ACQ(mem)       __atomic_load_n((mem), __ATOMIC_ACQUIRE)
   #define BOOST_INTERPROCESS_ATOMIC_STORE32(mem, val)     __atomic_store_n((mem), (val), __ATOMIC_SEQ_CST)
   #define BOOST_INTERPROCESS_ATOMIC_STORE32_REL(mem, val) __atomic_store_n((mem), (val), __ATOMIC_RELEASE)
#elif defined( _MSC_VER )
   #if defined(_M_ARM64EC) || defined(_M_ARM64)
      //ARMv8 has load-acquire/store-release instructions, so no explicit
      //barrier and no interlocked operation are needed. As the pair is also
      //sequentially consistent, the same instruction serves both orders.
      #include <intrin.h>
      #define BOOST_INTERPROCESS_ATOMIC_LOAD32(mem) \
                  (boost::uint32_t)__ldar32(reinterpret_cast<unsigned __int32 volatile *>(mem))
      #define BOOST_INTERPROCESS_ATOMIC_STORE32(mem, val) \
                  __stlr32(reinterpret_cast<unsigned __int32 volatile *>(mem), (unsigned __int32)(val))
      #define BOOST_INTERPROCESS_ATOMIC_LOAD32_ACQ(mem)       BOOST_INTERPROCESS_ATOMIC_LOAD32(mem)
      #define BOOST_INTERPROCESS_ATOMIC_STORE32_REL(mem, val) BOOST_INTERPROCESS_ATOMIC_STORE32(mem, val)
   #elif defined(_M_ARM)
      //ARMv7 is weakly ordered and has no load-acquire/store-release
      //instructions, so real hardware barriers are required
      #include <intrin.h>
      #define BOOST_INTERPROCESS_READ_BARRIER  __dmb(_ARM_BARRIER_ISH)
      #define BOOST_INTERPROCESS_WRITE_BARRIER __dmb(_ARM_BARRIER_ISH)
   #else
      //x86/x64 never reorders a load with the loads and stores that follow it,
      //nor a store with the loads and stores that precede it, so only the
      //compiler must be kept in place.
      extern "C" void _ReadWriteBarrier(void);
      #pragma intrinsic(_ReadWriteBarrier)

      #define BOOST_INTERPROCESS_READ_BARRIER \
                  BOOST_INTERPROCESS_DISABLE_DEPRECATED_WARNING \
                  _ReadWriteBarrier() \
                  BOOST_INTERPROCESS_RESTORE_WARNING
      #define BOOST_INTERPROCESS_WRITE_BARRIER BOOST_INTERPROCESS_READ_BARRIER
   #endif
#elif defined(__GNUC__)
   //GCC 4.1 to 4.6, only the legacy __sync builtins are available
   #define BOOST_INTERPROCESS_READ_BARRIER  __sync_synchronize()
   #define BOOST_INTERPROCESS_WRITE_BARRIER __sync_synchronize()
#else
#  error "Unsupported Compiler for Window"
#endif

namespace boost{
namespace interprocess{
namespace ipcdetail{

//! Atomically add "val" to an boost::uint32_t
//! "mem": pointer to the object
//! "val": value to add
//! Returns the old value pointed to by mem
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_add32(volatile boost::uint32_t *mem, boost::uint32_t val)
{  return (boost::uint32_t)winapi::interlocked_exchange_add(reinterpret_cast<volatile long*>(mem), (long)val);  }

//! Same as atomic_add32, but with relaxed semantics
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_add32_relaxed(volatile boost::uint32_t *mem, boost::uint32_t val)
{
   #if defined(__ATOMIC_RELAXED)
   return __atomic_fetch_add(mem, val, __ATOMIC_RELAXED);
   #elif defined(_M_ARM64EC) || defined(_M_ARM64) || defined(_M_ARM)
   //ARM has unordered interlocked operations, cheaper than the full barrier
   //ones used by atomic_add32
   return (boost::uint32_t)_InterlockedExchangeAdd_nf(reinterpret_cast<volatile long*>(mem), (long)val);
   #else
   //x86/x64 has no weaker atomic read-modify-write instruction, LOCK XADD is
   //always a full barrier
   return atomic_add32(mem, val);
   #endif
}

//! Same as atomic_add32, but with release semantics
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_add32_release(volatile boost::uint32_t *mem, boost::uint32_t val)
{
   #if defined(__ATOMIC_RELEASE)
   return __atomic_fetch_add(mem, val, __ATOMIC_RELEASE);
   #elif defined(_M_ARM64EC) || defined(_M_ARM64) || defined(_M_ARM)
   return (boost::uint32_t)_InterlockedExchangeAdd_rel(reinterpret_cast<volatile long*>(mem), (long)val);
   #else
   return atomic_add32(mem, val);
   #endif
}

//! Atomically subtract "val" from an boost::uint32_t
//! "mem": pointer to the atomic value
//! "val": value to subtract
//! Returns the old value pointed to by mem
//!
//! Subtracting is adding the two's complement, the operation is modulo 2^32
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_sub32(volatile boost::uint32_t *mem, boost::uint32_t val)
{  return atomic_add32(mem, boost::uint32_t(0u) - val);  }

//! Same as atomic_sub32, but with relaxed semantics
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_sub32_relaxed(volatile boost::uint32_t *mem, boost::uint32_t val)
{
   #if defined(__ATOMIC_RELAXED)
   return __atomic_fetch_sub(mem, val, __ATOMIC_RELAXED);
   #else
   return atomic_add32_relaxed(mem, boost::uint32_t(0u) - val);
   #endif
}

//! Same as atomic_sub32, but with release semantics
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_sub32_release(volatile boost::uint32_t *mem, boost::uint32_t val)
{
   #if defined(__ATOMIC_RELEASE)
   return __atomic_fetch_sub(mem, val, __ATOMIC_RELEASE);
   #else
   return atomic_add32_release(mem, boost::uint32_t(0u) - val);
   #endif
}

//! Atomically read an boost::uint32_t from memory
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_read32(volatile boost::uint32_t *mem)
{
   #if defined(BOOST_INTERPROCESS_ATOMIC_LOAD32)
   return BOOST_INTERPROCESS_ATOMIC_LOAD32(mem);
   #else
   const boost::uint32_t val = *mem;
   BOOST_INTERPROCESS_READ_BARRIER;
   return val;
   #endif
}

//! Atomically read an boost::uint32_t from memory with acquire semantics
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_read32_acquire(volatile boost::uint32_t *mem)
{
   #if defined(BOOST_INTERPROCESS_ATOMIC_LOAD32_ACQ)
   return BOOST_INTERPROCESS_ATOMIC_LOAD32_ACQ(mem);
   #else
   const boost::uint32_t val = *mem;
   BOOST_INTERPROCESS_READ_BARRIER;
   return val;
   #endif
}

//! Atomically set an boost::uint32_t in memory
//! "mem": pointer to the object
//! "param": val value that the object will assume
BOOST_INTERPROCESS_FORCEINLINE void atomic_write32(volatile boost::uint32_t *mem, boost::uint32_t val)
{
   #if defined(BOOST_INTERPROCESS_ATOMIC_STORE32)
   BOOST_INTERPROCESS_ATOMIC_STORE32(mem, val);
   #else
   winapi::interlocked_exchange(reinterpret_cast<volatile long*>(mem), (long)val);
   #endif
}

//! Atomically set an boost::uint32_t in memory with release semantics
//! "mem": pointer to the object
//! "param": val value that the object will assume
BOOST_INTERPROCESS_FORCEINLINE void atomic_write32_release(volatile boost::uint32_t *mem, boost::uint32_t val)
{
   #if defined(BOOST_INTERPROCESS_ATOMIC_STORE32_REL)
   BOOST_INTERPROCESS_ATOMIC_STORE32_REL(mem, val);
   #else
   BOOST_INTERPROCESS_WRITE_BARRIER;
   *mem = val;
   #endif
}

//! Compare an boost::uint32_t's value with "cmp".
//! If they are the same swap the value with "with"
//! "mem": pointer to the value
//! "with": what to swap it with
//! "cmp": the value to compare it to
//! Returns the old value of *mem
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_cas32
   (volatile boost::uint32_t *mem, boost::uint32_t with, boost::uint32_t cmp)
{  return (boost::uint32_t)winapi::interlocked_compare_exchange(reinterpret_cast<volatile long*>(mem), (long)with, (long)cmp);  }

//! Same as atomic_cas32, but with acquire semantics for the success case
//! and relaxed semantics for the failure case
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_cas32_acquire
   (volatile boost::uint32_t *mem, boost::uint32_t with, boost::uint32_t cmp)
{
   #if defined(__ATOMIC_ACQUIRE)
   //On failure "cmp" receives the current value and on success it is left
   //untouched, already holding the old value, so it is the old value in both
   //cases, just like the value returned by atomic_cas32.
   //A failed compare and swap performs no store, so nothing has to be ordered
   //in that case and the cheapest failure order is used
   //The "false" argument requests a strong compare and swap, which this
   //interface requires: see the note in the declaration of atomic_cas32
   __atomic_compare_exchange_n(mem, &cmp, with, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
   return cmp;
   #elif defined(_M_ARM64EC) || defined(_M_ARM64) || defined(_M_ARM)
   //ARM has acquire/release interlocked operations, cheaper than the full
   //barrier ones used by atomic_cas32. This intrinsic takes a single memory
   //order, so it also orders the failure case: stronger than promised, which
   //callers must not depend on
   return (boost::uint32_t)_InterlockedCompareExchange_acq
      (reinterpret_cast<volatile long*>(mem), (long)with, (long)cmp);
   #else
   //x86/x64 has no weaker atomic read-modify-write instruction, LOCK CMPXCHG
   //is always a full barrier
   return atomic_cas32(mem, with, cmp);
   #endif
}

//! Same as atomic_cas32, but with release semantics with the success case
//! and relaxed semantics for the failure case
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_cas32_release
   (volatile boost::uint32_t *mem, boost::uint32_t with, boost::uint32_t cmp)
{
   #if defined(__ATOMIC_RELEASE)
   //A failed compare and swap performs no store, so its memory order can't be
   //release: relaxed is the strongest order allowed for the failure case
   //The "false" argument requests a strong compare and swap, which this
   //interface requires: see the note in the declaration of atomic_cas32
   __atomic_compare_exchange_n(mem, &cmp, with, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED);
   return cmp;
   #elif defined(_M_ARM64EC) || defined(_M_ARM64) || defined(_M_ARM)
   return (boost::uint32_t)_InterlockedCompareExchange_rel
      (reinterpret_cast<volatile long*>(mem), (long)with, (long)cmp);
   #else
   return atomic_cas32(mem, with, cmp);
   #endif
}

}  //namespace ipcdetail{
}  //namespace interprocess{
}  //namespace boost{

#elif defined(__GNUC__) && ( __GNUC__ * 100 + __GNUC_MINOR__ >= 401 )

namespace boost {
namespace interprocess {
namespace ipcdetail{

//! Atomically add "val" to an boost::uint32_t
//! "mem": pointer to the object
//! "val": value to add
//! Returns the old value pointed to by mem
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_add32(volatile boost::uint32_t *mem, boost::uint32_t val)
{  return __sync_fetch_and_add(const_cast<boost::uint32_t *>(mem), val);   }

//! Atomically subtract "val" from an boost::uint32_t
//! "mem": pointer to the atomic value
//! "val": value to subtract
//! Returns the old value pointed to by mem
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_sub32(volatile boost::uint32_t *mem, boost::uint32_t val)
{  return __sync_fetch_and_sub(const_cast<boost::uint32_t *>(mem), val);   }

//! Same as atomic_add32, but with relaxed semantics
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_add32_relaxed(volatile boost::uint32_t *mem, boost::uint32_t val)
{
   #if defined(__ATOMIC_RELAXED)
   return __atomic_fetch_add(mem, val, __ATOMIC_RELAXED);
   #else
   return atomic_add32(mem, val);
   #endif
}

//! Same as atomic_add32, but with release semantics
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_add32_release(volatile boost::uint32_t *mem, boost::uint32_t val)
{
   #if defined(__ATOMIC_RELEASE)
   return __atomic_fetch_add(mem, val, __ATOMIC_RELEASE);
   #else
   return atomic_add32(mem, val);
   #endif
}

//! Same as atomic_sub32, but with relaxed semantics
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_sub32_relaxed(volatile boost::uint32_t *mem, boost::uint32_t val)
{
   #if defined(__ATOMIC_RELAXED)
   return __atomic_fetch_sub(mem, val, __ATOMIC_RELAXED);
   #else
   return atomic_sub32(mem, val);
   #endif
}

//! Same as atomic_sub32, but with release semantics
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_sub32_release(volatile boost::uint32_t *mem, boost::uint32_t val)
{
   #if defined(__ATOMIC_RELEASE)
   return __atomic_fetch_sub(mem, val, __ATOMIC_RELEASE);
   #else
   return atomic_sub32(mem, val);
   #endif
}

//! Compare an boost::uint32_t's value with "cmp".
//! If they are the same swap the value with "with"
//! "mem": pointer to the value
//! "with" what to swap it with
//! "cmp": the value to compare it to
//! Returns the old value of *mem
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_cas32
   (volatile boost::uint32_t *mem, boost::uint32_t with, boost::uint32_t cmp)
{  return __sync_val_compare_and_swap(const_cast<boost::uint32_t *>(mem), cmp, with);   }

//! Same as atomic_cas32, but with acquire semantics
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_cas32_acquire
   (volatile boost::uint32_t *mem, boost::uint32_t with, boost::uint32_t cmp)
{
   #if defined(__ATOMIC_ACQUIRE)
   //On failure "cmp" receives the current value and on success it is left
   //untouched, already holding the old value, so it is the old value in both
   //cases, just like the value returned by atomic_cas32.
   //A failed compare and swap performs no store, so nothing has to be ordered
   //in that case and the cheapest failure order is used
   //The "false" argument requests a strong compare and swap, which this
   //interface requires: see the note in the declaration of atomic_cas32
   __atomic_compare_exchange_n(mem, &cmp, with, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
   return cmp;
   #else
   return __sync_val_compare_and_swap(const_cast<boost::uint32_t *>(mem), cmp, with);
   #endif
}

//! Same as atomic_cas32, but with release semantics
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_cas32_release
   (volatile boost::uint32_t *mem, boost::uint32_t with, boost::uint32_t cmp)
{
   #if defined(__ATOMIC_RELEASE)
   //A failed compare and swap performs no store, so its memory order can't be
   //release: relaxed is the strongest order allowed for the failure case
   //The "false" argument requests a strong compare and swap, which this
   //interface requires: see the note in the declaration of atomic_cas32
   __atomic_compare_exchange_n(mem, &cmp, with, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED);
   return cmp;
   #else
   return __sync_val_compare_and_swap(const_cast<boost::uint32_t *>(mem), cmp, with);
   #endif
}

//! Atomically read an boost::uint32_t from memory
//! Note: this must NOT be a read-modify-write operation, as atomic_read32
//! is also used on read-only mapped regions
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_read32(volatile boost::uint32_t *mem)
{
   #if defined(__ATOMIC_SEQ_CST)
   //GCC 4.7 and later: a real atomic load, the compiler emits the optimal
   //sequentially consistent load for the target
   return __atomic_load_n(mem, __ATOMIC_SEQ_CST);
   #else
   const boost::uint32_t old_val = *mem; __sync_synchronize(); return old_val;
   #endif
}

//! Atomically read an boost::uint32_t from memory with acquire semantics
//! Note: this must NOT be a read-modify-write operation, as it is also
//! used on read-only mapped regions
BOOST_INTERPROCESS_FORCEINLINE boost::uint32_t atomic_read32_acquire(volatile boost::uint32_t *mem)
{
   #if defined(__ATOMIC_ACQUIRE)
   return __atomic_load_n(mem, __ATOMIC_ACQUIRE);
   #else
   const boost::uint32_t old_val = *mem; __sync_synchronize(); return old_val;
   #endif
}

//! Atomically set an boost::uint32_t in memory
//! "mem": pointer to the object
//! "param": val value that the object will assume
BOOST_INTERPROCESS_FORCEINLINE void atomic_write32(volatile boost::uint32_t *mem, boost::uint32_t val)
{
   #if defined(__ATOMIC_SEQ_CST)
   __atomic_store_n(mem, val, __ATOMIC_SEQ_CST);
   #else
   //The trailing fence is what gives StoreLoad ordering, needed to make
   //this store sequentially consistent
   __sync_synchronize(); *mem = val; __sync_synchronize();
   #endif
}

//! Atomically set an boost::uint32_t in memory with release semantics
//! "mem": pointer to the object
//! "param": val value that the object will assume
BOOST_INTERPROCESS_FORCEINLINE void atomic_write32_release(volatile boost::uint32_t *mem, boost::uint32_t val)
{
   #if defined(__ATOMIC_RELEASE)
   __atomic_store_n(mem, val, __ATOMIC_RELEASE);
   #else
   //No trailing fence: a release store does not need StoreLoad ordering
   __sync_synchronize(); *mem = val;
   #endif
}

}  //namespace ipcdetail{
}  //namespace interprocess{
}  //namespace boost{

#else

#error No atomic operations implemented for this platform, sorry!

#endif

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_DETAIL_ATOMIC_HPP
