//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2026. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
//
// Micro-benchmark for boost::interprocess::offset_ptr.
//
// It measures the cost of the operations that dominate real code that uses
// offset_ptr (node traversal, iteration, stores, copies, comparisons) and
// compares them against raw pointers.
//
//////////////////////////////////////////////////////////////////////////////

//Define LONG_BENCH for a long, high confidence run. Without it the benchmark
//measures exactly the same thing with fewer repetitions, so that it stays
//usable as a quick check. It is meant to be activated by hand, it is not
//defined by the build of the regression tests.
#ifndef LONG_BENCH
//#define LONG_BENCH
#endif

#ifdef _MSC_VER
#pragma warning (disable : 4512)
#include <intrin.h>     //_ReadWriteBarrier
#endif

#include <boost/interprocess/offset_ptr.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/move/detail/nsec_clock.hpp>
#include <boost/cstdint.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <new>       //placement new
#include <string>
#include <vector>

namespace bench {

typedef boost::move_detail::nanosecond_type nanosecond_type;
typedef boost::interprocess::uintptr_t      uintptr_type;

//////////////////////////////////////////////////////////////////////////////
//
//                               Utilities
//
//////////////////////////////////////////////////////////////////////////////

//!Value the benchmark accumulates into, so that the compiler can't
//!remove the measured loops.
volatile std::size_t g_sink = 0;

//!Destination of the read that escape() performs on MSVC.
volatile char g_escape_sink = 0;

//////////////////////////////////////////////////////////////////////////////
//
//                            Compiler barriers
//
//!escape(p) makes the memory that 'p' points to observable from outside the
//!function, so that the writes to it are not removed. clobber() tells the
//!compiler that memory may have been read and written by someone else, so
//!that a measured loop is not hoisted out of the repetition loop nor merged
//!with the repetitions that follow it. Neither of them generates any
//!instruction.
//
//////////////////////////////////////////////////////////////////////////////

//#define BOOST_INTERPROCESS_BENCH_NO_BARRIERS

#ifdef BOOST_INTERPROCESS_BENCH_NO_BARRIERS
   #define BOOST_INTERPROCESS_BENCH_CLOBBER()   ((void)0)
   #define BOOST_INTERPROCESS_BENCH_ESCAPE(p)   ((void)(p))
#else
   #if defined(_MSC_VER)
      #define BOOST_INTERPROCESS_BENCH_CLOBBER()   _ReadWriteBarrier()
      #define BOOST_INTERPROCESS_BENCH_ESCAPE(p)   (g_escape_sink = *static_cast<const volatile char *>(p))
   #elif defined(__GNUC__)
      #define BOOST_INTERPROCESS_BENCH_CLOBBER()   asm volatile("" : : : "memory")
      #define BOOST_INTERPROCESS_BENCH_ESCAPE(p)   asm volatile("" : : "g"(p) : "memory")
   #else
      #define BOOST_INTERPROCESS_BENCH_CLOBBER()   ((void)0)
      #define BOOST_INTERPROCESS_BENCH_ESCAPE(p)   ((void)(p))
   #endif
#endif

inline void clobber()
{  BOOST_INTERPROCESS_BENCH_CLOBBER();  }

inline void escape(const void *p)
{  BOOST_INTERPROCESS_BENCH_ESCAPE(p);  }

//!Deterministic, cheap and portable random number generator, so that
//!results can be reproduced between runs and between platforms.
class xorshift
{
   public:
   explicit xorshift(boost::uint32_t seed = 2463534242u)
      : m_state(seed)
   {}

   boost::uint32_t next()
   {
      m_state ^= boost::uint32_t(m_state << 13);
      m_state ^= boost::uint32_t(m_state >> 17);
      m_state ^= boost::uint32_t(m_state << 5);
      return m_state;
   }

   std::size_t below(std::size_t n)
   {  return std::size_t(this->next()) % n;  }

   private:
   boost::uint32_t m_state;
};

class stopwatch
{
   public:
   stopwatch()
   {  m_timer.resume();  }

   nanosecond_type nsecs() const
   {  return m_timer.elapsed().wall;  }

   private:
   boost::move_detail::cpu_timer m_timer;
};

//////////////////////////////////////////////////////////////////////////////
//
//                          Pointer type traits
//
//////////////////////////////////////////////////////////////////////////////

struct raw_traits
{
   template<class T>
   struct ptr
   {  typedef T *type;  };

   template<class T>
   struct const_ptr
   {  typedef const T *type;  };

   typedef void *void_ptr;

   template<class T>
   static T *pointer_cast(void_ptr p)
   {  return static_cast<T*>(p);  }

   template<class T>
   static T *address_of(T &r)
   {  return &r;  }

   template<class T>
   static T *to_raw(T *p)
   {  return p;  }

   template<class T, class U>
   static T *const_cast_ptr(U *p)
   {  return const_cast<T*>(p);  }

   template<class T, class U>
   static T *reinterpret_cast_ptr(U *p)
   {  return reinterpret_cast<T*>(p);  }

   template<class T, class U>
   static T *dynamic_cast_ptr(U *p)
   {  return dynamic_cast<T*>(p);  }
};

struct offset_traits
{
   template<class T>
   struct ptr
   {  typedef boost::interprocess::offset_ptr<T> type;  };

