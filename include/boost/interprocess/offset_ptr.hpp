//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2015. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_OFFSET_PTR_HPP
#define BOOST_INTERPROCESS_OFFSET_PTR_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/move/detail/type_traits.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/interprocess_printers.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/cast_tags.hpp>
#include <boost/interprocess/detail/mpl.hpp>
#include <boost/container/detail/type_traits.hpp>  //alignment_of, aligned_storage
#include <boost/assert.hpp>
#include <iosfwd>
#include <cstddef>

#if defined(BOOST_GCC) && (BOOST_GCC >= 40700)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

//!\file
//!Describes a smart pointer that stores the offset between this pointer and
//!target pointee, called offset_ptr.

namespace boost {

#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

//Predeclarations
template <class T>
struct has_trivial_destructor;

#endif   //#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

namespace interprocess {

#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

#if !defined( BOOST_NO_CXX11_NULLPTR )
   typedef decltype(nullptr) op_nullptr_t;
#else
struct op_nullptr_t
{
   void* lx;  //to achieve pointer alignment

   struct nat {int bool_conversion;};

   BOOST_INTERPROCESS_FORCEINLINE op_nullptr_t() {}

   BOOST_INTERPROCESS_FORCEINLINE op_nullptr_t(int nat::*)  {}

   BOOST_INTERPROCESS_FORCEINLINE operator int nat::*() const {  return 0;   }

   template <class T>
   BOOST_INTERPROCESS_FORCEINLINE operator T*() const {  return 0;   }

   template <class T, class U>
   BOOST_INTERPROCESS_FORCEINLINE operator T U::* () const {  return 0;   }

   friend BOOST_INTERPROCESS_FORCEINLINE bool operator==(op_nullptr_t, op_nullptr_t) {   return true;   }
   friend BOOST_INTERPROCESS_FORCEINLINE bool operator!=(op_nullptr_t, op_nullptr_t) {   return false;  }
};

#endif

namespace ipcdetail {

   //workarounds for void offset_ptrs
   struct op_nat{};

   template <class T> struct op_reference
      : add_reference<T>
   {};

   template <> struct op_reference<void>
   {  typedef op_nat type;   };

   template <> struct op_reference<void const>
   {  typedef op_nat type;   };

   template <> struct op_reference<void volatile>
   {  typedef op_nat type;   };

   template <> struct op_reference<void const volatile>
   {  typedef op_nat type;   };

   template<class OffsetType, std::size_t OffsetAlignment>
   union offset_ptr_internal
   {
      BOOST_INTERPROCESS_STATIC_ASSERT(sizeof(OffsetType) >= sizeof(uintptr_t));
      BOOST_INTERPROCESS_STATIC_ASSERT(boost::move_detail::is_integral<OffsetType>::value && boost::move_detail::is_unsigned<OffsetType>::value);

      BOOST_INTERPROCESS_FORCEINLINE explicit offset_ptr_internal(OffsetType off)
         : m_offset(off)
      {}

      OffsetType m_offset; //Distance between this object and pointee address

      typename ::boost::container::dtl::aligned_storage
         < sizeof(OffsetType)//for offset_type_alignment m_offset will be enough
         , (OffsetAlignment == offset_type_alignment) ? 1u : OffsetAlignment
         >::type alignment_helper;
   };


   //The conversions between a pointer and the stored offset must treat the
   //null pointer as a special case. Branchless implementations, which replace
   //that test with an arithmetic mask, are used by default because they are
   //usually faster.


   ////////////////////////////////////////////////////////////////////////
   //
   //                         offset_ptr_launder
   //
   ////////////////////////////////////////////////////////////////////////

   //!Makes an offset opaque to the optimizer before it is converted back to
   //!a pointer.
   //!
   //!Conversions between a pointer and an integer are implementation-
   //!defined ([expr.reinterpret.cast]) and GCC defines them as (GCC manual,
   //!"Implementation-defined behavior / Arrays and pointers"):
   //!
   //!   "When casting from pointer to integer and back again, the resulting
   //!    pointer must reference the same object as the original pointer,
   //!    otherwise the behavior is undefined. That is, one may not use integer
   //!    arithmetic to avoid the undefined behavior of pointer arithmetic as
   //!    proscribed in C99 and C11 6.5.6/8."
   //!
   //!Every sum that becomes a pointer to the pointee needs this.
   //!
   //!Clang, MSVC and EDG based compilers do not use the provenance this way,
   //!and the barrier costs them vectorization, so it is applied only to GCC.
   template <class OffsetType>
   BOOST_INTERPROCESS_FORCEINLINE OffsetType offset_ptr_launder(OffsetType off)
   {
      #if defined(BOOST_GCC) && !defined(BOOST_INTERPROCESS_OFFSET_PTR_NO_LAUNDER)
      __asm__("" : "+r"(off));
      #endif
      return off;
   }

   ////////////////////////////////////////////////////////////////////////
   //
   //                      offset_ptr_to_raw_pointer
   //
   ////////////////////////////////////////////////////////////////////////
   #if !defined(BOOST_INTERPROCESS_OFFSET_PTR_NO_BRANCHLESS) && \
       !defined(BOOST_INTERPROCESS_OFFSET_PTR_NO_BRANCHLESS_TO_PTR)
   #define BOOST_INTERPROCESS_OFFSET_PTR_BRANCHLESS_TO_PTR
   #endif
   template <class OffsetType>
   BOOST_INTERPROCESS_FORCEINLINE void * offset_ptr_to_raw_pointer(const volatile void *this_ptr, OffsetType offset)
   {
      typedef pointer_offset_caster<void*, OffsetType> caster_t;
      #ifndef BOOST_INTERPROCESS_OFFSET_PTR_BRANCHLESS_TO_PTR
         if(offset == 1){
            return 0;
         }
         else{
            return caster_t(offset_ptr_launder(caster_t(this_ptr).offset() + offset)).pointer();
         }
      #else
         //The mask is written as ~(0 - (x == k)) and not as the equivalent
         //"m = (x == k); --m;" because only the first form is recognized as
         //a select (cmov)
         const OffsetType mask = ~(OffsetType(0) - OffsetType(offset == 1));
         OffsetType target_offset = caster_t(this_ptr).offset() + offset;
         target_offset &= mask;
         return caster_t(offset_ptr_launder(target_offset)).pointer();
      #endif
   }

