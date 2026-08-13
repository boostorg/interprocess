//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Peter Dimov 2008.
// (C) Copyright Ion Gaztanaga 2013-2013. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

//Parts of this file come from boost/smart_ptr/detail/yield_k.hpp
//Many thanks to Peter Dimov.

#ifndef BOOST_INTERPROCESS_SYNC_WAIT_HPP_INCLUDED
#define BOOST_INTERPROCESS_SYNC_WAIT_HPP_INCLUDED

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
# pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/os_thread_functions.hpp>

//#define BOOST_INTERPROCESS_SPIN_WAIT_DEBUG
#ifdef BOOST_INTERPROCESS_SPIN_WAIT_DEBUG
#include <iostream>
#endif

//////////////////////////////////////////////////////////////////////////////
//
//                   BOOST_INTERPROCESS_SMT_PAUSE
//
//! Emits the processor hint that marks a spin loop. It saves power and, on
//! simultaneous multithreading processors, hands the shared execution
//! resources over to the sibling hardware threads, which is what lets the
//! thread holding the lock make progress.
//!
//! It's always defined: on processors with no such hint it expands to
//! nothing, so that spin_wait follows the same strategy everywhere.
//
//////////////////////////////////////////////////////////////////////////////

//Detect the portable x86 pause builtin (Clang, GCC 10 and later). Excluded on
//MSVC ARM targets, where the x86 spellings of the architecture macros are also
//defined but the builtin is not available.
#if defined(__has_builtin) && !defined(_M_ARM64EC) && !defined(_M_ARM64) && !defined(_M_ARM)
#  if __has_builtin(__builtin_ia32_pause) && !defined(__INTEL_COMPILER)
#     define BOOST_INTERPROCESS_HAS_BUILTIN_IA32_PAUSE
#  endif
#endif

//Forward declaration of MSVC intrinsics
//Note: ARM64EC also defines _M_AMD64/_M_X64, so it must be tested first
#if defined(_MSC_VER)
#if defined(_M_ARM64EC) || defined(_M_ARM64) || defined(_M_ARM)
extern "C" void __yield(void);
#if defined(BOOST_MSVC)
#pragma intrinsic(__yield)
#endif
#elif defined(_M_AMD64) || defined(_M_IX86) || defined(_M_X64)
extern "C" void _mm_pause(void);
#if defined(BOOST_MSVC)
#pragma intrinsic(_mm_pause)
#endif
#endif
#endif

#if defined(BOOST_INTERPROCESS_HAS_BUILTIN_IA32_PAUSE)

//x86/x86-64 PAUSE, without inline assembly
#define BOOST_INTERPROCESS_SMT_PAUSE   __builtin_ia32_pause();

#elif defined(_MSC_VER) && ( defined(_M_ARM64EC) || defined(_M_ARM64) || defined(_M_ARM) )

#define BOOST_INTERPROCESS_SMT_PAUSE __yield();

#elif defined(_MSC_VER) && ( defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64) )

#define BOOST_INTERPROCESS_SMT_PAUSE _mm_pause();

#elif defined(__GNUC__) && ( defined(__i386__) || defined(__x86_64__) ) && !defined(_CRAYC)

#define BOOST_INTERPROCESS_SMT_PAUSE   __asm__ __volatile__("rep; nop" : : : "memory");

#elif defined(__GNUC__) &&\
      (  defined(__aarch64__) || defined(__ARM_ARCH_8A__)\
      || (defined(__ARM_ARCH) && __ARM_ARCH >= 7)\
      || defined(__ARM_ARCH_7__)   || defined(__ARM_ARCH_7A__)  || defined(__ARM_ARCH_7R__)\
      || defined(__ARM_ARCH_7M__)  || defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_7S__)\
      || defined(__ARM_ARCH_6K__)  || defined(__ARM_ARCH_6KZ__) || defined(__ARM_ARCH_6ZK__) )

//YIELD, available on AArch64 and on 32 bit ARM since ARMv6K/ARMv7. Older ARM
//processors have no such hint, and the instruction does not even assemble.
#define BOOST_INTERPROCESS_SMT_PAUSE   __asm__ __volatile__("yield" : : : "memory");

#elif defined(__GNUC__) &&\
      ( defined(__powerpc__) || defined(__powerpc64__) || defined(__ppc__)\
     || defined(__ppc64__)   || defined(__PPC__)       || defined(__PPC64__) || defined(_ARCH_PPC) )

//Drop this thread's program priority to low while spinning and restore it to
//medium right afterwards, which is how a POWER processor is told to give its
//shared resources to the sibling threads. Both are "or rX,rX,rX" forms, which
//are plain no-ops on processors that don't implement the priority hints, so
//they are safe everywhere.
#define BOOST_INTERPROCESS_SMT_PAUSE   __asm__ __volatile__("or 1,1,1\n\tor 2,2,2" : : : "memory");

