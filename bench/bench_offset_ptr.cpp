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

#ifdef _MSC_VER
#pragma warning (disable : 4512)
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
};

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
   t_store,          //raw pointer -> offset_ptr conversion (stores)
   t_copy,           //offset_ptr -> offset_ptr conversion (copies)
   t_convert,        //conversion to a pointer to const, address preserving
   t_cast,           //static_pointer_cast from a void pointer, as allocators do
   t_pointer_to,     //pointer_to, used by pointer_traits in every container
   t_nullcheck,      //null tests
   t_compare,        //equality against a fixed pointer
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
   , "store raw"
   , "copy ptr"
   , "convert"
   , "cast void*"
   , "pointer_to"
   , "null check"
   , "compare"
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

   stopwatch sw;
   std::size_t sum = 0;
   for(std::size_t r = 0; r != reps; ++r){
      node_ptr p(head);
      while(p){
         sum += p->value;
         p = p->next;
      }
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

   stopwatch sw;
   std::size_t sum = 0;
   for(std::size_t r = 0; r != reps; ++r){
      node_ptr p(&nodes[0]);
      const node_ptr e(&nodes[0] + n);
      for(; p != e; ++p){
         sum += p->value;
      }
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

   stopwatch sw;
   std::size_t sum = 0;
   for(std::size_t r = 0; r != reps; ++r){
      node_ptr p(&nodes[0]);
      const node_ptr e(&nodes[0] + n);
      for(; p != e; ++p){
         sum += (*p).value;
      }
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

   stopwatch sw;
   std::size_t sum = 0;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         sum += base[std::ptrdiff_t(i)].value;
      }
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

   stopwatch sw;
   std::size_t sum = 0;
   for(std::size_t r = 0; r != reps; ++r){
      node_ptr p(&nodes[0]);
      const node_ptr e(&nodes[0] + n);
      while(p != e){
         sum += (p++)->value;
      }
   }
   const nanosecond_type ns = sw.nsecs();
   g_sink += sum;
   return ns;
}

template<class Traits>
nanosecond_type test_store(std::size_t n, std::size_t reps)
{
   typedef node<Traits>                               node_t;
   typedef typename Traits::template ptr<node_t>::type ptr_t;

   std::vector<node_t> nodes(n);
   std::vector<ptr_t>  ptrs(n);
   node_t *const raw = &nodes[0];

   stopwatch sw;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         ptrs[i] = raw + i;
      }
      //Consume one element per repetition, otherwise some compilers
      //remove every repetition but the last one
      g_sink += std::size_t(!ptrs[r % n]);
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

   stopwatch sw;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         dst[i] = src[i];
      }
      //See test_store
      g_sink += std::size_t(!dst[r % n]);
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

   stopwatch sw;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         dst[i] = src[i];
      }
      //See test_store
      g_sink += std::size_t(!dst[r % n]);
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

   stopwatch sw;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         dst[i] = Traits::template pointer_cast<node_t>(src[i]);
      }
      //See test_store
      g_sink += std::size_t(!dst[r % n]);
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

   stopwatch sw;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         dst[i] = Traits::address_of(nodes[i]);
      }
      //See test_store
      g_sink += std::size_t(!dst[r % n]);
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

   stopwatch sw;
   std::size_t sum = 0;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         sum += !ptrs[i] ? 0u : 1u;
      }
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

   stopwatch sw;
   std::size_t sum = 0;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         sum += (ptrs[i] == needle) ? 1u : 0u;
      }
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

   stopwatch sw;
   std::size_t sum = 0;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         sum += (ptrs[i] < needle) ? 1u : 0u;
      }
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

   stopwatch sw;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         swap(a[i], b[i]);
      }
      //See test_store
      g_sink += std::size_t(!a[r % n]);
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
   nanosecond_type total = 0;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         ptrs[i] = raw + order[i];
      }
      stopwatch sw;
      std::sort(ptrs.begin(), ptrs.end());
      total += sw.nsecs();
      g_sink += std::size_t(ptrs[0] != ptr_t());
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

   stopwatch sw;
   std::size_t sum = 0;
   for(std::size_t r = 0; r != reps; ++r){
      for(std::size_t i = 0; i != n; ++i){
         sum += std::size_t(ptrs[i] - base);
      }
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
   std::size_t elements;
   std::size_t reps;
   std::size_t runs;        //measurements per test, the minimum is kept
   std::size_t big_elements;//for the cache-hostile test
   std::size_t big_reps;
};

