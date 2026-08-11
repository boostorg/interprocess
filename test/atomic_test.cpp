//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2004-2026. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/interprocess/detail/os_thread_functions.hpp>
#include <boost/interprocess/sync/spin/wait.hpp>
#include <boost/cstdint.hpp>
#include <cstddef>
#include <iostream>
#include "boost_interprocess_check.hpp"

using namespace boost::interprocess;
using boost::interprocess::ipcdetail::atomic_read32;
using boost::interprocess::ipcdetail::atomic_read32_acquire;
using boost::interprocess::ipcdetail::atomic_write32;
using boost::interprocess::ipcdetail::atomic_write32_release;
using boost::interprocess::ipcdetail::atomic_add32;
using boost::interprocess::ipcdetail::atomic_sub32;
using boost::interprocess::ipcdetail::atomic_cas32;
using boost::interprocess::ipcdetail::atomic_cas32_acquire;
using boost::interprocess::ipcdetail::atomic_cas32_release;

//!"Take one unless the value is already zero", the compare and swap loop that
//!a counting semaphore needs. It used to be a primitive of its own
//!(atomic_add_unless32) and is now built from the primitives it was made of,
//!so it is tested here in the shape the library really uses it.
inline bool dec_unless_zero(volatile boost::uint32_t *mem)
{
   boost::uint32_t count = atomic_read32_acquire(mem);
   while(count != 0u){
      const boost::uint32_t prev = atomic_cas32_acquire(mem, count - 1u, count);
      if(prev == count){
         return true;
      }
      count = prev;
   }
   return false;
}

//////////////////////////////////////////////////////////////////////////////
//
//    Single threaded tests: return values and resulting state
//
//////////////////////////////////////////////////////////////////////////////

void test_read_write()
{
   volatile boost::uint32_t v = 0u;

   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == 0u);
   BOOST_INTERPROCESS_CHECK(atomic_read32_acquire(&v) == 0u);

   atomic_write32(&v, 12345u);
   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == 12345u);
   BOOST_INTERPROCESS_CHECK(atomic_read32_acquire(&v) == 12345u);

   atomic_write32_release(&v, 54321u);
   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == 54321u);
   BOOST_INTERPROCESS_CHECK(atomic_read32_acquire(&v) == 54321u);

   //The whole 32 bit range must survive a write/read round trip
   const boost::uint32_t all_ones = ~boost::uint32_t(0);
   atomic_write32(&v, all_ones);
   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == all_ones);
   atomic_write32_release(&v, all_ones);
   BOOST_INTERPROCESS_CHECK(atomic_read32_acquire(&v) == all_ones);

   //Reading must not modify the value: atomic_read32 is also used on
   //read-only mapped regions, so it can't be a read-modify-write operation
   atomic_write32(&v, 7u);
   for(int i = 0; i != 8; ++i){
      BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == 7u);
      BOOST_INTERPROCESS_CHECK(atomic_read32_acquire(&v) == 7u);
   }
   BOOST_INTERPROCESS_CHECK(v == 7u);
}

void test_add_sub()
{
   volatile boost::uint32_t v = 0u;

   //Both must return the *old* value
   BOOST_INTERPROCESS_CHECK(atomic_add32(&v, 1u) == 0u);
   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == 1u);
   BOOST_INTERPROCESS_CHECK(atomic_add32(&v, 1u) == 1u);
   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == 2u);

   BOOST_INTERPROCESS_CHECK(atomic_sub32(&v, 1u) == 2u);
   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == 1u);
   BOOST_INTERPROCESS_CHECK(atomic_sub32(&v, 1u) == 1u);
   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == 0u);

   //Wrap around, the operations are modulo 2^32
   const boost::uint32_t all_ones = ~boost::uint32_t(0);
   atomic_write32(&v, all_ones);
   BOOST_INTERPROCESS_CHECK(atomic_add32(&v, 1u) == all_ones);
   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == 0u);
   BOOST_INTERPROCESS_CHECK(atomic_sub32(&v, 1u) == 0u);
   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == all_ones);
}