   template<class T>
   struct const_ptr
   {  typedef boost::interprocess::offset_ptr<const T> type;  };

   typedef boost::interprocess::offset_ptr<void> void_ptr;

   template<class T>
   static boost::interprocess::offset_ptr<T> pointer_cast(const void_ptr &p)
   {  return boost::interprocess::static_pointer_cast<T>(p);  }

   template<class T>
   static boost::interprocess::offset_ptr<T> address_of(T &r)
   {  return boost::interprocess::offset_ptr<T>::pointer_to(r);  }

   template<class T>
   static T *to_raw(const boost::interprocess::offset_ptr<T> &p)
   {  return boost::interprocess::to_raw_pointer(p);  }

   template<class T, class U>
   static boost::interprocess::offset_ptr<T> const_cast_ptr(const boost::interprocess::offset_ptr<U> &p)
   {  return boost::interprocess::const_pointer_cast<T>(p);  }

   template<class T, class U>
   static boost::interprocess::offset_ptr<T> reinterpret_cast_ptr(const boost::interprocess::offset_ptr<U> &p)
   {  return boost::interprocess::reinterpret_pointer_cast<T>(p);  }

   template<class T, class U>
   static boost::interprocess::offset_ptr<T> dynamic_cast_ptr(const boost::interprocess::offset_ptr<U> &p)
   {  return boost::interprocess::dynamic_pointer_cast<T>(p);  }
};

//!Types of the conversion benchmarks. 'up_derived' has two bases, so that
//!the conversion to 'up_base_b' really has to move the address and can not
//!be optimized away. 'dyn_derived' is polymorphic, as dynamic_cast needs
struct up_base_a
{  std::size_t a;  };

struct up_base_b
{  std::size_t b;  };

struct up_derived
   : up_base_a, up_base_b
{};

struct dyn_base
{
   virtual ~dyn_base(){}
   std::size_t v;
};

struct dyn_derived
   : dyn_base
{  std::size_t w;  };

//!Node used by the traversal benchmarks. Its layout only depends on the
//!size of the pointer, which is the same for every tested pointer type.
template<class Traits>
struct node
{
   typedef typename Traits::template ptr<node>::type node_ptr;

   node_ptr    next;
   std::size_t value;
};

//////////////////////////////////////////////////////////////////////////////
//
//                              Test cases
//
//////////////////////////////////////////////////////////////////////////////

enum test_id
{
   t_chase_seq,      //dependent loads, cache friendly: ALU bound
   t_chase_rand,     //dependent loads, cache hostile: latency bound
   t_iterate,        //++ptr + operator-> over an array
   t_deref,          //++ptr + operator* over an array
   t_index,          //operator[] over an array
   t_postinc,        //ptr++, which has to build an adjusted copy
   t_postdec,        //ptr--, the same for the other direction
   t_add,            //ptr + n and n + ptr
   t_sub,            //ptr - n
   t_addassign,      //ptr += n and ptr -= n
   t_store,          //raw pointer -> offset_ptr conversion (stores)
   t_assign_null,    //assignment of a null pointer
   t_copy,           //offset_ptr -> offset_ptr assignment
   t_copy_ctor,      //offset_ptr -> offset_ptr construction
   t_convert,        //conversion to a pointer to const, address preserving
   t_upcast,         //conversion to a base class, which moves the address
   t_cast,           //static_pointer_cast from a void pointer, as allocators do
   t_const_cast,     //const_pointer_cast
   t_reinterp_cast,  //reinterpret_pointer_cast
   t_dynamic_cast,   //dynamic_pointer_cast
   t_pointer_to,     //pointer_to, used by pointer_traits in every container
   t_get,            //get(), to_raw_pointer
   t_nullcheck,      //null tests
   t_compare,        //equality against a fixed pointer
   t_compare_raw,    //equality against a fixed raw pointer
   t_less,           //ordering against a fixed pointer
   t_sort,           //std::sort over an array of pointers
   t_swap,           //swap between two pointers
   t_difference,     //ptr - ptr
   t_count
};

const char *const test_name[t_count] =
   { "chase seq"
   , "chase rand"
   , "iterate"
   , "deref *p"
   , "index p[i]"
   , "post ++"
   , "post --"
   , "ptr + n"
   , "ptr - n"
   , "ptr +=/-= n"
   , "store raw"
   , "assign null"
   , "copy ptr"
   , "copy ctor"
   , "convert"
   , "derived->base"
   , "cast void*"
   , "const cast"
   , "reinterp cast"
   , "dynamic cast"
   , "pointer_to"
   , "get()"
   , "null check"
   , "compare"
   , "compare raw"
   , "less"
   , "sort"
   , "swap"
   , "difference"
   };

//!Builds a chain over 'nodes'. If 'shuffled' is false the chain follows
//!the array order, otherwise it follows a random permutation cycle.
template<class Traits>
void build_chain(std::vector< node<Traits> > &nodes, bool shuffled)
{
   const std::size_t n = nodes.size();
   std::vector<std::size_t> order(n);
   for(std::size_t i = 0; i != n; ++i){
      order[i] = i;
   }
   if(shuffled){
      xorshift rng(12345u);
      for(std::size_t i = n; i > 1; --i){
         const std::size_t j = rng.below(i);
         std::swap(order[i-1], order[j]);
      }
   }
   for(std::size_t i = 0; i != n; ++i){
      nodes[order[i]].value = i;
      nodes[order[i]].next  = (i+1 == n) ? 0 : &nodes[order[i+1]];
   }
}

template<class Traits>
nanosecond_type test_chase(std::size_t n, std::size_t reps, bool shuffled)
{
   typedef node<Traits>                      node_t;
   typedef typename node_t::node_ptr         node_ptr;

   std::vector<node_t> nodes(n);
   build_chain<Traits>(nodes, shuffled);
   node_ptr const head(&nodes[0]);

   escape(&nodes[0]);
   stopwatch sw;
   std::size_t sum = 0;
   for(std::size_t r = 0; r != reps; ++r){
      node_ptr p(head);
      while(p){
         sum += p->value;
         p = p->next;
      }
      clobber();
   }
   const nanosecond_type ns = sw.nsecs();
   g_sink += sum;
   return ns;
}

template<class Traits>
nanosecond_type test_iterate(std::size_t n, std::size_t reps)
{
   typedef node<Traits>                      node_t;
   typedef typename node_t::node_ptr         node_ptr;

   std::vector<node_t> nodes(n);
   build_chain<Traits>(nodes, false);

   escape(&nodes[0]);
   stopwatch sw;
   std::size_t sum = 0;
   for(std::size_t r = 0; r != reps; ++r){
      node_ptr p(&nodes[0]);
      const node_ptr e(&nodes[0] + n);
      for(; p != e; ++p){
         sum += p->value;
      }
      clobber();
   }
   const nanosecond_type ns = sw.nsecs();
   g_sink += sum;
   return ns;
}

//!Same as test_iterate, but the elements are read through operator*
//!instead of operator->
template<class Traits>
nanosecond_type test_deref(std::size_t n, std::size_t reps)
{
   typedef node<Traits>                      node_t;
   typedef typename node_t::node_ptr         node_ptr;

   std::vector<node_t> nodes(n);
   build_chain<Traits>(nodes, false);

   escape(&nodes[0]);
   stopwatch sw;
   std::size_t sum = 0;
   for(std::size_t r = 0; r != reps; ++r){
      node_ptr p(&nodes[0]);
      const node_ptr e(&nodes[0] + n);
      for(; p != e; ++p){
         sum += (*p).value;
      }
      clobber();
   }
   const nanosecond_type ns = sw.nsecs();
   g_sink += sum;
   return ns;
}

//!Reads the elements through operator[] from a single pointer, the way
//!a vector-like container indexes its storage
template<class Traits>
nanosecond_type test_index(std::size_t n, std::size_t reps)
{
   typedef node<Traits>                      node_t;
   typedef typename node_t::node_ptr         node_ptr;

   std::vector<node_t> nodes(n);
   build_chain<Traits>(nodes, false);
   const node_ptr base(&nodes[0]);

   escape(&nodes[0]);
   stopwatch sw;
   std::size_t sum = 0;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         sum += base[std::ptrdiff_t(i)].value;
      }
      clobber();
   }
   const nanosecond_type ns = sw.nsecs();
   g_sink += sum;
   return ns;
}