   ////////////////////////////////////////////////////////////////////////
   //
   //                 offset_ptr_to_raw_pointer_unchecked
   //
   ////////////////////////////////////////////////////////////////////////

   //!Same as offset_ptr_to_raw_pointer, but the null pointer test is omitted.
   //!The caller must guarantee that the offset does not represent a null
   //!pointer, which is the case in every operation whose precondition is
   //!already a dereferenceable pointer.
   template <class OffsetType>
   BOOST_INTERPROCESS_FORCEINLINE void * offset_ptr_to_raw_pointer_unchecked(const volatile void *this_ptr, OffsetType offset)
   {
      typedef pointer_offset_caster<void*, OffsetType> caster_t;
      BOOST_ASSERT(offset != 1);
      return caster_t(offset_ptr_launder(caster_t(this_ptr).offset() + offset)).pointer();
   }

   ////////////////////////////////////////////////////////////////////////
   //
   //                      offset_ptr_to_offset
   //
   ////////////////////////////////////////////////////////////////////////
   #if !defined(BOOST_INTERPROCESS_OFFSET_PTR_NO_BRANCHLESS) && \
       !defined(BOOST_INTERPROCESS_OFFSET_PTR_NO_BRANCHLESS_TO_OFF)
   #define BOOST_INTERPROCESS_OFFSET_PTR_BRANCHLESS_TO_OFF
   #endif
   template<class OffsetType>
   BOOST_INTERPROCESS_FORCEINLINE OffsetType offset_ptr_to_offset(const volatile void *ptr, const volatile void *this_ptr)
   {
      typedef pointer_offset_caster<void*, OffsetType> caster_t;
      #ifndef BOOST_INTERPROCESS_OFFSET_PTR_BRANCHLESS_TO_OFF
         //offset == 1 && ptr != 0 is not legal for this pointer
         if(!ptr){
            return 1;
         }
         else{
            OffsetType offset = caster_t(ptr).offset()- caster_t(this_ptr).offset();
            BOOST_ASSERT(offset != 1);
            return offset;
         }
      #else
         OffsetType offset = caster_t(ptr).offset() - caster_t(this_ptr).offset();
         --offset;
         //The mask is written as ~(0 - (x == k)) and not as the equivalent
         //"m = (x == k); --m;" because only the first form is recognized as
         //a select (cmov)
         const OffsetType mask = ~(OffsetType(0) - OffsetType(ptr == 0));
         offset &= mask;
         return ++offset;
      #endif
   }

   ////////////////////////////////////////////////////////////////////////
   //
   //                     offset_ptr_to_offset_unchecked
   //
   ////////////////////////////////////////////////////////////////////////

   //!Same as offset_ptr_to_offset, but the null pointer test is omitted. The
   //!caller must guarantee that 'ptr' is not null, which is the case when it
   //!comes from a reference.
   template<class OffsetType>
   BOOST_INTERPROCESS_FORCEINLINE OffsetType offset_ptr_to_offset_unchecked(const volatile void *ptr, const volatile void *this_ptr)
   {
      typedef pointer_offset_caster<void*, OffsetType> caster_t;
      BOOST_ASSERT(ptr != 0);
      #if defined(BOOST_GCC) && (BOOST_GCC < 40900)
      //GCC 4.8.1 miscompiles the bare difference of the two addresses at -O1
      //and above. GCC 4.7 and 4.9 are not affected.
      OffsetType ptr_off = caster_t(ptr).offset();
      __asm__("" : "+r"(ptr_off));
      const OffsetType offset = ptr_off - caster_t(this_ptr).offset();
      #else
      const OffsetType offset = caster_t(ptr).offset() - caster_t(this_ptr).offset();
      #endif
      BOOST_ASSERT(offset != 1);
      return offset;
   }

   ////////////////////////////////////////////////////////////////////////
   //
   //                      offset_ptr_to_offset_from_other
   //
   ////////////////////////////////////////////////////////////////////////
   #if !defined(BOOST_INTERPROCESS_OFFSET_PTR_NO_BRANCHLESS) && \
       !defined(BOOST_INTERPROCESS_OFFSET_PTR_NO_BRANCHLESS_TO_OFF_FROM_OTHER)
   #define BOOST_INTERPROCESS_OFFSET_PTR_BRANCHLESS_TO_OFF_FROM_OTHER
   #endif
   template<class OffsetType>
   BOOST_INTERPROCESS_FORCEINLINE OffsetType offset_ptr_to_offset_from_other
      (const volatile void *this_ptr, const volatile void *other_ptr, OffsetType other_offset)
   {
      typedef pointer_offset_caster<void*, OffsetType> caster_t;
      #ifndef BOOST_INTERPROCESS_OFFSET_PTR_BRANCHLESS_TO_OFF_FROM_OTHER
      if(other_offset == 1){
         return 1;
      }
      else{
         OffsetType offset = caster_t(other_ptr).offset() - caster_t(this_ptr).offset() + other_offset;
         BOOST_ASSERT(offset != 1);
         return offset;
      }
      #else
      const OffsetType mask = ~(OffsetType(0) - OffsetType(other_offset == 1));
      OffsetType offset = caster_t(other_ptr).offset() - caster_t(this_ptr).offset();
      offset &= mask;
      return offset + other_offset;
      #endif
   }

   ////////////////////////////////////////////////////////////////////////
   //
   //             offset_ptr_to_offset_from_other_unchecked
   //
   ////////////////////////////////////////////////////////////////////////