void test_cas()
{
   volatile boost::uint32_t v = 100u;

   //Comparison fails: value is left untouched, old value returned
   BOOST_INTERPROCESS_CHECK(atomic_cas32(&v, 1u, 99u) == 100u);
   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == 100u);

   //Comparison succeeds: value is swapped, old value returned
   BOOST_INTERPROCESS_CHECK(atomic_cas32(&v, 200u, 100u) == 100u);
   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == 200u);

   //Swapping a value for itself is still a successful compare
   BOOST_INTERPROCESS_CHECK(atomic_cas32(&v, 200u, 200u) == 200u);
   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == 200u);

   //Zero and all-ones must be usable both as comparand and as new value
   const boost::uint32_t all_ones = ~boost::uint32_t(0);
   atomic_write32(&v, 0u);
   BOOST_INTERPROCESS_CHECK(atomic_cas32(&v, all_ones, 0u) == 0u);
   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == all_ones);
   BOOST_INTERPROCESS_CHECK(atomic_cas32(&v, 0u, all_ones) == all_ones);
   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == 0u);
}

//!The acquire/release variants must behave exactly like atomic_cas32, only
//!their memory order differs
void test_cas_acquire_release()
{
   const boost::uint32_t all_ones = ~boost::uint32_t(0);
   volatile boost::uint32_t v = 100u;

   //Failed compare: value untouched, old value returned
   BOOST_INTERPROCESS_CHECK(atomic_cas32_acquire(&v, 1u, 99u) == 100u);
   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == 100u);
   BOOST_INTERPROCESS_CHECK(atomic_cas32_release(&v, 1u, 99u) == 100u);
   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == 100u);

   //Successful compare: value swapped, old value returned
   BOOST_INTERPROCESS_CHECK(atomic_cas32_acquire(&v, 200u, 100u) == 100u);
   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == 200u);
   BOOST_INTERPROCESS_CHECK(atomic_cas32_release(&v, 300u, 200u) == 200u);
   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == 300u);

   //Swapping a value for itself is still a successful compare
   BOOST_INTERPROCESS_CHECK(atomic_cas32_acquire(&v, 300u, 300u) == 300u);
   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == 300u);
   BOOST_INTERPROCESS_CHECK(atomic_cas32_release(&v, 300u, 300u) == 300u);
   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == 300u);

   //Zero and all-ones as comparand and as new value
   atomic_write32(&v, 0u);
   BOOST_INTERPROCESS_CHECK(atomic_cas32_acquire(&v, all_ones, 0u) == 0u);
   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == all_ones);
   BOOST_INTERPROCESS_CHECK(atomic_cas32_release(&v, 0u, all_ones) == all_ones);
   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == 0u);
}

void test_dec_unless_zero()
{
   volatile boost::uint32_t v = 5u;

   //Takes one and returns true while there is something left
   BOOST_INTERPROCESS_CHECK(dec_unless_zero(&v));
   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == 4u);

   //Counting down to zero, as a semaphore would
   atomic_write32(&v, 2u);
   BOOST_INTERPROCESS_CHECK(dec_unless_zero(&v));
   BOOST_INTERPROCESS_CHECK(dec_unless_zero(&v));
   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == 0u);

   //Now it must refuse to go below zero, no matter how many times it is asked
   BOOST_INTERPROCESS_CHECK(!dec_unless_zero(&v));
   BOOST_INTERPROCESS_CHECK(!dec_unless_zero(&v));
   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == 0u);

   //All ones must be usable as a count, it is never confused with zero
   const boost::uint32_t all_ones = ~boost::uint32_t(0);
   atomic_write32(&v, all_ones);
   BOOST_INTERPROCESS_CHECK(dec_unless_zero(&v));
   BOOST_INTERPROCESS_CHECK(atomic_read32(&v) == all_ones - 1u);
}

//////////////////////////////////////////////////////////////////////////////
//
//    Multithreaded tests
//
//////////////////////////////////////////////////////////////////////////////

