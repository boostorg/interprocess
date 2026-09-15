//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2007-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/offset_ptr.hpp>
#include <boost/interprocess/detail/type_traits.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/intrusive/pointer_traits.hpp>
#include <boost/core/lightweight_test.hpp>
#include <boost/cstdint.hpp>

#include <algorithm>   //std::copy, std::sort, std::equal
#include <cstddef>     //std::size_t, std::ptrdiff_t
#include <cstring>     //std::memcpy
#include <iterator>    //std::distance, std::random_access_iterator_tag
#include <sstream>     //std::stringstream

using namespace boost::interprocess;

class Base
{
   int padding;
   public:
   Base() : padding(0){}
   int *get() { return &padding; }
   virtual ~Base(){}
};

class Base2
{
   int padding;
   public:
   Base2() : padding(0){}
   int *get() { return &padding; }
   virtual ~Base2(){}
};

class Base3
{
   int padding;
   public:
   Base3() : padding(0){}
   int *get() { return &padding; }
   virtual ~Base3(){}
};

class Derived
   : public Base
{};

struct dummy_struct
{
   dummy_struct() : value(0) {}
   int value;
   int get_value() const {  return value;  }
};

struct reloc_node
{
   boost::interprocess::offset_ptr<reloc_node> next;
   int value;
};

class Derived3
   : public Base, public Base2, public Base3
{};

class VirtualDerived
   : public virtual Base
{};

class VirtualDerived3
   : public virtual Base, public virtual Base2, public virtual Base3
{};

void test_types_and_conversions()
{
   typedef offset_ptr<int>                pint_t;
   typedef offset_ptr<const int>          pcint_t;
   typedef offset_ptr<volatile int>       pvint_t;
   typedef offset_ptr<const volatile int> pcvint_t;

   BOOST_INTERPROCESS_STATIC_ASSERT((ipcdetail::is_same<pint_t::element_type, int>::value));
   BOOST_INTERPROCESS_STATIC_ASSERT((ipcdetail::is_same<pcint_t::element_type, const int>::value));
   BOOST_INTERPROCESS_STATIC_ASSERT((ipcdetail::is_same<pvint_t::element_type, volatile int>::value));
   BOOST_INTERPROCESS_STATIC_ASSERT((ipcdetail::is_same<pcvint_t::element_type, const volatile int>::value));

   BOOST_INTERPROCESS_STATIC_ASSERT((ipcdetail::is_same<pint_t::value_type,   int>::value));
   BOOST_INTERPROCESS_STATIC_ASSERT((ipcdetail::is_same<pcint_t::value_type,  int>::value));
   BOOST_INTERPROCESS_STATIC_ASSERT((ipcdetail::is_same<pvint_t::value_type,  int>::value));
   BOOST_INTERPROCESS_STATIC_ASSERT((ipcdetail::is_same<pcvint_t::value_type, int>::value));
   int dummy_int = 9;

   {  pint_t pint(&dummy_int);
      pcint_t  pcint(pint);
      BOOST_TEST(pcint.get() == &dummy_int);
   }
   {  pint_t pint(&dummy_int);
      pvint_t  pvint(pint);
      BOOST_TEST(pvint.get() == &dummy_int);
   }
   {  pint_t pint(&dummy_int);
      pcvint_t  pcvint(pint);
      BOOST_TEST(pcvint.get() == &dummy_int);
   }
   {  pcint_t pcint(&dummy_int);
      pcvint_t  pcvint(pcint);
      BOOST_TEST(pcvint.get() == &dummy_int);
   }
   {  pvint_t pvint(&dummy_int);
      pcvint_t  pcvint(pvint);
      BOOST_TEST(pcvint.get() == &dummy_int);
   }

   pint_t   pint(0);
   pcint_t  pcint(0);
   pvint_t  pvint(0);
   pcvint_t pcvint(0);

   {
      pint_t   pint2 = 0;
      pcint_t  pcint2 = 0;
      pvint_t  pvint2 = 0;
      pcvint_t pcvint2 = 0;
      (void)pint2;
      (void)pcint2;
      (void)pvint2;
      (void)pcvint2;
   }

   {
      pint_t   pint2 = op_nullptr_t();
      pcint_t  pcint2 = op_nullptr_t();
      pvint_t  pvint2 = op_nullptr_t();
      pcvint_t pcvint2 = op_nullptr_t();
      (void)pint2;
      (void)pcint2;
      (void)pvint2;
      (void)pcvint2;
   }

   {
      pint_t   pint2((op_nullptr_t()));
      pcint_t  pcint2((op_nullptr_t()));
      pvint_t  pvint2((op_nullptr_t()));
      pcvint_t pcvint2((op_nullptr_t()));
      (void)pint2;
      (void)pcint2;
      (void)pvint2;
      (void)pcvint2;
   }

   pint     = &dummy_int;
   pcint    = &dummy_int;
   pvint    = &dummy_int;
   pcvint   = &dummy_int;

   {  pcint  = pint;
      BOOST_TEST(pcint.get() == &dummy_int);
   }
   {  pvint  = pint;
      BOOST_TEST(pvint.get() == &dummy_int);
   }
   {  pcvint = pint;
      BOOST_TEST(pcvint.get() == &dummy_int);
   }
   {  pcvint = pcint;
      BOOST_TEST(pcvint.get() == &dummy_int);
   }
   {  pcvint = pvint;
      BOOST_TEST(pcvint.get() == &dummy_int);
   }

   BOOST_TEST(pint);

   pint = 0;
   BOOST_TEST(!pint);

   BOOST_TEST(pint == 0);

   BOOST_TEST(0 == pint);

   pint = &dummy_int;
   BOOST_TEST(0 != pint);

   pcint = &dummy_int;

   BOOST_TEST( (pcint - pint) == 0);
   BOOST_TEST( (pint - pcint) == 0);

   typedef offset_ptr<void>                pvoid_t;
   typedef offset_ptr<const void>          pcvoid_t;
   typedef offset_ptr<volatile void>       pvvoid_t;
   typedef offset_ptr<const volatile void> pcvvoid_t;
   {
      pvoid_t   pvoid  = pint;
      pcvoid_t  pcvoid = pcint;
      pvvoid_t  pvvoid = pvint;
      pcvvoid_t pcvvoid = pcvint;
      (void)pvoid;
      (void)pcvoid;
      (void)pvvoid;
      (void)pcvvoid;
   }
   {
      pvoid_t   pvoid(pint);
      pcvoid_t  pcvoid(pcint);
      pvvoid_t  pvvoid(pvint);
      pcvvoid_t pcvvoid(pcvint);
      (void)pvoid;
      (void)pcvoid;
      (void)pvvoid;
      (void)pcvvoid;
   }
   {
      pvoid_t   pvoid;
      pcvoid_t  pcvoid;
      pvvoid_t  pvvoid;
      pcvvoid_t pcvvoid;

      pvoid = pint;
      pcvoid = pcint;
      pvvoid = pvint;
      pcvvoid = pcvint;

      (void)pvoid;
      (void)pcvoid;
      (void)pvvoid;
      (void)pcvvoid;
   }
}