template<class Traits>
void run_all(const config &cfg, double (&out)[t_count])
{
   for(std::size_t t = 0; t != t_count; ++t){
      nanosecond_type best = nanosecond_type(-1);
      std::size_t ops = 0;
      for(std::size_t run = 0; run != cfg.runs; ++run){
         nanosecond_type ns = 0;
         switch(t){
            case t_chase_seq:
               ns = test_chase<Traits>(cfg.elements, cfg.reps, false);
               ops = cfg.elements*cfg.reps;
            break;
            case t_chase_rand:
               ns = test_chase<Traits>(cfg.big_elements, cfg.big_reps, true);
               ops = cfg.big_elements*cfg.big_reps;
            break;
            case t_iterate:
               ns = test_iterate<Traits>(cfg.elements, cfg.reps);
               ops = cfg.elements*cfg.reps;
            break;
            case t_deref:
               ns = test_deref<Traits>(cfg.elements, cfg.reps);
               ops = cfg.elements*cfg.reps;
            break;
            case t_index:
               ns = test_index<Traits>(cfg.elements, cfg.reps);
               ops = cfg.elements*cfg.reps;
            break;
            case t_postinc:
               ns = test_postinc<Traits>(cfg.elements, cfg.reps);
               ops = cfg.elements*cfg.reps;
            break;
            case t_store:
               ns = test_store<Traits>(cfg.elements, cfg.reps);
               ops = cfg.elements*cfg.reps;
            break;
            case t_copy:
               ns = test_copy<Traits>(cfg.elements, cfg.reps);
               ops = cfg.elements*cfg.reps;
            break;
            case t_convert:
               ns = test_convert<Traits>(cfg.elements, cfg.reps);
               ops = cfg.elements*cfg.reps;
            break;
            case t_cast:
               ns = test_cast<Traits>(cfg.elements, cfg.reps);
               ops = cfg.elements*cfg.reps;
            break;
            case t_pointer_to:
               ns = test_pointer_to<Traits>(cfg.elements, cfg.reps);
               ops = cfg.elements*cfg.reps;
            break;
            case t_nullcheck:
               ns = test_nullcheck<Traits>(cfg.elements, cfg.reps);
               ops = cfg.elements*cfg.reps;
            break;
            case t_compare:
               ns = test_compare<Traits>(cfg.elements, cfg.reps);
               ops = cfg.elements*cfg.reps;
            break;
            case t_less:
               ns = test_less<Traits>(cfg.elements, cfg.reps);
               ops = cfg.elements*cfg.reps;
            break;
            case t_swap:
               ns = test_swap<Traits>(cfg.elements, cfg.reps);
               ops = cfg.elements*cfg.reps;
            break;
            case t_sort:
               ns = test_sort<Traits>(cfg.elements, cfg.reps/8 + 1);
               ops = cfg.elements*(cfg.reps/8 + 1);
            break;
            case t_difference:
               ns = test_difference<Traits>(cfg.elements, cfg.reps);
               ops = cfg.elements*cfg.reps;
            break;
            default:
            break;
         }
         if(ns < best){
            best = ns;
         }
      }
      out[t] = double(best)/double(ops ? ops : 1u);
   }
}

void print_report(const double (&raw)[t_count], const double (&off)[t_count])
{
   const int name_w = 12;
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
   cfg.elements     = 64*1024;      //~1 MB of nodes on 64 bit
   cfg.reps         = 128;
   cfg.runs         = 5;
   cfg.big_elements = 2*1024*1024;  //~32 MB of nodes on 64 bit
   cfg.big_reps     = 2;

   if(argc > 1){
      cfg.elements = std::size_t(std::atol(argv[1]));
   }
   if(argc > 2){
      cfg.reps = std::size_t(std::atol(argv[2]));
   }

   std::cout << "boost::interprocess::offset_ptr benchmark\n"
             << "  sizeof(offset_ptr<char>): "
             << sizeof(boost::interprocess::offset_ptr<char>) << "\n"
             << "  elements/reps/runs:       "
             << cfg.elements << '/' << cfg.reps << '/' << cfg.runs << "\n"
             << "  random chase elements:    " << cfg.big_elements << "\n\n";

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