static const boost::uint32_t NumThreads     = 4u;
static const boost::uint32_t NumIterations  = 20000u;

//!Enough rounds to give a broken implementation many chances to be caught,
//!while keeping the test short on the slowest tested platforms
static const boost::uint32_t NumOrderRounds = 50000u;

//!Spins until "expected" threads reached this point, so that the threads of a
//!test really run at the same time instead of one finishing before the next
//!one starts. A plain counter is enough: no test depends on this rendezvous
//!for its correctness, it only makes contention likely.
class rendezvous
{
   public:
   rendezvous() : m_arrived(0u) {}

   void wait(boost::uint32_t expected)
   {
      atomic_add32(&m_arrived, 1u);
      spin_wait swait;
      while(atomic_read32(&m_arrived) < expected){
         swait.yield();
      }
   }

   private:
   volatile boost::uint32_t m_arrived;
};

//----------------------------------------------------------------------------
// Add/subtract stress: no update may be lost
//----------------------------------------------------------------------------

struct counter_data
{
   counter_data() : m_counter(0u), m_rendezvous() {}
   volatile boost::uint32_t m_counter;
   rendezvous              m_rendezvous;
};

class counter_thread
{
   public:
   explicit counter_thread(counter_data &d) : m_data(&d) {}

   void operator()() const
   {
      m_data->m_rendezvous.wait(NumThreads);
      for(boost::uint32_t i = 0; i != NumIterations; ++i){
         atomic_add32(&m_data->m_counter, 1u);
      }
      for(boost::uint32_t i = 0; i != NumIterations; ++i){
         atomic_sub32(&m_data->m_counter, 1u);
      }
      for(boost::uint32_t i = 0; i != NumIterations; ++i){
         atomic_add32(&m_data->m_counter, 1u);
      }
   }

   private:
   counter_data *m_data;
};

void test_concurrent_inc_dec()
{
   counter_data d;
   ipcdetail::OS_thread_t threads[NumThreads];

   for(boost::uint32_t i = 0; i != NumThreads; ++i){
      ipcdetail::thread_launch(threads[i], counter_thread(d));
   }
   for(boost::uint32_t i = 0; i != NumThreads; ++i){
      ipcdetail::thread_join(threads[i]);
   }

   //Every increment and decrement must have been accounted for
   BOOST_INTERPROCESS_CHECK(atomic_read32(&d.m_counter) == NumThreads*NumIterations);
}

//----------------------------------------------------------------------------
// CAS as a spinlock: a non-atomic counter guarded by atomic_cas32 must not
// lose updates, which only holds if the CAS is really atomic
//----------------------------------------------------------------------------

struct cas_lock_data
{
   cas_lock_data() : m_lock(0u), m_guarded(0u), m_rendezvous() {}
   volatile boost::uint32_t m_lock;
   //Deliberately not atomic: it is protected by the CAS based lock
   boost::uint32_t          m_guarded;
   rendezvous               m_rendezvous;
};

class cas_lock_thread
{
   public:
   explicit cas_lock_thread(cas_lock_data &d) : m_data(&d) {}

   void operator()() const
   {
      m_data->m_rendezvous.wait(NumThreads);
      for(boost::uint32_t i = 0; i != NumIterations; ++i){
         //Lock with acquire semantics, so that the guarded data written by
         //the previous owner is visible to this thread
         spin_wait swait;
         while(atomic_cas32_acquire(&m_data->m_lock, 1u, 0u) != 0u){
            swait.yield();
         }
         ++m_data->m_guarded;
         //Unlock with release semantics, so that the write above is visible
         //to the next owner
         atomic_cas32_release(&m_data->m_lock, 0u, 1u);
      }
   }

   private:
   cas_lock_data *m_data;
};

void test_cas_mutual_exclusion()
{
   cas_lock_data d;
   ipcdetail::OS_thread_t threads[NumThreads];

   for(boost::uint32_t i = 0; i != NumThreads; ++i){
      ipcdetail::thread_launch(threads[i], cas_lock_thread(d));
   }
   for(boost::uint32_t i = 0; i != NumThreads; ++i){
      ipcdetail::thread_join(threads[i]);
   }

   BOOST_INTERPROCESS_CHECK(atomic_read32(&d.m_lock) == 0u);
   BOOST_INTERPROCESS_CHECK(d.m_guarded == NumThreads*NumIterations);
}