template<class BasePtr, class DerivedPtr>
void test_base_derived_impl()
{
   typename DerivedPtr::element_type d;
   DerivedPtr pderi(&d);

   {  BasePtr pbase2 = pderi;  (void)pbase2; }

   BasePtr pbase(pderi);
   pbase = pderi;
   BOOST_TEST(pbase == pderi);
   BOOST_TEST(!(pbase != pderi));
   BOOST_TEST((pbase - pderi) == 0);
   BOOST_TEST(!(pbase < pderi));
   BOOST_TEST(!(pbase > pderi));
   BOOST_TEST(pbase <= pderi);
   BOOST_TEST((pbase >= pderi));
}

void test_base_derived()
{
   typedef offset_ptr<Base>               pbase_t;
   typedef offset_ptr<const Base>         pcbas_t;
   typedef offset_ptr<Derived>            pderi_t;
   typedef offset_ptr<VirtualDerived>     pvder_t;

   test_base_derived_impl<pbase_t, pderi_t>();
   test_base_derived_impl<pbase_t, pvder_t>();
   test_base_derived_impl<pcbas_t, pderi_t>();
   test_base_derived_impl<pcbas_t, pvder_t>();
}

void test_arithmetic()
{
   typedef offset_ptr<int> pint_t;
   const int NumValues = 5;
   int values[NumValues];

   //Initialize p
   pint_t p = values;
   BOOST_TEST(p.get() == values);

   //Initialize p + NumValues
   pint_t pe = &values[NumValues];
   BOOST_TEST(pe != p);
   BOOST_TEST(pe.get() == &values[NumValues]);

   //ptr - ptr
   BOOST_TEST((pe - p) == NumValues);
   BOOST_TEST((p - pe) == -NumValues);

   //Two null pointers stored at different addresses subtract to zero, like
   //two null raw pointers do
   BOOST_TEST((pint_t() - pint_t()) == 0);

   //ptr - integer
   BOOST_TEST((pe - NumValues) == p);

   //ptr + integer
   BOOST_TEST((p + NumValues) == pe);

   //integer + ptr
   BOOST_TEST((NumValues + p) == pe);

   //indexing
   BOOST_TEST(pint_t(&p[NumValues]) == pe);
   BOOST_TEST(pint_t(&pe[-NumValues]) == p);

   //ptr -= integer
   pint_t p0 = pe;
   p0-= NumValues;
   BOOST_TEST(p == p0);

   //ptr += integer
   pint_t penew = p0;
   penew += NumValues;
   BOOST_TEST(penew == pe);

   //++ptr
   penew = p0;
   for(int j = 0; j != NumValues; ++j, ++penew);
   BOOST_TEST(penew == pe);

   //--ptr
   p0 = pe;
   for(int j = 0; j != NumValues; ++j, --p0);
   BOOST_TEST(p == p0);

   //ptr++
   penew = p0;
   for(int j = 0; j != NumValues; ++j){
      pint_t p_new_copy = penew;
      BOOST_TEST(p_new_copy == penew++);
   }
   //ptr--
   p0 = pe;
   for(int j = 0; j != NumValues; ++j){
      pint_t p0_copy = p0;
      BOOST_TEST(p0_copy == p0--);
   }
}