//!Postfix increment, which has to build a copy of the pointer adjusted to
//!the address of the temporary
template<class Traits>
nanosecond_type test_postinc(std::size_t n, std::size_t reps)
{
   typedef node<Traits>                      node_t;
   typedef typename node_t::node_ptr         node_ptr;

   std::vector<node_t> nodes(n);
   build_chain<Traits>(nodes, false);

   escape(&nodes[0]);
   stopwatch sw;
   std::size_t sum = 0;
   for(std::size_t r = 0; r != reps; ++r){
      node_ptr p(&nodes[0]);
      const node_ptr e(&nodes[0] + n);
      while(p != e){
         sum += (p++)->value;
      }
      clobber();
   }
   const nanosecond_type ns = sw.nsecs();
   g_sink += sum;
   return ns;
}

//!Postfix decrement. "(p--)[-1]" reads the element just below the old value
//!of the pointer, so the walk stays inside the array from end to begin
template<class Traits>
nanosecond_type test_postdec(std::size_t n, std::size_t reps)
{
   typedef node<Traits>                      node_t;
   typedef typename node_t::node_ptr         node_ptr;

   std::vector<node_t> nodes(n);
   build_chain<Traits>(nodes, false);

   escape(&nodes[0]);
   stopwatch sw;
   std::size_t sum = 0;
   for(std::size_t r = 0; r != reps; ++r){
      node_ptr p(&nodes[0] + n);
      const node_ptr b(&nodes[0]);
      while(p != b){
         sum += (p--)[-1].value;
      }
      clobber();
   }
   const nanosecond_type ns = sw.nsecs();
   g_sink += sum;
   return ns;
}

//!ptr + n. The even repetitions use "ptr + n" and the odd ones "n + ptr",
//!which is the other overload
template<class Traits>
nanosecond_type test_add(std::size_t n, std::size_t reps)
{
   typedef node<Traits>                                node_t;
   typedef typename Traits::template ptr<node_t>::type ptr_t;

   std::vector<node_t> nodes(n);
   std::vector<ptr_t>  dst(n);
   const ptr_t base(&nodes[0]);

   escape(&dst[0]);
   stopwatch sw;
   for(std::size_t r = 0; r != reps; ++r){
      if(r & 1u){
         for(std::size_t i = 0; i != n; ++i){
            dst[i] = std::ptrdiff_t(i) + base;
         }
      }
      else{
         for(std::size_t i = 0; i != n; ++i){
            dst[i] = base + std::ptrdiff_t(i);
         }
      }
      clobber();
   }
   return sw.nsecs();
}