#elif defined(__GNUC__) && defined(__riscv)

//PAUSE (Zihintpause extension). It's encoded in the FENCE space and defined
//as a no-op on processors that don't implement it, so it's always safe to
//emit. It's written as an encoding rather than as the "pause" mnemonic
//because assemblers without Zihintpause support reject the mnemonic, notably
//on 32 bit RISC-V.
#define BOOST_INTERPROCESS_SMT_PAUSE   __asm__ __volatile__(".insn i 0x0F, 0, x0, x0, 0x010" : : : "memory");

#endif

#if !defined(BOOST_INTERPROCESS_SMT_PAUSE)

//No spin loop hint is known for this processor. Defined empty so that
//spin_wait needs no conditional code.
#define BOOST_INTERPROCESS_SMT_PAUSE

#endif


namespace boost{
namespace interprocess{
namespace ipcdetail {

template<int Dummy = 0>
class num_core_holder
{
   public:
   static unsigned int get()
   {
      if(!num_cores){
         return ipcdetail::get_num_cores();
      }
      else{
         return num_cores;
      }
   }

   private:
   static unsigned int num_cores;
};

template<int Dummy>
unsigned int num_core_holder<Dummy>::num_cores = ipcdetail::get_num_cores();

}  //namespace ipcdetail {

class spin_wait
{
   public:

   static const unsigned int nop_pause_limit = 32u;
   spin_wait()
      : m_count_start(), m_ul_yield_only_counts(), m_k()
   {}

   #ifdef BOOST_INTERPROCESS_SPIN_WAIT_DEBUG
   ~spin_wait()
   {
      if(m_k){
         std::cout << "final m_k: " << m_k
                   << " system tick(us): " << ipcdetail::get_system_tick_us() << std::endl;
      }
   }
   #endif

   unsigned int count() const
   {  return m_k;  }

   void yield()
   {
      //Lazy initialization of limits
      if( !m_k){
         this->init_limits();
      }
      //Nop tries
      if( m_k < (nop_pause_limit >> 2) ){

      }
      //Pause tries
      else if( m_k < nop_pause_limit ){
         BOOST_INTERPROCESS_SMT_PAUSE
      }
      //Yield/Sleep strategy
      else{
         //Lazy initialization of tick information
         if(m_k == nop_pause_limit){
            this->init_tick_info();
         }
         else if( this->yield_or_sleep() ){
            ipcdetail::thread_yield();
         }
         else{
            ipcdetail::thread_sleep_tick();
         }
      }
      ++m_k;
   }

   void reset()
   {
      m_k = 0u;
   }

   private:

   void init_limits()
   {
      unsigned int num_cores = ipcdetail::num_core_holder<0>::get();
      m_k = num_cores > 1u ? 0u : nop_pause_limit;
   }

   void init_tick_info()
   {
      m_ul_yield_only_counts = ipcdetail::get_system_tick_in_highres_counts();
      m_count_start = ipcdetail::get_current_system_highres_count();
   }

   //Returns true if yield must be called, false is sleep must be called
   bool yield_or_sleep()
   {
      if(!m_ul_yield_only_counts){  //If yield-only limit was reached then yield one in every two tries
         return (m_k & 1u) != 0;
      }
      else{ //Try to see if we've reached yield-only time limit
         const ipcdetail::OS_highres_count_t now = ipcdetail::get_current_system_highres_count();
         const ipcdetail::OS_highres_count_t elapsed = ipcdetail::system_highres_count_subtract(now, m_count_start);
         if(!ipcdetail::system_highres_count_less_ul(elapsed, m_ul_yield_only_counts)){
            #ifdef BOOST_INTERPROCESS_SPIN_WAIT_DEBUG
            std::cout << "elapsed!\n"
                      << "  m_ul_yield_only_counts: " << m_ul_yield_only_counts
                     << " system tick(us): " << ipcdetail::get_system_tick_us() << '\n'
                      << "  m_k: " << m_k << " elapsed counts: ";
                     ipcdetail::ostream_highres_count(std::cout, elapsed) << std::endl;
            #endif
            //Yield-only time reached, now it's time to sleep
            m_ul_yield_only_counts = 0ul;
            return false;
         }
      }
      return true;   //Otherwise yield
   }

   ipcdetail::OS_highres_count_t m_count_start;
   unsigned long m_ul_yield_only_counts;
   unsigned int  m_k;
};

} // namespace interprocess
} // namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif // #ifndef BOOST_INTERPROCESS_SYNC_WAIT_HPP_INCLUDED