   //!Same as offset_ptr_to_offset_from_other, but the null pointer test is
   //!omitted. The caller must guarantee that 'other_offset' does not represent
   //!a null pointer.
   template<class OffsetType>
   BOOST_INTERPROCESS_FORCEINLINE OffsetType offset_ptr_to_offset_from_other_unchecked
      (const volatile void *this_ptr, const volatile void *other_ptr, OffsetType other_offset)
   {
      typedef pointer_offset_caster<void*, OffsetType> caster_t;
      BOOST_ASSERT(other_offset != 1);
      const OffsetType offset = caster_t(other_ptr).offset() - caster_t(this_ptr).offset() + other_offset;
      BOOST_ASSERT(offset != 1);
      return offset;
   }

   ////////////////////////////////////////////////////////////////////////
   //
   // Let's assume casts from/to void and cv casts don't change any target address
   //
   ////////////////////////////////////////////////////////////////////////
   template<class From, class To>
   struct offset_ptr_maintains_address
   {
      static const bool value =    ipcdetail::is_cv_same<From, To>::value
                                || ipcdetail::is_cv_same<void, To>::value
                                || ipcdetail::is_cv_same<void, From>::value
                                || ipcdetail::is_cv_same<char, To>::value
                                ;
   };

   template<class From, class To, class Ret = void>
   struct enable_if_convertible_equal_address
      : enable_if_c< ::boost::move_detail::is_convertible<From*, To*>::value
                     && offset_ptr_maintains_address<From, To>::value
                  , Ret>
   {};

   template<class From, class To, class Ret = void>
   struct enable_if_convertible_unequal_address
      : enable_if_c< ::boost::move_detail::is_convertible<From*, To*>::value
                     && !offset_ptr_maintains_address<From, To>::value
                   , Ret>
   {};

   template <class T, class P>
   struct is_ptr_constructible;

   template <class T, class P>
   struct is_ptr_constructible<T*, P*>
   {
      private:
      template<class U> static U get();

      template <typename U>
      static yes_type test( typename enable_if_c< sizeof( new U*(get<P*>()) ) != 0, int >::type );
         
      template <typename U>
      static no_type test(...);
         
      public:
      static const bool value = sizeof(test<T>(0)) == sizeof(yes_type);
   };

}  //namespace ipcdetail {
#endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

//!A smart pointer that stores the offset between the pointer and the
//!object it points to. This allows special properties, since
//!the pointer is independent from the address of the pointee, if the
//!pointer and the pointee are still separated by the same offset. This feature
//!converts offset_ptr in a smart pointer that can be placed in shared memory and
//!memory mapped files mapped at different addresses in every process.
//!
//! \tparam PointedType The type of the pointee.
//! \tparam DifferenceType A signed integer type that can represent the arithmetic operations on the pointer
//! \tparam OffsetType An unsigned integer type that can represent the
//!   distance between two pointers reinterpret_cast-ed as unsigned integers. This type
//!   should be at least of the same size of std::uintptr_t. In some systems it's possible to communicate
//!   between 32 and 64 bit processes using 64 bit offsets.
//! \tparam OffsetAlignment Alignment of the OffsetType stored inside. In some systems might be necessary
//!   to align it to 64 bits in order to communicate 32 and 64 bit processes using 64 bit offsets.
//!
//!<b>Note</b>: offset_ptr uses implementation defined properties, present in most platforms, for
//!performance reasons:
//!   - Assumes that OffsetType representation of nullptr is (OffsetType)zero.
//!   - Assumes that incrementing a OffsetType obtained from a pointer is equivalent
//!     to incrementing the pointer and then converting it back to OffsetType.
template <class PointedType, class DifferenceType, class OffsetType, std::size_t OffsetAlignment>
class offset_ptr
{
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   typedef offset_ptr<PointedType, DifferenceType, OffsetType, OffsetAlignment>   self_t;
   void unspecified_bool_type_func() const {}
   typedef void (self_t::*unspecified_bool_type)() const;
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   public:
   typedef PointedType                       element_type;
   typedef PointedType *                     pointer;
   typedef typename ipcdetail::
      op_reference<PointedType>::type        reference;

   typedef typename ipcdetail::
      remove_volatile<typename ipcdetail::
         remove_const<PointedType>::type
            >::type                          value_type;
   typedef DifferenceType                    difference_type;
   typedef std::random_access_iterator_tag   iterator_category;
   typedef OffsetType                        offset_type;

   public:   //Public Functions

   //!Default constructor (null pointer).
   //!Never throws.
   BOOST_INTERPROCESS_FORCEINLINE offset_ptr() BOOST_NOEXCEPT
      : internal(1)
   {}

   //!Constructor from nullptr.
   //!Never throws.
   BOOST_INTERPROCESS_FORCEINLINE offset_ptr(op_nullptr_t) BOOST_NOEXCEPT
      : internal(1)
   {}

   #if defined( BOOST_NO_CXX11_NULLPTR )
   //!Constructor from nullptr. Some compilers (e.g. g++14) in C++03 mode have problems with op_nullptr_t
   //!so a helper overload is needed. Never throws.
   BOOST_INTERPROCESS_FORCEINLINE offset_ptr(int ipcdetail::op_nat::*) BOOST_NOEXCEPT
      : internal(1)
   {}
   #endif   //BOOST_NO_CXX11_NULLPTR

   //!Constructor from raw pointer. Only takes part in overload resolution if T* is convertible to PointedType*
   //!Never throws.
   template <class T>
   BOOST_INTERPROCESS_FORCEINLINE offset_ptr( T *ptr
      #ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
             , typename ipcdetail::enable_if< ::boost::move_detail::is_convertible<T*, PointedType*> >::type * = 0
      #endif
      ) BOOST_NOEXCEPT
      : internal(ipcdetail::offset_ptr_to_offset<OffsetType>(static_cast<PointedType*>(ptr), this))
   {}

   //!Constructor from other offset_ptr.
   //!Never throws.
   BOOST_INTERPROCESS_FORCEINLINE offset_ptr(const offset_ptr& ptr) BOOST_NOEXCEPT
      : internal(ipcdetail::offset_ptr_to_offset_from_other(this, &ptr, ptr.internal.m_offset))
   {}