//!ptr - n
template<class Traits>
nanosecond_type test_sub(std::size_t n, std::size_t reps)
{
   typedef node<Traits>                                node_t;
   typedef typename Traits::template ptr<node_t>::type ptr_t;

   std::vector<node_t> nodes(n);
   std::vector<ptr_t>  dst(n);
   const ptr_t last(&nodes[0] + n);

   escape(&dst[0]);
   stopwatch sw;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         dst[i] = last - std::ptrdiff_t(i);
      }
      clobber();
   }
   return sw.nsecs();
}

//!ptr += n and ptr -= n, which are pure offset arithmetic and need no
//!conversion at all. Both directions alternate so that the pointers keep
//!oscillating between two positions inside the array
template<class Traits>
nanosecond_type test_addassign(std::size_t n, std::size_t reps)
{
   typedef node<Traits>                                node_t;
   typedef typename Traits::template ptr<node_t>::type ptr_t;

   std::vector<node_t> nodes(n);
   std::vector<ptr_t>  ptrs(n);
   node_t *const raw = &nodes[0];
   for(std::size_t i = 0; i != n; ++i){
      ptrs[i] = raw + i;
   }

   escape(&ptrs[0]);
   stopwatch sw;
   for(std::size_t r = 0; r != reps; ++r){
      if(r & 1u){
         for(std::size_t i = 0; i != n; ++i){
            ptrs[i] -= 1;
         }
      }
      else{
         for(std::size_t i = 0; i != n; ++i){
            ptrs[i] += 1;
         }
      }
      clobber();
   }
   return sw.nsecs();
}

template<class Traits>
nanosecond_type test_store(std::size_t n, std::size_t reps)
{
   typedef node<Traits>                               node_t;
   typedef typename Traits::template ptr<node_t>::type ptr_t;

   std::vector<node_t> nodes(n);
   std::vector<ptr_t>  ptrs(n);
   node_t *const raw = &nodes[0];

   escape(&ptrs[0]);
   stopwatch sw;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         ptrs[i] = raw + i;
      }
      //Without the barrier some compilers remove every repetition but the
      //last one, because nothing reads what the previous ones wrote
      clobber();
   }
   return sw.nsecs();
}

//!Assignment of a null pointer, the one value that every conversion has to
//!treat as a special case
template<class Traits>
nanosecond_type test_assign_null(std::size_t n, std::size_t reps)
{
   typedef node<Traits>                                node_t;
   typedef typename Traits::template ptr<node_t>::type ptr_t;

   std::vector<ptr_t>  ptrs(n);

   escape(&ptrs[0]);
   stopwatch sw;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         ptrs[i] = 0;
      }
      clobber();
   }
   return sw.nsecs();
}

template<class Traits>
nanosecond_type test_copy(std::size_t n, std::size_t reps)
{
   typedef node<Traits>                               node_t;
   typedef typename Traits::template ptr<node_t>::type ptr_t;

   std::vector<node_t> nodes(n);
   std::vector<ptr_t>  src(n);
   std::vector<ptr_t>  dst(n);
   node_t *const raw = &nodes[0];
   for(std::size_t i = 0; i != n; ++i){
      src[i] = raw + i;
   }

   escape(&src[0]);
   escape(&dst[0]);
   stopwatch sw;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         dst[i] = src[i];
      }
      clobber();   //See test_store
   }
   return sw.nsecs();
}

//!Copy construction. A placement new is used so that what is measured is a
//!construction and not an assignment over a live object
template<class Traits>
nanosecond_type test_copy_ctor(std::size_t n, std::size_t reps)
{
   typedef node<Traits>                                node_t;
   typedef typename Traits::template ptr<node_t>::type ptr_t;

   std::vector<node_t> nodes(n);
   std::vector<ptr_t>  src(n);
   std::vector<ptr_t>  dst(n);
   node_t *const raw = &nodes[0];
   for(std::size_t i = 0; i != n; ++i){
      src[i] = raw + i;
   }

   escape(&src[0]);
   escape(&dst[0]);
   stopwatch sw;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         ::new (static_cast<void*>(&dst[i])) ptr_t(src[i]);
      }
      clobber();
   }
   return sw.nsecs();
}

//!Conversion to a pointer to const, the address preserving conversion that
//!containers use between pointer, const_pointer and void_pointer
template<class Traits>
nanosecond_type test_convert(std::size_t n, std::size_t reps)
{
   typedef node<Traits>                                      node_t;
   typedef typename Traits::template ptr<node_t>::type       ptr_t;
   typedef typename Traits::template const_ptr<node_t>::type cptr_t;

   std::vector<node_t> nodes(n);
   std::vector<ptr_t>  src(n);
   std::vector<cptr_t> dst(n);
   node_t *const raw = &nodes[0];
   for(std::size_t i = 0; i != n; ++i){
      src[i] = raw + i;
   }

   escape(&src[0]);
   escape(&dst[0]);
   stopwatch sw;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         dst[i] = src[i];
      }
      clobber();   //See test_store
   }
   return sw.nsecs();
}