void test_comparison()
{
   typedef offset_ptr<int> pint_t;
   const int NumValues = 5;
   int values[NumValues];

   //Initialize p
   pint_t p = values;
   BOOST_TEST(p.get() == values);

   //Initialize p + NumValues
   pint_t pe = &values[NumValues];
   BOOST_TEST(pe != p);

   BOOST_TEST(pe.get() == &values[NumValues]);

   //operators
   BOOST_TEST(p != pe);
   BOOST_TEST(p == p);
   BOOST_TEST((p < pe));
   BOOST_TEST((p <= pe));
   BOOST_TEST((pe > p));
   BOOST_TEST((pe >= p));
}

bool test_pointer_traits()
{
   typedef offset_ptr<int> OInt;
   typedef boost::intrusive::pointer_traits< OInt > PTOInt;
   BOOST_INTERPROCESS_STATIC_ASSERT((ipcdetail::is_same<PTOInt::element_type, int>::value));
   BOOST_INTERPROCESS_STATIC_ASSERT((ipcdetail::is_same<PTOInt::pointer, OInt >::value));
   BOOST_INTERPROCESS_STATIC_ASSERT((ipcdetail::is_same<PTOInt::difference_type, OInt::difference_type >::value));
   BOOST_INTERPROCESS_STATIC_ASSERT((ipcdetail::is_same<PTOInt::rebind_pointer<double>::type, offset_ptr<double> >::value));
   int dummy;
   OInt oi(&dummy);
   if(boost::intrusive::pointer_traits<OInt>::pointer_to(dummy) != oi){
      return false;
   }
   return true;
}

struct node 
{
   offset_ptr<node> next;
};

void test_pointer_plus_bits()
{
   BOOST_INTERPROCESS_STATIC_ASSERT((boost::intrusive::max_pointer_plus_bits< offset_ptr<void>, boost::move_detail::alignment_of<node>::value >::value >= 1U));
   typedef boost::intrusive::pointer_plus_bits< offset_ptr<node>, 1u > ptr_plus_bits;

   node n, n2;
   offset_ptr<node> pnode(&n);

   BOOST_TEST(ptr_plus_bits::get_pointer(pnode) == &n);
   BOOST_TEST(0 == ptr_plus_bits::get_bits(pnode));
   ptr_plus_bits::set_bits(pnode, 1u);
   BOOST_TEST(1 == ptr_plus_bits::get_bits(pnode));
   BOOST_TEST(ptr_plus_bits::get_pointer(pnode) == &n);

   ptr_plus_bits::set_pointer(pnode, &n2);
   BOOST_TEST(ptr_plus_bits::get_pointer(pnode) == &n2);
   BOOST_TEST(1 == ptr_plus_bits::get_bits(pnode));
   ptr_plus_bits::set_bits(pnode, 0u);
   BOOST_TEST(0 == ptr_plus_bits::get_bits(pnode));
   BOOST_TEST(ptr_plus_bits::get_pointer(pnode) == &n2);

   ptr_plus_bits::set_pointer(pnode, offset_ptr<node>());
   BOOST_TEST(ptr_plus_bits::get_pointer(pnode) ==0);
   BOOST_TEST(0 == ptr_plus_bits::get_bits(pnode));
   ptr_plus_bits::set_bits(pnode, 1u);
   BOOST_TEST(1 == ptr_plus_bits::get_bits(pnode));
   BOOST_TEST(ptr_plus_bits::get_pointer(pnode) == 0);
}