   //!Constructor from other offset_ptr. Only takes part in overload resolution
   //!if T2* is convertible to PointedType*. Never throws.
   template<class T2>
   BOOST_INTERPROCESS_FORCEINLINE offset_ptr( const offset_ptr<T2, DifferenceType, OffsetType, OffsetAlignment> &ptr
             #ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
             , typename ipcdetail::enable_if_convertible_equal_address<T2, PointedType>::type* = 0
             #endif
             ) BOOST_NOEXCEPT
      : internal(ipcdetail::offset_ptr_to_offset_from_other(this, &ptr, ptr.get_offset()))
   {}

   #ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   template<class T2>
   BOOST_INTERPROCESS_FORCEINLINE offset_ptr( const offset_ptr<T2, DifferenceType, OffsetType, OffsetAlignment> &ptr
             , typename ipcdetail::enable_if_convertible_unequal_address<T2, PointedType>::type* = 0) BOOST_NOEXCEPT
      : internal(ipcdetail::offset_ptr_to_offset<OffsetType>(static_cast<PointedType*>(ptr.get()), this))
   {}

   //!Constructor from other offset_ptr available so that static_cast<> works according to Allocator::pointer requirements:
   //!   static_cast<pointer>(void_pointer()) + static_cast<const_pointer>(const_void_pointer())
   //!Discouraged for any other conversion, static_pointer_cast is the way for downcasts and other static_cast-like conversions.
   template<class T2>
   BOOST_INTERPROCESS_FORCEINLINE explicit offset_ptr(const offset_ptr<T2, DifferenceType, OffsetType, OffsetAlignment> &ptr
             #ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
             , typename ipcdetail::enable_if_c< ipcdetail::is_cv_same<T2, void>::value && //Allow only void to something casts for static_cast
                                                !::boost::move_detail::is_convertible<T2*, PointedType*>::value &&
                                                ipcdetail::is_ptr_constructible<T2*, PointedType*>::value
                                              >::type * = 0
             #endif
             ) BOOST_NOEXCEPT //void -> T conversion is address-preserving, so take advantage of that
      : internal(ipcdetail::offset_ptr_to_offset_from_other(this, &ptr, ptr.get_offset()))
   {}

   #endif

   //!Emulates static_cast operator. Only the pointed type can change, the
   //!rest of the template parameters must be the same in both pointers.
   //!Never throws.
   template<class T2>
   BOOST_INTERPROCESS_FORCEINLINE offset_ptr(const offset_ptr<T2, DifferenceType, OffsetType, OffsetAlignment> & r, ipcdetail::static_cast_tag) BOOST_NOEXCEPT
      : internal(priv_static_cast_offset(r, this, ipcdetail::bool_
                  < ipcdetail::offset_ptr_maintains_address<T2, PointedType>::value >()))
   {  //The cast must be valid even when the offset is rebased instead of cast
      (void)static_cast<PointedType*>(static_cast<T2*>(0));
   }

   //!Emulates const_cast operator. Only the pointed type can change, the
   //!rest of the template parameters must be the same in both pointers.
   //!Never throws.
   template<class T2>
   BOOST_INTERPROCESS_FORCEINLINE offset_ptr(const offset_ptr<T2, DifferenceType, OffsetType, OffsetAlignment> & r, ipcdetail::const_cast_tag) BOOST_NOEXCEPT
      : internal(ipcdetail::offset_ptr_to_offset_from_other(this, &r, r.get_offset()))
   {  //A const_cast never changes the address, so only the offset is rebased,
      //but the cast must still be a valid one
      (void)const_cast<PointedType*>(static_cast<T2*>(0));
   }

   //!Emulates dynamic_cast operator. Only the pointed type can change, the
   //!rest of the template parameters must be the same in both pointers.
   //!Never throws.
   template<class T2>
   BOOST_INTERPROCESS_FORCEINLINE offset_ptr(const offset_ptr<T2, DifferenceType, OffsetType, OffsetAlignment> & r, ipcdetail::dynamic_cast_tag) BOOST_NOEXCEPT
      : internal(ipcdetail::offset_ptr_to_offset<OffsetType>(dynamic_cast<PointedType*>(r.get()), this))
   {}

   //!Emulates reinterpret_cast operator. Only the pointed type can change, the
   //!rest of the template parameters must be the same in both pointers.
   //!Never throws.
   template<class T2>
   BOOST_INTERPROCESS_FORCEINLINE offset_ptr(const offset_ptr<T2, DifferenceType, OffsetType, OffsetAlignment> & r, ipcdetail::reinterpret_cast_tag) BOOST_NOEXCEPT
      : internal(ipcdetail::offset_ptr_to_offset_from_other(this, &r, r.get_offset()))
   {}  //A reinterpret_cast never changes the address, so the offset is rebased

   //!Obtains raw pointer from offset.
   //!Never throws.
   BOOST_INTERPROCESS_FORCEINLINE pointer get() const BOOST_NOEXCEPT
   {  return static_cast<pointer>(ipcdetail::offset_ptr_to_raw_pointer(this, this->internal.m_offset));   }

   BOOST_INTERPROCESS_FORCEINLINE offset_type get_offset() const BOOST_NOEXCEPT
   {  return this->internal.m_offset;  }

   //!Pointer-like -> operator. It can return 0 pointer.
   //!Never throws.
   BOOST_INTERPROCESS_FORCEINLINE pointer operator->() const BOOST_NOEXCEPT
   {  return this->get(); }

   //!Dereferencing operator, if it is a null offset_ptr behavior
   //!   is undefined. Never throws.
   BOOST_INTERPROCESS_FORCEINLINE reference operator*() const BOOST_NOEXCEPT
   {
      BOOST_ASSERT(this->internal.m_offset != 1);
      pointer p = static_cast<pointer>
         (ipcdetail::offset_ptr_to_raw_pointer_unchecked(this, this->internal.m_offset));
      reference r = *p;
      return r;
   }

   //!Indexing operator, if it is a null offset_ptr behavior
   //!   is undefined. Never throws.
   BOOST_INTERPROCESS_FORCEINLINE reference operator[](difference_type idx) const BOOST_NOEXCEPT
   {
      BOOST_ASSERT(this->internal.m_offset != 1);
      return static_cast<pointer>
         (ipcdetail::offset_ptr_to_raw_pointer_unchecked(this, this->internal.m_offset))[idx];
   }