//!Conversion to a base class. Unlike the conversion to a pointer to const
//!this one moves the address, so it can not keep the offset and has to go
//!through a raw pointer
template<class Traits>
nanosecond_type test_upcast(std::size_t n, std::size_t reps)
{
   typedef typename Traits::template ptr<up_derived>::type dptr_t;
   typedef typename Traits::template ptr<up_base_b>::type  bptr_t;

   std::vector<up_derived> objs(n);
   std::vector<dptr_t>     src(n);
   std::vector<bptr_t>     dst(n);
   up_derived *const raw = &objs[0];
   for(std::size_t i = 0; i != n; ++i){
      src[i] = raw + i;
   }

   escape(&src[0]);
   escape(&dst[0]);
   stopwatch sw;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         dst[i] = src[i];
      }
      clobber();
   }
   return sw.nsecs();
}

//!Casts a void pointer back to a typed pointer, which is what every
//!allocator does on each allocation
template<class Traits>
nanosecond_type test_cast(std::size_t n, std::size_t reps)
{
   typedef node<Traits>                                node_t;
   typedef typename Traits::template ptr<node_t>::type ptr_t;
   typedef typename Traits::void_ptr                   void_ptr;

   std::vector<node_t>   nodes(n);
   std::vector<void_ptr> src(n);
   std::vector<ptr_t>    dst(n);
   node_t *const raw = &nodes[0];
   for(std::size_t i = 0; i != n; ++i){
      src[i] = raw + i;
   }

   escape(&src[0]);
   escape(&dst[0]);
   stopwatch sw;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         dst[i] = Traits::template pointer_cast<node_t>(src[i]);
      }
      clobber();   //See test_store
   }
   return sw.nsecs();
}

//!const_pointer_cast, which always keeps the address and so needs no null
//!pointer test
template<class Traits>
nanosecond_type test_const_cast(std::size_t n, std::size_t reps)
{
   typedef node<Traits>                                      node_t;
   typedef typename Traits::template ptr<node_t>::type       ptr_t;
   typedef typename Traits::template const_ptr<node_t>::type cptr_t;

   std::vector<node_t> nodes(n);
   std::vector<cptr_t> src(n);
   std::vector<ptr_t>  dst(n);
   node_t *const raw = &nodes[0];
   for(std::size_t i = 0; i != n; ++i){
      src[i] = raw + i;
   }

   escape(&src[0]);
   escape(&dst[0]);
   stopwatch sw;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         dst[i] = Traits::template const_cast_ptr<node_t>(src[i]);
      }
      clobber();
   }
   return sw.nsecs();
}

//!reinterpret_pointer_cast, which also always keeps the address
template<class Traits>
nanosecond_type test_reinterp_cast(std::size_t n, std::size_t reps)
{
   typedef node<Traits>                                     node_t;
   typedef typename Traits::template ptr<node_t>::type      ptr_t;
   typedef typename Traits::template ptr<std::size_t>::type sptr_t;

   std::vector<node_t> nodes(n);
   std::vector<ptr_t>  src(n);
   std::vector<sptr_t> dst(n);
   node_t *const raw = &nodes[0];
   for(std::size_t i = 0; i != n; ++i){
      src[i] = raw + i;
   }

   escape(&src[0]);
   escape(&dst[0]);
   stopwatch sw;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         dst[i] = Traits::template reinterpret_cast_ptr<std::size_t>(src[i]);
      }
      clobber();
   }
   return sw.nsecs();
}

//!dynamic_pointer_cast. Its cost is dominated by the run time type
//!information lookup, which both pointers pay alike
template<class Traits>
nanosecond_type test_dynamic_cast(std::size_t n, std::size_t reps)
{
   typedef typename Traits::template ptr<dyn_base>::type    bptr_t;
   typedef typename Traits::template ptr<dyn_derived>::type dptr_t;

   std::vector<dyn_derived> objs(n);
   std::vector<bptr_t>      src(n);
   std::vector<dptr_t>      dst(n);
   dyn_derived *const raw = &objs[0];
   for(std::size_t i = 0; i != n; ++i){
      src[i] = static_cast<dyn_base*>(raw + i);
   }

   escape(&src[0]);
   escape(&dst[0]);
   stopwatch sw;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         dst[i] = Traits::template dynamic_cast_ptr<dyn_derived>(src[i]);
      }
      clobber();
   }
   return sw.nsecs();
}

//!pointer_to, which pointer_traits uses to build a pointer from a reference
template<class Traits>
nanosecond_type test_pointer_to(std::size_t n, std::size_t reps)
{
   typedef node<Traits>                                node_t;
   typedef typename Traits::template ptr<node_t>::type ptr_t;

   std::vector<node_t> nodes(n);
   std::vector<ptr_t>  dst(n);

   escape(&nodes[0]);
   escape(&dst[0]);
   stopwatch sw;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         dst[i] = Traits::address_of(nodes[i]);
      }
      clobber();   //See test_store
   }
   return sw.nsecs();
}