void test_cast()
{
   typedef offset_ptr<int>                pint_t;
   typedef offset_ptr<const int>          pcint_t;
   typedef offset_ptr<volatile int>       pvint_t;
   typedef offset_ptr<const volatile int> pcvint_t;
   typedef offset_ptr<void>                pvoid_t;
   typedef offset_ptr<const void>          pcvoid_t;
   typedef offset_ptr<volatile void>       pvvoid_t;
   typedef offset_ptr<const volatile void> pcvvoid_t;

   int dummy_int = 9;

   {  pint_t   pint(&dummy_int);
      pcint_t  pcint(pint);
      pvint_t  pvint = pint;
      pcvint_t pcvint = pvint;

      pvoid_t  pvoid = pint;
      pcvoid_t pcvoid = pvoid;
      pvvoid_t pvvoid = pvoid;
      pcvvoid_t pcvvoid = pvoid;
      pcvvoid = pvvoid;

      //Test valid static_cast conversions required by Allocator::pointer 
      //requirements (void_pointer -> pointer, const_void_pointer -> const_pointer)
      pint  = static_cast<pint_t>(pvoid);
      pcint = static_cast<pcint_t>(pcvoid);

      BOOST_TEST(pint == pvoid);
      BOOST_TEST(pcint == pcvoid);
      BOOST_TEST(pcint == pint);

      //Test valid static_pointer_cast conversions
      {
         pint   = static_pointer_cast<int>(pvoid);
         pcint  = static_pointer_cast<const int>(pcvoid);
         pvint  = static_pointer_cast<volatile int>(pvoid);
         pcvint = static_pointer_cast<const volatile int>(pvoid);

         BOOST_TEST(pint == pvoid);
         BOOST_TEST(pcint == pcvoid);
         BOOST_TEST(pcint == pint);
         BOOST_TEST(pvint == pint);
         BOOST_TEST(pcvint == pint);

         Derived d;
         offset_ptr<Derived> pd(&d);
         offset_ptr<Base> pb;
         //Downcast
         pb = static_pointer_cast<Base>(pd);
         //Upcast
         pd = static_pointer_cast<Derived>(pb);

         Derived3 d3;
         offset_ptr<Derived3> pd3(&d3);
         offset_ptr<Base3> pb3;

         //Downcast
         pb3 = static_pointer_cast<Base3>(pd3);
         //Upcast
         pd3 = static_pointer_cast<Derived3>(pb3);
         //Test addresses don't match in multiple inheritance
         BOOST_TEST((pvoid_t)pb3 != (pvoid_t)pd3);
         BOOST_TEST(pb3.get() == static_cast<Base3*>(pd3.get()));
      }

      //Test valid const_pointer_cast conversions
      {
         pint = const_pointer_cast<int>(pcint);
         pint = const_pointer_cast<int>(pvint);
         pint = const_pointer_cast<int>(pcvint);

         pvint = const_pointer_cast<volatile int>(pcint);
         pvint = const_pointer_cast<volatile int>(pcvint);

         pcint = const_pointer_cast<const int>(pvint);
         pcint = const_pointer_cast<const int>(pcvint);

         //Test valid reinterpret_pointer_cast conversions
         pint   = reinterpret_pointer_cast<int>(pvoid);
         pcint  = reinterpret_pointer_cast<const int>(pcvoid);
         pvint  = reinterpret_pointer_cast<volatile int>(pvoid);
         pcvint = reinterpret_pointer_cast<const volatile int>(pvoid);
      }

      //Test valid dynamic_pointer_cast conversions
      {
         {
            Derived3 d3;
            offset_ptr<Derived3> pd3(&d3);
            offset_ptr<Base2> pb2;

            //Downcast
            pb2 = dynamic_pointer_cast<Base2>(pd3);
            //Upcast
            pd3 = dynamic_pointer_cast<Derived3>(pb2);
            BOOST_TEST((pvoid_t)pb2 != (pvoid_t)pd3);
            BOOST_TEST(pb2.get() == dynamic_cast<Base2*>(&d3));
            BOOST_TEST(static_cast<void*>(pb2.get()) != static_cast<void*>(pd3.get()));
         }
         {
            VirtualDerived3 vd3;
            offset_ptr<VirtualDerived3> pdv3(&vd3);
            offset_ptr<Base3> pb3;
            offset_ptr<Base2> pb2;
            offset_ptr<Base>  pb;

            //Downcast
            pb3 = dynamic_pointer_cast<Base3>(pdv3);
            pb2 = dynamic_pointer_cast<Base2>(pdv3);
            pb  = dynamic_pointer_cast<Base> (pdv3);
            //Upcast
            pdv3 = dynamic_pointer_cast<VirtualDerived3>(pb);
            pdv3 = dynamic_pointer_cast<VirtualDerived3>(pb2);
            pdv3 = dynamic_pointer_cast<VirtualDerived3>(pb3);
            //Test addresses don't match in multiple inheritance
            BOOST_TEST((pvoid_t)pb2 != (pvoid_t)pdv3);
            BOOST_TEST(pb2.get() != static_cast<void*>(pdv3.get()));
            BOOST_TEST((pvoid_t)pb3 != (pvoid_t)pdv3);
            BOOST_TEST(pb3.get() != static_cast<void*>(pdv3.get()));
         }
      }
   }
}