   //!Assignment from raw pointer. Only takes part in overload resolution if T* is convertible to PointedType*
   //!Never throws.

   template<class T> BOOST_INTERPROCESS_FORCEINLINE 
   #ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
   typename ipcdetail::enable_if_c
      < ::boost::move_detail::is_convertible<T*, PointedType*>::value, offset_ptr&>::type
   #else
   offset_ptr&
   #endif
      operator= (T *ptr) BOOST_NOEXCEPT
   {
      this->internal.m_offset = ipcdetail::offset_ptr_to_offset<OffsetType>(static_cast<PointedType*>(ptr), this);
      return *this;
   }

   //!Assignment from other offset_ptr.
   //!Never throws.
   BOOST_INTERPROCESS_FORCEINLINE offset_ptr& operator= (const offset_ptr & ptr) BOOST_NOEXCEPT
   {
      this->internal.m_offset = ipcdetail::offset_ptr_to_offset_from_other(this, &ptr, ptr.internal.m_offset);
      return *this;
   }

   //!Assignment from nullptr.
   //!Never throws.
   BOOST_INTERPROCESS_FORCEINLINE offset_ptr& operator= (op_nullptr_t) BOOST_NOEXCEPT
   {
      this->internal.m_offset = 1;
      return *this;
   }

   #if defined( BOOST_NO_CXX11_NULLPTR )
   //!Assignment from nullptr. Some compilers (e.g. g++14) in C++03 mode have problems with op_nullptr_t
   //!so a helper overload is needed. Never throws.
   BOOST_INTERPROCESS_FORCEINLINE offset_ptr& operator= (int ipcdetail::op_nat::*) BOOST_NOEXCEPT
   {
      this->internal.m_offset = 1;
      return *this;
   }
   #endif   //BOOST_NO_CXX11_NULLPTR

   //!Assignment from related offset_ptr.
   //!Only takes part in overload resolution if T2* is convertible to PointedType*
   //!Never throws.
   template<class T2> BOOST_INTERPROCESS_FORCEINLINE 
   #ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
   typename ipcdetail::enable_if_c
      < ::boost::move_detail::is_convertible<T2*, PointedType*>::value, offset_ptr&>::type
   #else
   offset_ptr&
   #endif
      operator= (const offset_ptr<T2, DifferenceType, OffsetType, OffsetAlignment> &ptr) BOOST_NOEXCEPT
   {
      this->assign(ptr, ipcdetail::bool_<ipcdetail::offset_ptr_maintains_address<T2, PointedType>::value>());
      return *this;
   }

   public:

   //!offset_ptr += difference_type.
   //!Never throws.
   BOOST_INTERPROCESS_FORCEINLINE offset_ptr &operator+= (difference_type offset) BOOST_NOEXCEPT
   {  this->inc_offset(offset * difference_type(sizeof(PointedType)));   return *this;  }

   //!offset_ptr -= difference_type.
   //!Never throws.
   BOOST_INTERPROCESS_FORCEINLINE offset_ptr &operator-= (difference_type offset) BOOST_NOEXCEPT
   {  this->dec_offset(offset * difference_type(sizeof(PointedType)));   return *this;  }

   //!++offset_ptr.
   //!Never throws.
   BOOST_INTERPROCESS_FORCEINLINE offset_ptr& operator++ (void) BOOST_NOEXCEPT
   {  this->inc_offset(difference_type(sizeof(PointedType)));   return *this;  }

   //!offset_ptr++.
   //!Never throws.
   BOOST_INTERPROCESS_FORCEINLINE offset_ptr operator++ (int) BOOST_NOEXCEPT
   {
      //Incrementing requires an array, so it can not be null
      offset_ptr tmp;
      tmp.internal.m_offset = ipcdetail::offset_ptr_to_offset_from_other_unchecked
         (&tmp, this, this->internal.m_offset);
      this->inc_offset(sizeof (PointedType));
      return tmp;
   }

   //!--offset_ptr.
   //!Never throws.
   BOOST_INTERPROCESS_FORCEINLINE offset_ptr& operator-- (void) BOOST_NOEXCEPT
   {  this->dec_offset(sizeof (PointedType));   return *this;  }

   //!offset_ptr--.
   //!Never throws.
   BOOST_INTERPROCESS_FORCEINLINE offset_ptr operator-- (int) BOOST_NOEXCEPT
   {
      //Decrementing requires an array, so it can not be null
      offset_ptr tmp;
      tmp.internal.m_offset = ipcdetail::offset_ptr_to_offset_from_other_unchecked
         (&tmp, this, this->internal.m_offset);
      this->dec_offset(sizeof (PointedType));
      return tmp;
   }

   //!safe bool conversion operator.
   //!Never throws.
   #if defined(BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS)
   BOOST_INTERPROCESS_FORCEINLINE operator unspecified_bool_type() const BOOST_NOEXCEPT
   {  return this->internal.m_offset != 1? &self_t::unspecified_bool_type_func : 0;   }
   #else
   BOOST_INTERPROCESS_FORCEINLINE explicit operator bool() const BOOST_NOEXCEPT
   {  return this->internal.m_offset != 1;  }
   #endif
   
   //!Not operator. Not needed in theory, but improves portability.
   //!Never throws
   BOOST_INTERPROCESS_FORCEINLINE bool operator! () const BOOST_NOEXCEPT
   {  return this->internal.m_offset == 1;   }

   //!Compatibility with pointer_traits
   //!
   #if defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)
   template <class U>
   struct rebind
   {  typedef offset_ptr<U, DifferenceType, OffsetType, OffsetAlignment> other;  };
   #else
   template <class U>
   using rebind = offset_ptr<U, DifferenceType, OffsetType, OffsetAlignment>;
   #ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
   typedef offset_ptr<PointedType, DifferenceType, OffsetType, OffsetAlignment> other;
   #endif //BOOST_INTERPROCESS_DOXYGEN_INVOKED
   #endif