//----------------------------------------------------------------------------
// The decrement-unless-zero loop as a semaphore: exactly "initial" threads
// may take a token, never one more
//----------------------------------------------------------------------------

struct add_unless_data
{
   add_unless_data() : m_tokens(0u), m_taken(0u), m_rendezvous() {}
   volatile boost::uint32_t m_tokens;
   volatile boost::uint32_t m_taken;
   rendezvous               m_rendezvous;
};

class add_unless_thread
{
   public:
   explicit add_unless_thread(add_unless_data &d) : m_data(&d) {}

   void operator()() const
   {
      m_data->m_rendezvous.wait(NumThreads);
      //Every thread tries to take more tokens than exist in total
      for(boost::uint32_t i = 0; i != NumIterations; ++i){
         if(dec_unless_zero(&m_data->m_tokens)){
            atomic_add32(&m_data->m_taken, 1u);
         }
      }
   }

   private:
   add_unless_data *m_data;
};

void test_concurrent_dec_unless_zero()
{
   const boost::uint32_t InitialTokens = NumThreads*NumIterations/2u;

   add_unless_data d;
   atomic_write32(&d.m_tokens, InitialTokens);

   ipcdetail::OS_thread_t threads[NumThreads];
   for(boost::uint32_t i = 0; i != NumThreads; ++i){
      ipcdetail::thread_launch(threads[i], add_unless_thread(d));
   }
   for(boost::uint32_t i = 0; i != NumThreads; ++i){
      ipcdetail::thread_join(threads[i]);
   }

   //The counter must have stopped exactly at zero, never wrapped around,
   //and exactly InitialTokens threads must have succeeded
   BOOST_INTERPROCESS_CHECK(atomic_read32(&d.m_tokens) == 0u);
   BOOST_INTERPROCESS_CHECK(atomic_read32(&d.m_taken) == InitialTokens);
}

//----------------------------------------------------------------------------
// Release/acquire publication: everything written before a release store must
// be visible to a thread that reads the flag with an acquire load
//----------------------------------------------------------------------------

static const std::size_t PayloadSize = 8u;

struct publish_data
{
   publish_data() : m_flag(0u), m_failures(0u), m_rendezvous()
   {
      for(std::size_t i = 0; i != PayloadSize; ++i){
         m_payload[i] = 0u;
      }
   }

   volatile boost::uint32_t m_flag;
   //Plain, non atomic data: its visibility is what the release/acquire pair
   //is responsible for
   boost::uint32_t          m_payload[PayloadSize];
   volatile boost::uint32_t m_failures;
   rendezvous               m_rendezvous;
};

class publisher_thread
{
   public:
   explicit publisher_thread(publish_data &d) : m_data(&d) {}

   void operator()() const
   {
      m_data->m_rendezvous.wait(2u);
      for(boost::uint32_t round = 1u; round <= NumOrderRounds; ++round){
         //Wait until the consumer has acknowledged the previous round
         spin_wait swait;
         while(atomic_read32_acquire(&m_data->m_flag) != 0u){
            swait.yield();
         }
         for(std::size_t i = 0; i != PayloadSize; ++i){
            m_data->m_payload[i] = round;
         }
         //Publish: the payload writes above must not be reordered after this
         atomic_write32_release(&m_data->m_flag, round);
      }
   }

   private:
   publish_data *m_data;
};

class consumer_thread
{
   public:
   explicit consumer_thread(publish_data &d) : m_data(&d) {}