//////////////////////////////////////////////////////////////////////////////
//
//                Nested types and pointer/iterator traits
//
//////////////////////////////////////////////////////////////////////////////

//ipcdetail::is_same can't be used with reference types
template<class T, class U>
struct op_is_same
{  static const bool value = false;  };

template<class T>
struct op_is_same<T, T>
{  static const bool value = true;   };

void test_nested_types()
{
   typedef offset_ptr<int>  pint_t;
   typedef offset_ptr<void> pvoid_t;

   BOOST_INTERPROCESS_STATIC_ASSERT((op_is_same<pint_t::element_type, int>::value));
   BOOST_INTERPROCESS_STATIC_ASSERT((op_is_same<pint_t::pointer, int*>::value));
   BOOST_INTERPROCESS_STATIC_ASSERT((op_is_same<pint_t::reference, int&>::value));
   BOOST_INTERPROCESS_STATIC_ASSERT((op_is_same<pint_t::difference_type, std::ptrdiff_t>::value));
   BOOST_INTERPROCESS_STATIC_ASSERT((op_is_same<pint_t::offset_type, uintptr_t>::value));
   BOOST_INTERPROCESS_STATIC_ASSERT
      ((op_is_same<pint_t::iterator_category, std::random_access_iterator_tag>::value));

   //offset_ptr<void> has no real reference type
   BOOST_INTERPROCESS_STATIC_ASSERT((op_is_same<pvoid_t::value_type, void>::value));

   //Only the offset is stored
   BOOST_INTERPROCESS_STATIC_ASSERT(sizeof(pint_t) == sizeof(pint_t::offset_type));

   //rebind
   #if defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)
   BOOST_INTERPROCESS_STATIC_ASSERT
      ((ipcdetail::is_same<pint_t::rebind<double>::other, offset_ptr<double> >::value));
   #else
   BOOST_INTERPROCESS_STATIC_ASSERT
      ((ipcdetail::is_same<pint_t::rebind<double>, offset_ptr<double> >::value));
   BOOST_INTERPROCESS_STATIC_ASSERT((ipcdetail::is_same<pint_t::other, pint_t>::value));
   #endif

   //Backwards compatibility with pointer_to_other
   BOOST_INTERPROCESS_STATIC_ASSERT
      ((ipcdetail::is_same<boost::pointer_to_other<pint_t, double>::type, offset_ptr<double> >::value));

   //Optimization traits
   BOOST_INTERPROCESS_STATIC_ASSERT((boost::has_trivial_destructor<pint_t>::value));
   BOOST_INTERPROCESS_STATIC_ASSERT((boost::move_detail::is_trivially_destructible<pint_t>::value));
}

//////////////////////////////////////////////////////////////////////////////
//
//                     Offset encoding and null pointer
//
//////////////////////////////////////////////////////////////////////////////

void test_offset_encoding()
{
   typedef offset_ptr<int>             pint_t;
   typedef pint_t::offset_type         offset_type;

   int dummy_int = 0;
   pint_t p(&dummy_int);

   //A non-null offset_ptr stores the distance between itself and the pointee
   const offset_type expected = offset_type
      ( reinterpret_cast<const char*>(&dummy_int) - reinterpret_cast<const char*>(&p) );
   BOOST_TEST(p.get_offset() == expected);
   BOOST_TEST(p.get() == &dummy_int);

   //A null offset_ptr stores 1
   const pint_t null_p;
   BOOST_TEST(null_p.get_offset() == offset_type(1));
   BOOST_TEST(null_p.get() == 0);

   //Assigning null restores the reserved offset
   p = 0;
   BOOST_TEST(p.get_offset() == offset_type(1));
}