   //!Compatibility with pointer_traits
   //!
   BOOST_INTERPROCESS_FORCEINLINE static offset_ptr pointer_to(typename ipcdetail::op_reference<PointedType>::type r) BOOST_NOEXCEPT
   {  //The address of a reference is never null, so the null pointer test of
      //the general conversion is dead work here
      offset_ptr p;
      p.internal.m_offset = ipcdetail::offset_ptr_to_offset_unchecked<OffsetType>(&r, &p);
      return p;
   }

   //!difference_type + offset_ptr
   //!operation
   BOOST_INTERPROCESS_FORCEINLINE friend offset_ptr operator+(difference_type diff, const offset_ptr &right) BOOST_NOEXCEPT
   {  return right.priv_shifted(diff);  }

   //!offset_ptr + difference_type
   //!operation
   BOOST_INTERPROCESS_FORCEINLINE friend offset_ptr operator+(const offset_ptr &left, difference_type diff) BOOST_NOEXCEPT
   {  return left.priv_shifted(diff);  }

   //!offset_ptr - diff
   //!operation
   BOOST_INTERPROCESS_FORCEINLINE friend offset_ptr operator-(const offset_ptr &left, difference_type diff) BOOST_NOEXCEPT
   {  return left.priv_shifted_back(diff);  }

   //!offset_ptr - offset_ptr. Both pointers must point into the
   //!same array, so either both of them are null, or neither of them is.
   //!Never throws.
   BOOST_INTERPROCESS_FORCEINLINE friend difference_type operator-(const offset_ptr &pt, const offset_ptr &pt2) BOOST_NOEXCEPT
   {
      BOOST_ASSERT((pt.internal.m_offset == 1) == (pt2.internal.m_offset == 1));
      typedef pointer_offset_caster<void*, OffsetType> caster_t;
      //Only the first operand needs the address without a null pointer test.
      //Everything that depends on the second one, its address and the mask
      //that zeroes the result when both are null, is loop invariant in the
      //usual "it - begin()" shape and is hoisted out of the loop
      const pointer p1 = static_cast<pointer>
         (caster_t(ipcdetail::offset_ptr_launder(caster_t(&pt).offset() + pt.internal.m_offset)).pointer());
      const pointer p2 = pt2.get();
      const difference_type mask =
         ~(difference_type(0) - difference_type(pt2.internal.m_offset == 1));
      return difference_type(p1 - p2) & mask;
   }

   //Comparison
   BOOST_INTERPROCESS_FORCEINLINE friend bool operator== (const offset_ptr &pt1, const offset_ptr &pt2) BOOST_NOEXCEPT
   {  return pt1.get() == pt2.get();  }

   BOOST_INTERPROCESS_FORCEINLINE friend bool operator!= (const offset_ptr &pt1, const offset_ptr &pt2) BOOST_NOEXCEPT
   {  return pt1.get() != pt2.get();  }

   BOOST_INTERPROCESS_FORCEINLINE friend bool operator<(const offset_ptr &pt1, const offset_ptr &pt2) BOOST_NOEXCEPT
   {  return pt1.get() < pt2.get();  }

   BOOST_INTERPROCESS_FORCEINLINE friend bool operator<=(const offset_ptr &pt1, const offset_ptr &pt2) BOOST_NOEXCEPT
   {  return pt1.get() <= pt2.get();  }

   BOOST_INTERPROCESS_FORCEINLINE friend bool operator>(const offset_ptr &pt1, const offset_ptr &pt2) BOOST_NOEXCEPT
   {  return pt1.get() > pt2.get();  }

   BOOST_INTERPROCESS_FORCEINLINE friend bool operator>=(const offset_ptr &pt1, const offset_ptr &pt2) BOOST_NOEXCEPT
   {  return pt1.get() >= pt2.get();  }

   //Comparison to raw ptr to support literal 0
   BOOST_INTERPROCESS_FORCEINLINE friend bool operator== (pointer pt1, const offset_ptr &pt2) BOOST_NOEXCEPT
   {  return pt1 == pt2.get();  }

   BOOST_INTERPROCESS_FORCEINLINE friend bool operator!= (pointer pt1, const offset_ptr &pt2) BOOST_NOEXCEPT
   {  return pt1 != pt2.get();  }

   BOOST_INTERPROCESS_FORCEINLINE friend bool operator<(pointer pt1, const offset_ptr &pt2) BOOST_NOEXCEPT
   {  return pt1 < pt2.get();  }

   BOOST_INTERPROCESS_FORCEINLINE friend bool operator<=(pointer pt1, const offset_ptr &pt2) BOOST_NOEXCEPT
   {  return pt1 <= pt2.get();  }

   BOOST_INTERPROCESS_FORCEINLINE friend bool operator>(pointer pt1, const offset_ptr &pt2) BOOST_NOEXCEPT
   {  return pt1 > pt2.get();  }

   BOOST_INTERPROCESS_FORCEINLINE friend bool operator>=(pointer pt1, const offset_ptr &pt2) BOOST_NOEXCEPT
   {  return pt1 >= pt2.get();  }

   //Comparison
   BOOST_INTERPROCESS_FORCEINLINE friend bool operator== (const offset_ptr &pt1, pointer pt2) BOOST_NOEXCEPT
   {  return pt1.get() == pt2;  }

   BOOST_INTERPROCESS_FORCEINLINE friend bool operator!= (const offset_ptr &pt1, pointer pt2) BOOST_NOEXCEPT
   {  return pt1.get() != pt2;  }

   BOOST_INTERPROCESS_FORCEINLINE friend bool operator<(const offset_ptr &pt1, pointer pt2) BOOST_NOEXCEPT
   {  return pt1.get() < pt2;  }

   BOOST_INTERPROCESS_FORCEINLINE friend bool operator<=(const offset_ptr &pt1, pointer pt2) BOOST_NOEXCEPT
   {  return pt1.get() <= pt2;  }

   BOOST_INTERPROCESS_FORCEINLINE friend bool operator>(const offset_ptr &pt1, pointer pt2) BOOST_NOEXCEPT
   {  return pt1.get() > pt2;  }