   void operator()() const
   {
      m_data->m_rendezvous.wait(2u);
      for(boost::uint32_t round = 1u; round <= NumOrderRounds; ++round){
         boost::uint32_t seen;
         spin_wait swait;
         while((seen = atomic_read32_acquire(&m_data->m_flag)) == 0u){
            swait.yield();
         }
         //Having seen the flag, the whole payload must already be visible
         for(std::size_t i = 0; i != PayloadSize; ++i){
            if(m_data->m_payload[i] != seen){
               atomic_add32(&m_data->m_failures, 1u);
            }
         }
         //Acknowledge, letting the publisher start the next round
         atomic_write32_release(&m_data->m_flag, 0u);
      }
   }

   private:
   publish_data *m_data;
};

void test_release_acquire_publication()
{
   publish_data d;
   ipcdetail::OS_thread_t producer, consumer;

   ipcdetail::thread_launch(producer, publisher_thread(d));
   ipcdetail::thread_launch(consumer, consumer_thread(d));
   ipcdetail::thread_join(producer);
   ipcdetail::thread_join(consumer);

   BOOST_INTERPROCESS_CHECK(atomic_read32(&d.m_failures) == 0u);
}

//----------------------------------------------------------------------------
// Sequential consistency: the store buffer (Dekker) litmus test.
//
// Each thread stores 1 to its own variable and then reads the other one.
// Under sequential consistency at least one of the two reads must see 1;
// both reading 0 means the store was reordered after the load, which
// acquire/release would allow but seq_cst must not.
//
// This is a best effort detector: not every slot really races, so it can miss
// a violation, but it can never fail unless one really happened.
//----------------------------------------------------------------------------

struct sb_slot
{
   volatile boost::uint32_t x;
   volatile boost::uint32_t y;
};

struct store_buffer_data
{
   //The arrays are allocated and initialized by the caller
   store_buffer_data()
      : m_slots(0), m_read_by_0(0), m_read_by_1(0), m_rendezvous()
   {}

   sb_slot         *m_slots;
   boost::uint32_t *m_read_by_0;
   boost::uint32_t *m_read_by_1;
   rendezvous       m_rendezvous;
};

class store_buffer_thread
{
   public:
   store_buffer_thread(store_buffer_data &d, bool first) : m_data(&d), m_first(first) {}

   void operator()() const
   {
      m_data->m_rendezvous.wait(2u);
      for(boost::uint32_t i = 0; i != NumOrderRounds; ++i){
         if(m_first){
            atomic_write32(&m_data->m_slots[i].x, 1u);
            m_data->m_read_by_0[i] = atomic_read32(&m_data->m_slots[i].y);
         }
         else{
            atomic_write32(&m_data->m_slots[i].y, 1u);
            m_data->m_read_by_1[i] = atomic_read32(&m_data->m_slots[i].x);
         }
      }
   }

   private:
   store_buffer_data *m_data;
   bool               m_first;
};

void test_sequential_consistency()
{
   store_buffer_data d;
   //Heap allocated so that the test does not need a huge stack
   d.m_slots     = new sb_slot[NumOrderRounds];
   d.m_read_by_0 = new boost::uint32_t[NumOrderRounds];
   d.m_read_by_1 = new boost::uint32_t[NumOrderRounds];
   for(boost::uint32_t i = 0; i != NumOrderRounds; ++i){
      d.m_slots[i].x = 0u;
      d.m_slots[i].y = 0u;
      d.m_read_by_0[i] = 1u;
      d.m_read_by_1[i] = 1u;
   }

   boost::uint32_t both_zero = 0u;
   {
      ipcdetail::OS_thread_t t0, t1;
      ipcdetail::thread_launch(t0, store_buffer_thread(d, true));
      ipcdetail::thread_launch(t1, store_buffer_thread(d, false));
      ipcdetail::thread_join(t0);
      ipcdetail::thread_join(t1);

      for(boost::uint32_t i = 0; i != NumOrderRounds; ++i){
         if(d.m_read_by_0[i] == 0u && d.m_read_by_1[i] == 0u){
            ++both_zero;
         }
      }
   }

   delete [] d.m_slots;
   delete [] d.m_read_by_0;
   delete [] d.m_read_by_1;

   if(both_zero){
      std::cout << "Sequential consistency violated in " << both_zero
                << " of " << NumOrderRounds << " rounds" << std::endl;
   }
   BOOST_INTERPROCESS_CHECK(both_zero == 0u);
}