//!get(), the conversion back to a raw pointer with its null pointer test.
//!to_raw_pointer and operator-> go through the same code
template<class Traits>
nanosecond_type test_get(std::size_t n, std::size_t reps)
{
   typedef node<Traits>                                node_t;
   typedef typename Traits::template ptr<node_t>::type ptr_t;

   std::vector<node_t>  nodes(n);
   std::vector<ptr_t>   src(n);
   std::vector<node_t*> dst(n);
   node_t *const raw = &nodes[0];
   for(std::size_t i = 0; i != n; ++i){
      src[i] = raw + i;
   }

   escape(&src[0]);
   escape(&dst[0]);
   stopwatch sw;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         dst[i] = Traits::to_raw(src[i]);
      }
      clobber();
   }
   return sw.nsecs();
}

template<class Traits>
nanosecond_type test_nullcheck(std::size_t n, std::size_t reps)
{
   typedef node<Traits>                               node_t;
   typedef typename Traits::template ptr<node_t>::type ptr_t;

   std::vector<node_t> nodes(n);
   std::vector<ptr_t>  ptrs(n);
   node_t *const raw = &nodes[0];
   xorshift rng(777u);
   for(std::size_t i = 0; i != n; ++i){
      ptrs[i] = (rng.next() & 3u) ? (raw + i) : 0;
   }

   escape(&ptrs[0]);
   stopwatch sw;
   std::size_t sum = 0;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         sum += !ptrs[i] ? 0u : 1u;
      }
      clobber();
   }
   const nanosecond_type ns = sw.nsecs();
   g_sink += sum;
   return ns;
}

template<class Traits>
nanosecond_type test_compare(std::size_t n, std::size_t reps)
{
   typedef node<Traits>                               node_t;
   typedef typename Traits::template ptr<node_t>::type ptr_t;

   std::vector<node_t> nodes(n);
   std::vector<ptr_t>  ptrs(n);
   node_t *const raw = &nodes[0];
   for(std::size_t i = 0; i != n; ++i){
      ptrs[i] = raw + i;
   }
   const ptr_t needle(raw + n/2);

   escape(&ptrs[0]);
   stopwatch sw;
   std::size_t sum = 0;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         sum += (ptrs[i] == needle) ? 1u : 0u;
      }
      clobber();
   }
   const nanosecond_type ns = sw.nsecs();
   g_sink += sum;
   return ns;
}

//!Equality against a fixed raw pointer, the mixed comparison that needs the
//!offset_ptr operand to be converted
template<class Traits>
nanosecond_type test_compare_raw(std::size_t n, std::size_t reps)
{
   typedef node<Traits>                                node_t;
   typedef typename Traits::template ptr<node_t>::type ptr_t;

   std::vector<node_t> nodes(n);
   std::vector<ptr_t>  ptrs(n);
   node_t *const raw = &nodes[0];
   for(std::size_t i = 0; i != n; ++i){
      ptrs[i] = raw + i;
   }
   node_t *const needle = raw + n/2;

   escape(&ptrs[0]);
   stopwatch sw;
   std::size_t sum = 0;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         sum += (ptrs[i] == needle) ? 1u : 0u;
      }
      clobber();
   }
   const nanosecond_type ns = sw.nsecs();
   g_sink += sum;
   return ns;
}

//!Ordering against a fixed pointer
template<class Traits>
nanosecond_type test_less(std::size_t n, std::size_t reps)
{
   typedef node<Traits>                                node_t;
   typedef typename Traits::template ptr<node_t>::type ptr_t;

   std::vector<node_t> nodes(n);
   std::vector<ptr_t>  ptrs(n);
   node_t *const raw = &nodes[0];
   for(std::size_t i = 0; i != n; ++i){
      ptrs[i] = raw + i;
   }
   const ptr_t needle(raw + n/2);

   escape(&ptrs[0]);
   stopwatch sw;
   std::size_t sum = 0;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         sum += (ptrs[i] < needle) ? 1u : 0u;
      }
      clobber();
   }
   const nanosecond_type ns = sw.nsecs();
   g_sink += sum;
   return ns;
}

//!swap between two pointers
template<class Traits>
nanosecond_type test_swap(std::size_t n, std::size_t reps)
{
   typedef node<Traits>                                node_t;
   typedef typename Traits::template ptr<node_t>::type ptr_t;
   using std::swap;

   std::vector<node_t> nodes(n);
   std::vector<ptr_t>  a(n), b(n);
   node_t *const raw = &nodes[0];
   for(std::size_t i = 0; i != n; ++i){
      a[i] = raw + i;
      b[i] = raw + (n - 1 - i);
   }

   escape(&a[0]);
   escape(&b[0]);
   stopwatch sw;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         swap(a[i], b[i]);
      }
      clobber();   //See test_store
   }
   return sw.nsecs();
}

template<class Traits>
nanosecond_type test_sort(std::size_t n, std::size_t reps)
{
   typedef node<Traits>                               node_t;
   typedef typename Traits::template ptr<node_t>::type ptr_t;

   std::vector<node_t> nodes(n);
   node_t *const raw = &nodes[0];

   std::vector<std::size_t> order(n);
   xorshift rng(4242u);
   for(std::size_t i = 0; i != n; ++i){
      order[i] = i;
   }
   for(std::size_t i = n; i > 1; --i){
      std::swap(order[i-1], order[rng.below(i)]);
   }

   std::vector<ptr_t> ptrs(n);
   escape(&ptrs[0]);
   nanosecond_type total = 0;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         ptrs[i] = raw + order[i];
      }
      clobber();
      stopwatch sw;
      std::sort(ptrs.begin(), ptrs.end());
      clobber();
      total += sw.nsecs();
   }
   return total;
}