   BOOST_INTERPROCESS_FORCEINLINE friend bool operator>=(const offset_ptr &pt1, pointer pt2) BOOST_NOEXCEPT
   {  return pt1.get() >= pt2;  }

   BOOST_INTERPROCESS_FORCEINLINE friend void swap(offset_ptr &left, offset_ptr &right) BOOST_NOEXCEPT
   {
      const OffsetType left_off  = left.internal.m_offset;
      const OffsetType right_off = right.internal.m_offset;
      left.internal.m_offset  = ipcdetail::offset_ptr_to_offset_from_other(&left,  &right, right_off);
      right.internal.m_offset = ipcdetail::offset_ptr_to_offset_from_other(&right, &left,  left_off);
   }

   private:
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   //!A static_cast that can't change the target address is done by rebasing
   //!the stored offset, which needs a single null pointer test instead of the
   //!two that a round trip through a raw pointer needs
   template<class T2>
   BOOST_INTERPROCESS_FORCEINLINE static OffsetType priv_static_cast_offset
      (const offset_ptr<T2, DifferenceType, OffsetType, OffsetAlignment> &r, const offset_ptr *this_ptr, ipcdetail::bool_<true>) BOOST_NOEXCEPT
   {  return ipcdetail::offset_ptr_to_offset_from_other(this_ptr, &r, r.get_offset());  }

   //!A static_cast that can change the target address, like a cast between
   //!classes of a multiple inheritance hierarchy, must go through the pointer
   template<class T2>
   BOOST_INTERPROCESS_FORCEINLINE static OffsetType priv_static_cast_offset
      (const offset_ptr<T2, DifferenceType, OffsetType, OffsetAlignment> &r, const offset_ptr *this_ptr, ipcdetail::bool_<false>) BOOST_NOEXCEPT
   {  return ipcdetail::offset_ptr_to_offset<OffsetType>(static_cast<PointedType*>(r.get()), this_ptr);  }

   template<class T2>
   BOOST_INTERPROCESS_FORCEINLINE void assign(const offset_ptr<T2, DifferenceType, OffsetType, OffsetAlignment> &ptr, ipcdetail::bool_<true>) BOOST_NOEXCEPT
   {  //no need to pointer adjustment
      this->internal.m_offset = ipcdetail::offset_ptr_to_offset_from_other<OffsetType>(this, &ptr, ptr.get_offset());
   }

   template<class T2>
   BOOST_INTERPROCESS_FORCEINLINE void assign(const offset_ptr<T2, DifferenceType, OffsetType, OffsetAlignment> &ptr, ipcdetail::bool_<false>) BOOST_NOEXCEPT
   {  //we must convert to raw before calculating the offset
      this->internal.m_offset = ipcdetail::offset_ptr_to_offset<OffsetType>(static_cast<PointedType*>(ptr.get()), this);
   }

   //!Returns a copy of this pointer displaced 'diff' elements. Taking the
   //!operand by reference and building the result in place needs a single
   //!rebase, where passing and returning it by value needed two. Adding zero
   //!to a null pointer must still give a null pointer, so the rebase keeps
   //!its null pointer test
   BOOST_INTERPROCESS_FORCEINLINE offset_ptr priv_shifted(DifferenceType diff) const BOOST_NOEXCEPT
   {
      offset_ptr tmp;
      tmp.internal.m_offset = ipcdetail::offset_ptr_to_offset_from_other
         (&tmp, this, this->internal.m_offset);
      tmp.inc_offset(diff * DifferenceType(sizeof(PointedType)));
      return tmp;
   }

   BOOST_INTERPROCESS_FORCEINLINE offset_ptr priv_shifted_back(DifferenceType diff) const BOOST_NOEXCEPT
   {
      offset_ptr tmp;
      tmp.internal.m_offset = ipcdetail::offset_ptr_to_offset_from_other
         (&tmp, this, this->internal.m_offset);
      tmp.dec_offset(diff * DifferenceType(sizeof(PointedType)));
      return tmp;
   }

   BOOST_INTERPROCESS_FORCEINLINE void inc_offset(DifferenceType bytes) BOOST_NOEXCEPT
   {  internal.m_offset += OffsetType(bytes);   }

   BOOST_INTERPROCESS_FORCEINLINE void dec_offset(DifferenceType bytes) BOOST_NOEXCEPT
   {  internal.m_offset -= OffsetType(bytes);   }

   ipcdetail::offset_ptr_internal<OffsetType, OffsetAlignment> internal;

   public:
   BOOST_INTERPROCESS_FORCEINLINE const OffsetType &priv_offset() const BOOST_NOEXCEPT
   {  return internal.m_offset;   }