void test_null()
{
   typedef offset_ptr<int> pint_t;
   int dummy_int = 0;

   //Default constructed
   pint_t p;
   BOOST_TEST(!p);
   BOOST_TEST(p.get() == 0);
   BOOST_TEST(p == pint_t());
   BOOST_TEST(!(p != pint_t()));
   BOOST_TEST(p == static_cast<int*>(0));
   BOOST_TEST(static_cast<int*>(0) == p);

   //Assignment from and to null
   p = &dummy_int;
   BOOST_TEST(!!p);
   p = 0;
   BOOST_TEST(!p);

   #if !defined(BOOST_NO_CXX11_NULLPTR)
   {
      pint_t pn(nullptr);
      BOOST_TEST(!pn);
      pn = &dummy_int;
      BOOST_TEST(pn);
      pn = nullptr;
      BOOST_TEST(!pn);
      BOOST_TEST(pn == nullptr);
      BOOST_TEST(nullptr == pn);
   }
   #endif

   //A null offset_ptr stays null when it is copied to another address
   pint_t arr[4];
   for(std::size_t i = 0; i != sizeof(arr)/sizeof(arr[0]); ++i){
      BOOST_TEST(!arr[i]);
   }
   pint_t copy(arr[0]);
   BOOST_TEST(!copy);
   copy = arr[3];
   BOOST_TEST(!copy);
   BOOST_TEST(arr[0] == arr[3]);

   //Null is preserved by address-preserving and address-changing conversions
   {
      offset_ptr<const int> pc(p);
      BOOST_TEST(!pc);
      offset_ptr<void> pv(p);
      BOOST_TEST(!pv);

      offset_ptr<VirtualDerived> pd;
      offset_ptr<Base> pb(pd);
      BOOST_TEST(!pb);
      pb = pd;
      BOOST_TEST(!pb);
      BOOST_TEST(pb == pd);
   }

   //operator-> is documented to return a null pointer for a null offset_ptr
   {
      const offset_ptr<dummy_struct> pnull;
      BOOST_TEST(pnull.operator->() == 0);
   }
}

//////////////////////////////////////////////////////////////////////////////
//
//                              Dereference
//
//////////////////////////////////////////////////////////////////////////////

void test_dereference()
{
   dummy_struct d;
   d.value = 42;

   offset_ptr<dummy_struct> p(&d);
   BOOST_TEST(&*p == &d);
   BOOST_TEST((*p).value == 42);
   BOOST_TEST(p->value == 42);
   BOOST_TEST(p->get_value() == 42);
   BOOST_TEST(p.operator->() == &d);

   p->value = 7;
   BOOST_TEST(d.value == 7);
   (*p).value = 8;
   BOOST_TEST(d.value == 8);

   const offset_ptr<const dummy_struct> cp(&d);
   BOOST_TEST(cp->value == 8);
   BOOST_TEST((*cp).value == 8);

   //Indexing, including negative indexes
   dummy_struct arr[3];
   arr[0].value = 10;
   arr[1].value = 11;
   arr[2].value = 12;

   offset_ptr<dummy_struct> pa(arr);
   BOOST_TEST(pa[0].value == 10);
   BOOST_TEST(pa[2].value == 12);
   pa[1].value = 21;
   BOOST_TEST(arr[1].value == 21);

   offset_ptr<dummy_struct> plast(&arr[2]);
   BOOST_TEST(plast[-2].value == 10);
   BOOST_TEST(&plast[-1] == &arr[1]);
}

//////////////////////////////////////////////////////////////////////////////
//
//                      Assignment corner cases
//
//////////////////////////////////////////////////////////////////////////////

void test_assignment()
{
   int a = 0, b = 0;
   offset_ptr<int> pa(&a), pb(&b);

   //Assignment between offset_ptrs stored at different addresses
   pb = pa;
   BOOST_TEST(pb.get() == &a);
   BOOST_TEST(pa.get() == &a);

   //Self assignment (through a reference)
   offset_ptr<int> &pa_ref = pa;
   pa = pa_ref;
   BOOST_TEST(pa.get() == &a);

   //Copy construction from an offset_ptr stored at a different address
   offset_ptr<int> pc(pa);
   BOOST_TEST(pc.get() == &a);

   //Assignment from a raw pointer
   pc = &b;
   BOOST_TEST(pc.get() == &b);

   //An offset_ptr can point to another offset_ptr
   offset_ptr<offset_ptr<int> > ppa(&pa);
   BOOST_TEST(ppa->get() == &a);
   BOOST_TEST((*ppa).get() == &a);
}