template<class Traits>
nanosecond_type test_difference(std::size_t n, std::size_t reps)
{
   typedef node<Traits>                               node_t;
   typedef typename Traits::template ptr<node_t>::type ptr_t;

   std::vector<node_t> nodes(n);
   std::vector<ptr_t>  ptrs(n);
   node_t *const raw = &nodes[0];
   for(std::size_t i = 0; i != n; ++i){
      ptrs[i] = raw + i;
   }
   const ptr_t base(raw);

   escape(&ptrs[0]);
   stopwatch sw;
   std::size_t sum = 0;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         sum += std::size_t(ptrs[i] - base);
      }
      clobber();
   }
   const nanosecond_type ns = sw.nsecs();
   g_sink += sum;
   return ns;
}

//////////////////////////////////////////////////////////////////////////////
//
//                                 Runner
//
//////////////////////////////////////////////////////////////////////////////

struct config
{
   //Cache resident array size, so the cost shown is the pointer, not the memory
   std::size_t     elements;

   //Repetitions over that array. The calibration raises it up to 'min_ns'
   std::size_t     reps;

   //Measurements per test. The smallest one is reported
   std::size_t     runs;

   //Array size of the random chase, big enough to defeat the cache and the TLB
   std::size_t     big_elements;

   //Repetitions of the random chase. One of them already costs a cache miss
   //per node, so it needs a much lower count than 'reps'
   std::size_t     big_reps;

   //Shortest accepted measurement, below it the clock and the scheduler win
   nanosecond_type min_ns;

   //Largest factor the calibration may apply to 'reps' and 'big_reps'
   std::size_t     max_scale;
};

//!Array size and initial repetitions of a single test. Only the number of
//!repetitions changes between a short and a long run, so both measure the
//!same working set and their results can be compared
inline void test_params(std::size_t t, const config &cfg, std::size_t &elements, std::size_t &reps)
{
   if(t == t_chase_rand){
      elements = cfg.big_elements;
      reps     = cfg.big_reps;
   }
   else if(t == t_sort){
      //Sorting is O(n log n) per repetition, so it needs far fewer of them
      elements = cfg.elements;
      reps     = cfg.reps/8u + 1u;
   }
   else{
      elements = cfg.elements;
      reps     = cfg.reps;
   }
}

//!Runs one test once and returns the measured time
template<class Traits>
nanosecond_type run_one(std::size_t t, std::size_t n, std::size_t reps)
{
   nanosecond_type ns = 0;
   switch(t){
      case t_chase_seq:
         ns = test_chase<Traits>(n, reps, false);
      break;
      case t_chase_rand:
         ns = test_chase<Traits>(n, reps, true);
      break;
      case t_iterate:
         ns = test_iterate<Traits>(n, reps);
      break;
      case t_deref:
         ns = test_deref<Traits>(n, reps);
      break;
      case t_index:
         ns = test_index<Traits>(n, reps);
      break;
      case t_postinc:
         ns = test_postinc<Traits>(n, reps);
      break;
      case t_postdec:
         ns = test_postdec<Traits>(n, reps);
      break;
      case t_add:
         ns = test_add<Traits>(n, reps);
      break;
      case t_sub:
         ns = test_sub<Traits>(n, reps);
      break;
      case t_addassign:
         ns = test_addassign<Traits>(n, reps);
      break;
      case t_store:
         ns = test_store<Traits>(n, reps);
      break;
      case t_assign_null:
         ns = test_assign_null<Traits>(n, reps);
      break;
      case t_copy:
         ns = test_copy<Traits>(n, reps);
      break;
      case t_copy_ctor:
         ns = test_copy_ctor<Traits>(n, reps);
      break;
      case t_convert:
         ns = test_convert<Traits>(n, reps);
      break;
      case t_upcast:
         ns = test_upcast<Traits>(n, reps);
      break;
      case t_cast:
         ns = test_cast<Traits>(n, reps);
      break;
      case t_const_cast:
         ns = test_const_cast<Traits>(n, reps);
      break;
      case t_reinterp_cast:
         ns = test_reinterp_cast<Traits>(n, reps);
      break;
      case t_dynamic_cast:
         ns = test_dynamic_cast<Traits>(n, reps);
      break;
      case t_pointer_to:
         ns = test_pointer_to<Traits>(n, reps);
      break;
      case t_get:
         ns = test_get<Traits>(n, reps);
      break;
      case t_nullcheck:
         ns = test_nullcheck<Traits>(n, reps);
      break;
      case t_compare:
         ns = test_compare<Traits>(n, reps);
      break;
      case t_compare_raw:
         ns = test_compare_raw<Traits>(n, reps);
      break;
      case t_less:
         ns = test_less<Traits>(n, reps);
      break;
      case t_swap:
         ns = test_swap<Traits>(n, reps);
      break;
      case t_sort:
         ns = test_sort<Traits>(n, reps);
      break;
      case t_difference:
         ns = test_difference<Traits>(n, reps);
      break;
      default:
      break;
   }
   return ns;
}