   BOOST_INTERPROCESS_FORCEINLINE       OffsetType &priv_offset() BOOST_NOEXCEPT
   {  return internal.m_offset;   }

   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};

//!operator<<
//!for offset ptr
template<class E, class T, class W, class X, class Y, std::size_t Z>
inline std::basic_ostream<E, T> & operator<<
   (std::basic_ostream<E, T> & os, offset_ptr<W, X, Y, Z> const & p)
{  return os << p.get_offset();   }

//!operator>>
//!for offset ptr
template<class E, class T, class W, class X, class Y, std::size_t Z>
inline std::basic_istream<E, T> & operator>>
   (std::basic_istream<E, T> & is, offset_ptr<W, X, Y, Z> & p)
{  return is >> p.priv_offset();  }

//!Simulation of static_cast between pointers. Never throws.
template<class T1, class P, class O, std::size_t A, class T2>
BOOST_INTERPROCESS_FORCEINLINE boost::interprocess::offset_ptr<T1, P, O, A>
   static_pointer_cast(const boost::interprocess::offset_ptr<T2, P, O, A> & r) BOOST_NOEXCEPT
{
   return boost::interprocess::offset_ptr<T1, P, O, A>
            (r, boost::interprocess::ipcdetail::static_cast_tag());
}

//!Simulation of const_cast between pointers. Never throws.
template<class T1, class P, class O, std::size_t A, class T2>
BOOST_INTERPROCESS_FORCEINLINE boost::interprocess::offset_ptr<T1, P, O, A>
   const_pointer_cast(const boost::interprocess::offset_ptr<T2, P, O, A> & r) BOOST_NOEXCEPT
{
   return boost::interprocess::offset_ptr<T1, P, O, A>
            (r, boost::interprocess::ipcdetail::const_cast_tag());
}

//!Simulation of dynamic_cast between pointers. Never throws.
template<class T1, class P, class O, std::size_t A, class T2>
BOOST_INTERPROCESS_FORCEINLINE boost::interprocess::offset_ptr<T1, P, O, A>
   dynamic_pointer_cast(const boost::interprocess::offset_ptr<T2, P, O, A> & r) BOOST_NOEXCEPT
{
   return boost::interprocess::offset_ptr<T1, P, O, A>
            (r, boost::interprocess::ipcdetail::dynamic_cast_tag());
}

//!Simulation of reinterpret_cast between pointers. Never throws.
template<class T1, class P, class O, std::size_t A, class T2>
BOOST_INTERPROCESS_FORCEINLINE boost::interprocess::offset_ptr<T1, P, O, A>
   reinterpret_pointer_cast(const boost::interprocess::offset_ptr<T2, P, O, A> & r) BOOST_NOEXCEPT
{
   return boost::interprocess::offset_ptr<T1, P, O, A>
            (r, boost::interprocess::ipcdetail::reinterpret_cast_tag());
}

}  //namespace interprocess {

#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

///has_trivial_destructor<> == true_type specialization for optimizations
template <class T, class P, class O, std::size_t A>
struct has_trivial_destructor< ::boost::interprocess::offset_ptr<T, P, O, A> >
{
   static const bool value = true;
};

namespace move_detail {

///has_trivial_destructor<> == true_type specialization for optimizations
template <class T, class P, class O, std::size_t A>
struct is_trivially_destructible< ::boost::interprocess::offset_ptr<T, P, O, A> >
{
   static const bool value = true;
};

}  //namespace move_detail {

namespace interprocess {

//!to_raw_pointer() enables boost::mem_fn to recognize offset_ptr.
//!Never throws.
template <class T, class P, class O, std::size_t A>
BOOST_INTERPROCESS_FORCEINLINE T * to_raw_pointer(boost::interprocess::offset_ptr<T, P, O, A> const & p) BOOST_NOEXCEPT
{  return ipcdetail::to_raw_pointer(p);   }

}  //namespace interprocess


#endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
}  //namespace boost {

#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

namespace boost{

//This is to support embedding a bit in the pointer
//for intrusive containers, saving space
namespace intrusive {

//Predeclaration to avoid including header
template<class VoidPointer, std::size_t N>
struct max_pointer_plus_bits;

template<std::size_t OffsetAlignment, class P, class O, std::size_t A>
struct max_pointer_plus_bits<boost::interprocess::offset_ptr<void, P, O, A>, OffsetAlignment>
{
   //The offset ptr can embed one bit less than the alignment since it
   //uses offset == 1 to store the null pointer.
   static const std::size_t value = ::boost::interprocess::ipcdetail::ls_zeros<OffsetAlignment>::value - 1;
};

//Predeclaration
template<class Pointer, std::size_t NumBits>
struct pointer_plus_bits;

template<class T, class P, class O, std::size_t A, std::size_t NumBits>
struct pointer_plus_bits<boost::interprocess::offset_ptr<T, P, O, A>, NumBits>
{
   typedef boost::interprocess::offset_ptr<T, P, O, A>      pointer;
   //Bits are stored in the lower bits of the pointer except the LSB,
   //because this bit is used to represent the null pointer.
   static const O Mask = ((static_cast<O>(1) << NumBits) - static_cast<O>(1)) << 1;
   BOOST_INTERPROCESS_STATIC_ASSERT(0 ==(Mask&1));

   //We must ALWAYS take argument "n" by reference as a copy of a null pointer
   //with a bit (e.g. offset == 3) would be incorrectly copied and interpreted as non-null.

   BOOST_INTERPROCESS_FORCEINLINE static pointer get_pointer(const pointer &n) BOOST_NOEXCEPT
   {
      pointer p;
      O const tmp_off = n.priv_offset() & ~Mask;
      p.priv_offset() = boost::interprocess::ipcdetail::offset_ptr_to_offset_from_other(&p, &n, tmp_off);
      return p;
   }

   BOOST_INTERPROCESS_FORCEINLINE static void set_pointer(pointer &n, const pointer &p) BOOST_NOEXCEPT
   {
      BOOST_ASSERT(0 == (get_bits)(p));
      O const stored_bits = n.priv_offset() & Mask;
      n = p;
      n.priv_offset() |= stored_bits;
   }

   BOOST_INTERPROCESS_FORCEINLINE static std::size_t get_bits(const pointer &n) BOOST_NOEXCEPT
   {
      return std::size_t((n.priv_offset() & Mask) >> 1u);
   }

   BOOST_INTERPROCESS_FORCEINLINE static void set_bits(pointer &n, std::size_t const b) BOOST_NOEXCEPT
   {
      BOOST_ASSERT(b < (std::size_t(1) << NumBits));
      O tmp = n.priv_offset();
      tmp &= ~Mask;
      tmp |= O(b << 1u);
      n.priv_offset() = tmp;
   }
};

}  //namespace intrusive

//Predeclaration
template<class T, class U>
struct pointer_to_other;

//Backwards compatibility with pointer_to_other
template <class PointedType, class DifferenceType, class OffsetType, std::size_t OffsetAlignment, class U>
struct pointer_to_other
   < ::boost::interprocess::offset_ptr<PointedType, DifferenceType, OffsetType, OffsetAlignment>, U >
{
   typedef ::boost::interprocess::offset_ptr<U, DifferenceType, OffsetType, OffsetAlignment> type;
};

}  //namespace boost{
#endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

#include <boost/interprocess/detail/config_end.hpp>

#if defined(BOOST_GCC) && (BOOST_GCC >= 40700)
#pragma GCC diagnostic pop
#endif

#endif //#ifndef BOOST_INTERPROCESS_OFFSET_PTR_HPP