void test_swap()
{
   int a = 0, b = 0;
   offset_ptr<int> pa(&a), pb(&b);

   swap(pa, pb);  //ADL
   BOOST_TEST(pa.get() == &b);
   BOOST_TEST(pb.get() == &a);

   swap(pa, pb);
   BOOST_TEST(pa.get() == &a);
   BOOST_TEST(pb.get() == &b);

   //Swapping with a null pointer
   offset_ptr<int> pnull;
   swap(pa, pnull);
   BOOST_TEST(!pa);
   BOOST_TEST(pnull.get() == &a);
   swap(pa, pnull);
   BOOST_TEST(pa.get() == &a);
   BOOST_TEST(!pnull);

   //Self swap
   swap(pa, pa);
   BOOST_TEST(pa.get() == &a);
}

//////////////////////////////////////////////////////////////////////////////
//
//                    Comparisons against raw pointers
//
//////////////////////////////////////////////////////////////////////////////

void test_raw_pointer_comparison()
{
   const int NumValues = 5;
   int values[NumValues];
   int *const raw0 = &values[0];
   int *const rawe = &values[NumValues];
   const offset_ptr<int> p0(raw0), pe(rawe);

   BOOST_TEST(p0 == raw0);    BOOST_TEST(raw0 == p0);
   BOOST_TEST(p0 != rawe);    BOOST_TEST(rawe != p0);
   BOOST_TEST(p0 <  rawe);    BOOST_TEST(raw0 <  pe);
   BOOST_TEST(p0 <= rawe);    BOOST_TEST(raw0 <= pe);
   BOOST_TEST(pe >  raw0);    BOOST_TEST(rawe >  p0);
   BOOST_TEST(pe >= raw0);    BOOST_TEST(rawe >= p0);
   BOOST_TEST(p0 <= raw0);    BOOST_TEST(raw0 <= p0);
   BOOST_TEST(p0 >= raw0);    BOOST_TEST(raw0 >= p0);
   BOOST_TEST(!(p0 >  raw0)); BOOST_TEST(!(raw0 >  p0));
   BOOST_TEST(!(p0 <  raw0)); BOOST_TEST(!(raw0 <  p0));
}

//////////////////////////////////////////////////////////////////////////////
//
//                          Stream insertion
//
//////////////////////////////////////////////////////////////////////////////

void test_stream_io()
{
   int dummy_int = 0;
   const offset_ptr<int> p(&dummy_int);

   //operator<< writes the stored offset, operator>> reads it back
   std::stringstream ss;
   ss << p;
   offset_ptr<int> q;
   ss >> q;
   BOOST_TEST(q.get_offset() == p.get_offset());

   //A null pointer is written as the reserved offset
   std::stringstream ss2;
   ss2 << offset_ptr<int>();
   BOOST_TEST(ss2.str() == "1");
}

//////////////////////////////////////////////////////////////////////////////
//
//                            to_raw_pointer
//
//////////////////////////////////////////////////////////////////////////////

void test_to_raw_pointer()
{
   int dummy_int = 0;
   const offset_ptr<int> p(&dummy_int);
   BOOST_TEST(boost::interprocess::to_raw_pointer(p) == &dummy_int);
   BOOST_TEST(ipcdetail::to_raw_pointer(p) == &dummy_int);

   const offset_ptr<int> null_p;
   BOOST_TEST(boost::interprocess::to_raw_pointer(null_p) == 0);
}

void test_pointer_to()
{
   int dummy_int = 0;
   const offset_ptr<int> p = offset_ptr<int>::pointer_to(dummy_int);
   BOOST_TEST(p.get() == &dummy_int);
}

//////////////////////////////////////////////////////////////////////////////
//
//                          offset_ptr<void>
//
//////////////////////////////////////////////////////////////////////////////

void test_void_pointer()
{
   int dummy_int = 0;

   offset_ptr<void> pv(&dummy_int);
   BOOST_TEST(pv.get() == static_cast<void*>(&dummy_int));
   BOOST_TEST(!!pv);

   offset_ptr<void> pv2;
   BOOST_TEST(!pv2);
   BOOST_TEST(pv != pv2);

   pv2 = pv;
   BOOST_TEST(pv == pv2);
   BOOST_TEST(!(pv < pv2));
   BOOST_TEST(pv <= pv2);
   BOOST_TEST(pv >= pv2);

   //void_pointer -> pointer, as required by Allocator::pointer
   const offset_ptr<int> pi(static_cast<offset_ptr<int> >(pv));
   BOOST_TEST(pi.get() == &dummy_int);

   //Comparison against raw void pointers
   BOOST_TEST(pv == static_cast<void*>(&dummy_int));
   BOOST_TEST(static_cast<void*>(&dummy_int) == pv);
}