//----------------------------------------------------------------------------
// Strong compare and swap: no spurious failure
//----------------------------------------------------------------------------

//!The compare and swap of this header must be strong: whenever the old value
//!equals the comparand the swap has to be performed. A weak one would be
//!allowed to fail from time to time without the value having changed, and the
//!interface could not even report it, as it returns the old value and not
//!whether the swap happened. Such a failure is indistinguishable from a
//!success by looking at the returned value, so it is detected here by checking
//!that the memory really changed.
//!
//!Spurious failures come from the load-linked/store-conditional pairs used by
//!weakly ordered platforms, and another thread writing to the same cache line
//!makes them far more likely, so a peer keeps a neighbour word busy meanwhile.
struct strong_cas_data
{
   strong_cas_data() : m_value(0u), m_neighbour(0u), m_stop(0u), m_rendezvous() {}

   volatile boost::uint32_t m_value;
   //!Different word, deliberately close to m_value so that both usually share
   //!a cache line and the peer steals its exclusive ownership
   volatile boost::uint32_t m_neighbour;
   volatile boost::uint32_t m_stop;
   rendezvous               m_rendezvous;
};

//!Keeps the cache line of the word under test busy until told to stop
class cache_line_disturber
{
   public:
   explicit cache_line_disturber(strong_cas_data &d) : m_data(&d) {}

   void operator()() const
   {
      m_data->m_rendezvous.wait(2u);
      while(atomic_read32(&m_data->m_stop) == 0u){
         atomic_add32(&m_data->m_neighbour, 1u);
      }
   }

   private:
   strong_cas_data *m_data;
};

void test_cas_is_strong()
{
   const boost::uint32_t A = 0xA5A5A5A5u;
   const boost::uint32_t B = 0x5A5A5A5Au;

   strong_cas_data d;
   ipcdetail::OS_thread_t disturber;
   ipcdetail::thread_launch(disturber, cache_line_disturber(d));
   d.m_rendezvous.wait(2u);

   for(boost::uint32_t i = 0; i != NumOrderRounds; ++i){
      //Each variant must swap every single time, as the comparand always
      //matches the current value
      atomic_write32(&d.m_value, A);
      BOOST_INTERPROCESS_CHECK(atomic_cas32(&d.m_value, B, A) == A);
      BOOST_INTERPROCESS_CHECK(atomic_read32(&d.m_value) == B);

      atomic_write32(&d.m_value, A);
      BOOST_INTERPROCESS_CHECK(atomic_cas32_acquire(&d.m_value, B, A) == A);
      BOOST_INTERPROCESS_CHECK(atomic_read32(&d.m_value) == B);

      atomic_write32(&d.m_value, A);
      BOOST_INTERPROCESS_CHECK(atomic_cas32_release(&d.m_value, B, A) == A);
      BOOST_INTERPROCESS_CHECK(atomic_read32(&d.m_value) == B);

      //A comparand that does not match must never swap, no matter the noise
      atomic_write32(&d.m_value, A);
      BOOST_INTERPROCESS_CHECK(atomic_cas32(&d.m_value, B, B) == A);
      BOOST_INTERPROCESS_CHECK(atomic_read32(&d.m_value) == A);
   }

   atomic_write32(&d.m_stop, 1u);
   ipcdetail::thread_join(disturber);
}

//////////////////////////////////////////////////////////////////////////////

int main()
{
   //BOOST_INTERPROCESS_CHECK reports and throws on failure, as in the rest
   //of the test suite, so a failing check aborts the test
   test_read_write();
   test_add_sub();
   test_cas();
   test_cas_acquire_release();
   test_dec_unless_zero();

   test_concurrent_inc_dec();
   test_cas_is_strong();
   test_cas_mutual_exclusion();
   test_concurrent_dec_unless_zero();
   test_release_acquire_publication();
   test_sequential_consistency();

   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