template<class Traits>
void run_all(const config &cfg, double (&out)[t_count])
{
   for(std::size_t t = 0; t != t_count; ++t){
      std::size_t n = 0, reps = 0;
      test_params(t, cfg, n, reps);

      //A first, discarded measurement warms the caches and the branch
      //predictors up, and tells how many repetitions a measurement needs to
      //last at least cfg.min_ns. Without that floor the result depends on the
      //resolution of the clock and on how the process is scheduled
      nanosecond_type ns = run_one<Traits>(t, n, reps);
      for(std::size_t scale = 1u; ns < cfg.min_ns && scale != cfg.max_scale; ){
         std::size_t factor = ns ? std::size_t(cfg.min_ns/ns) + 1u : 8u;
         if(factor > 8u){
            factor = 8u;
         }
         if(scale*factor > cfg.max_scale){
            factor = cfg.max_scale/scale;
         }
         if(factor < 2u){
            break;
         }
         scale *= factor;
         reps  *= factor;
         ns = run_one<Traits>(t, n, reps);
      }

      //The minimum of several measurements is the estimate that is least
      //polluted by other activity in the machine
      nanosecond_type best = ns;
      for(std::size_t run = 1u; run != cfg.runs; ++run){
         ns = run_one<Traits>(t, n, reps);
         if(ns < best){
            best = ns;
         }
      }
      const std::size_t ops = n*reps;
      out[t] = double(best)/double(ops ? ops : 1u);
   }
}

void print_report(const double (&raw)[t_count], const double (&off)[t_count])
{
   const int name_w = 15;
   const int col_w  = 12;

   std::cout << std::left << std::setw(name_w) << "test"
             << std::right << std::setw(col_w) << "raw ptr"
             << std::setw(col_w) << "offset_ptr"
             << std::setw(col_w) << "ratio" << "\n"
             << std::setfill('-') << std::setw(name_w + col_w*3) << ""
             << std::setfill(' ') << "\n";

   for(std::size_t t = 0; t != t_count; ++t){
      std::cout << std::left  << std::setw(name_w) << test_name[t]
                << std::right << std::fixed
                << std::setprecision(3) << std::setw(col_w) << raw[t]
                << std::setw(col_w) << off[t]
                << std::setprecision(2) << std::setw(col_w)
                << (raw[t] > 0.0 ? off[t]/raw[t] : 0.0) << '\n';
   }
   //The geometric mean is the right average for a set of ratios: no single
   //test dominates the summary and the result does not depend on which of
   //the two pointers is taken as the reference
   double logsum = 0.0;
   std::size_t counted = 0;
   for(std::size_t t = 0; t != t_count; ++t){
      if(raw[t] > 0.0 && off[t] > 0.0){
         logsum += std::log(off[t]/raw[t]);
         ++counted;
      }
   }
   const double geomean = counted ? std::exp(logsum/double(counted)) : 0.0;

   std::cout << std::setfill('-') << std::setw(name_w + col_w*3) << ""
             << std::setfill(' ') << "\n"
             << std::left  << std::setw(name_w) << "geomean"
             << std::right << std::setw(col_w) << ""
             << std::setw(col_w) << ""
             << std::fixed << std::setprecision(2) << std::setw(col_w)
             << geomean << "\n";

   std::cout << "\n(ns per operation, and offset_ptr cost relative to a raw"
                " pointer; lower is better)\n";
}

}  //namespace bench {

int main(int argc, char *argv[])
{
   using namespace bench;

   config cfg;
   //The working set is the same in both runs, only the confidence changes
   cfg.elements     = 64*1024;      //~1 MB of nodes on 64 bit
   cfg.big_elements = 2*1024*1024;  //~32 MB of nodes on 64 bit
#if defined(LONG_BENCH)
   cfg.reps         = 128;
   cfg.runs         = 7;
   cfg.big_reps     = 4;
   cfg.min_ns       = nanosecond_type(100)*1000000;  //100 ms
   cfg.max_scale    = 64;
#else
   cfg.reps         = 16;
   cfg.runs         = 3;
   cfg.big_reps     = 1;
   cfg.min_ns       = nanosecond_type(5)*1000000;    //5 ms
   cfg.max_scale    = 8;
#endif

   if(argc > 1){
      cfg.elements = std::size_t(std::atol(argv[1]));
   }
   if(argc > 2){
      cfg.reps = std::size_t(std::atol(argv[2]));
   }

   std::cout << "boost::interprocess::offset_ptr benchmark"
#if defined(LONG_BENCH)
             << " (LONG_BENCH)\n"
#else
             << " (short run; rebuild with -DLONG_BENCH for the full one)\n"
#endif
             << "  sizeof(offset_ptr<char>): "
             << sizeof(boost::interprocess::offset_ptr<char>) << "\n"
             << "  elements/reps/runs:       "
             << cfg.elements << '/' << cfg.reps << '/' << cfg.runs << "\n"
             << "  random chase elements:    " << cfg.big_elements << "\n"
             << "  minimum time/measurement: "
             << double(cfg.min_ns)/1000000.0 << " ms\n\n";

   double raw[t_count];
   double off[t_count];
   run_all<raw_traits>(cfg, raw);
   run_all<offset_traits>(cfg, off);
   print_report(raw, off);

   //Make sure the accumulated values are observable
   if(g_sink == 0xDEADBEEFu){
      std::cout << "unreachable\n";
   }
   return 0;
}