//////////////////////////////////////////////////////////////////////////////
//
//                   Non-default OffsetType/OffsetAlignment
//
//////////////////////////////////////////////////////////////////////////////

void test_custom_offset_type()
{
   typedef offset_ptr<int,  std::ptrdiff_t, boost::uint64_t, 8u> p64_t;
   typedef offset_ptr<void, std::ptrdiff_t, boost::uint64_t, 8u> pv64_t;

   BOOST_INTERPROCESS_STATIC_ASSERT((ipcdetail::is_same<p64_t::offset_type, boost::uint64_t>::value));
   BOOST_INTERPROCESS_STATIC_ASSERT(sizeof(p64_t) >= sizeof(boost::uint64_t));
   BOOST_INTERPROCESS_STATIC_ASSERT(boost::move_detail::alignment_of<p64_t>::value >= 8u);

   const int NumValues = 4;
   int values[NumValues];

   p64_t p(values);
   const p64_t pe(&values[NumValues]);
   BOOST_TEST(p.get() == values);
   BOOST_TEST((pe - p) == NumValues);

   p += 2;
   BOOST_TEST(p.get() == &values[2]);
   --p;
   BOOST_TEST(p.get() == &values[1]);

   const p64_t null_p;
   BOOST_TEST(!null_p);
   BOOST_TEST(null_p.get_offset() == boost::uint64_t(1));
   BOOST_TEST(null_p.get() == 0);

   //Conversions keep working with a non-default offset type
   const pv64_t pv(p);
   BOOST_TEST(pv.get() == static_cast<void*>(&values[1]));
   BOOST_TEST(static_pointer_cast<int>(pv) == p);
}

//////////////////////////////////////////////////////////////////////////////
//
//    Relocation: A block of memory holding offset_ptrs that point
//    inside the block can be copied to a different address
//
//////////////////////////////////////////////////////////////////////////////

void test_relocation()
{
   const std::size_t NumNodes = 4;
   reloc_node src[NumNodes];
   reloc_node dst[NumNodes];

   for(std::size_t i = 0; i != NumNodes; ++i){
      src[i].value = int(i);
      src[i].next  = (i+1 == NumNodes) ? 0 : &src[i+1];
   }

   //Raw copy of the whole block to a different address
   std::memcpy(static_cast<void*>(&dst[0]), static_cast<const void*>(&src[0]), sizeof(src));

   //The copied pointers now walk the destination block
   std::size_t count = 0;
   for(reloc_node *p = &dst[0]; p; p = p->next.get(), ++count){
      BOOST_TEST(p == &dst[count]);
      BOOST_TEST(p->value == int(count));
   }
   BOOST_TEST(count == NumNodes);

   //A null offset_ptr is still null after the relocation
   BOOST_TEST(!dst[NumNodes-1].next);

   //And the source block is untouched
   BOOST_TEST(src[0].next.get() == &src[1]);
}

//////////////////////////////////////////////////////////////////////////////
//
//                 offset_ptr as a random access iterator
//
//////////////////////////////////////////////////////////////////////////////

void test_iterator_use()
{
   const int NumValues = 5;
   int values[NumValues];
   values[0] = 4; values[1] = 2; values[2] = 5; values[3] = 1; values[4] = 3;
   int other[NumValues];

   const offset_ptr<int> first(values), last(&values[NumValues]);
   BOOST_TEST(std::distance(first, last) == NumValues);

   std::copy(first, last, offset_ptr<int>(other));
   BOOST_TEST(std::equal(first, last, offset_ptr<int>(other)));

   offset_ptr<int> ofirst(other), olast(&other[NumValues]);
   std::sort(ofirst, olast);
   for(int i = 0; i != NumValues-1; ++i){
      BOOST_TEST(ofirst[i] <= ofirst[i+1]);
   }
   BOOST_TEST(ofirst[0] == 1);
   BOOST_TEST(ofirst[NumValues-1] == 5);
}

int main()
{
   test_types_and_conversions();
   test_base_derived();
   test_arithmetic();
   test_comparison();
   test_pointer_traits();
   test_pointer_plus_bits();
   test_cast();
   test_nested_types();
   test_offset_encoding();
   test_null();
   test_dereference();
   test_assignment();
   test_swap();
   test_raw_pointer_comparison();
   test_stream_io();
   test_to_raw_pointer();
   test_pointer_to();
   test_void_pointer();
   test_custom_offset_type();
   test_relocation();
   test_iterator_use();
   return ::boost::report_errors();
}
