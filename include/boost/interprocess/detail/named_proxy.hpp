//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_NAMED_PROXY_HPP
#define BOOST_INTERPROCESS_NAMED_PROXY_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <new>
#include <boost/config.hpp>
#include <boost/preprocessor/iteration/local.hpp> 
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/has_trivial_constructor.hpp>
#include <boost/bind.hpp>

/*!\file
   Describes a proxy class that implements named allocation syntax.
*/

namespace boost { namespace interprocess { 

namespace detail {

/*!Function object that makes placement new without arguments*/
template<class T>
struct Ctor0Arg
{
   typedef Ctor0Arg self_t;
   typedef T target_t;
   enum { is_trivial = boost::has_trivial_constructor<T>::value };

   Ctor0Arg(){}

   self_t& operator++()       {  return *this;  }
   self_t  operator++(int)    {  return *this;  }
   static inline void construct(T *mem, boost::mpl::false_)
   { new(mem)T;  }
   static inline void construct(T *mem, boost::mpl::true_)
   {}
   void operator()(T *mem) const 
   {
      typedef boost::mpl::bool_<is_trivial> Result;
      Ctor0Arg<T>::construct(mem, Result());
   }
   #if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))
   private:
   char dummy_; // BCB would by default use 8 B for empty structure
   #endif
};

#ifndef BOOST_INTERPROCESS_MAX_CONSTRUCTOR_PARAMETERS
#  define BOOST_INTERPROCESS_MAX_CONSTRUCTOR_PARAMETERS 10
#endif

////////////////////////////////////////////////////////////////
//    What the macro should generate (n == 2):
//
//    template<class T, bool param_or_it, class P1, class P2>
//    struct Ctor2Arg
//    {
//       enum { is_trivial = false };
//       typedef Ctor2Arg self_t;
//
//       void do_increment(boost::mpl::false_)
//       { ++m_p1; ++m_p2;  }
//
//       void do_increment(boost::mpl::true_){}
//
//       self_t& operator++()
//       {
//          typedef boost::mpl::bool_<param_or_it> Result;
//          this->do_increment(Result());
//          return *this;
//       }
//
//       self_t  operator++(int) {  return ++*this;   *this;  }
//
//       Ctor2Arg(const P1 &p1, const P2 &p2)
//          : p1((P1 &)p_1), p2((P2 &)p_2) {}
//
//       void operator()(T* mem) const 
//       {  new (mem) T(m_p1, m_p2); }
//    private:
//       P1 &m_p1; P2 &m_p2;
//    };
////////////////////////////////////////////////////////////////

//Note:
//We define template parameters as const references to
//be able to bind temporaries. After that we will un-const them.
//This cast is ugly but it is necessary until "perfect forwarding"
//is achieved in C++0x. Meanwhile, if we want to be able to
//bind rvalues with non-const references, we have to be ugly
#define BOOST_INTERPROCESS_AUX_PARAM_LIST(z, n, data) \
  const BOOST_PP_CAT(P, n) & BOOST_PP_CAT(p, n) \
/**/

#define BOOST_INTERPROCESS_AUX_PARAM_INIT(z, n, data) \
  BOOST_PP_CAT(m_p, n) (const_cast<BOOST_PP_CAT(P, n) &>(BOOST_PP_CAT(p, n))) \
/**/

#define BOOST_INTERPROCESS_AUX_PARAM_INC(z, n, data)   \
  BOOST_PP_CAT(++m_p, n)                        \
/**/

#define BOOST_INTERPROCESS_AUX_PARAM_DEFINE(z, n, data)   \
  BOOST_PP_CAT(P, n) & BOOST_PP_CAT(m_p, n);       \
/**/

#define BOOST_PP_LOCAL_MACRO(n)                                         \
   template<class T, bool param_or_it, BOOST_PP_ENUM_PARAMS(n, class P) >\
   struct BOOST_PP_CAT(BOOST_PP_CAT(Ctor, n), Arg)                      \
   {                                                                    \
      enum { is_trivial = false };                                      \
      typedef BOOST_PP_CAT(BOOST_PP_CAT(Ctor, n), Arg) self_t;          \
      typedef T target_t;                                               \
                                                                        \
      void do_increment(boost::mpl::false_)                             \
         { BOOST_PP_ENUM(n, BOOST_INTERPROCESS_AUX_PARAM_INC, _); }     \
                                                                        \
      void do_increment(boost::mpl::true_){}                            \
                                                                        \
      self_t& operator++()                                              \
      {                                                                 \
         typedef boost::mpl::bool_<param_or_it> Result;                 \
         this->do_increment(Result());                                  \
         return *this;                                                  \
      }                                                                 \
                                                                        \
      self_t  operator++(int) {  return ++*this;   *this;  }            \
                                                                        \
      BOOST_PP_CAT(BOOST_PP_CAT(Ctor, n), Arg)                          \
         ( BOOST_PP_ENUM(n, BOOST_INTERPROCESS_AUX_PARAM_LIST, _) )     \
         : BOOST_PP_ENUM(n, BOOST_INTERPROCESS_AUX_PARAM_INIT, _) {}    \
                                                                        \
      void operator()(T* mem) const                                     \
         {  new (mem) T(BOOST_PP_ENUM_PARAMS(n, m_p));  }               \
                                                                        \
    private:                                                            \
         BOOST_PP_REPEAT(n, BOOST_INTERPROCESS_AUX_PARAM_DEFINE, _)     \
   };                                                                   \
/**/

#define BOOST_PP_LOCAL_LIMITS (1, BOOST_INTERPROCESS_MAX_CONSTRUCTOR_PARAMETERS)
#include BOOST_PP_LOCAL_ITERATE()

#undef BOOST_INTERPROCESS_AUX_PARAM_LIST
#undef BOOST_INTERPROCESS_AUX_PARAM_INIT
#undef BOOST_INTERPROCESS_AUX_PARAM_DEFINE
#undef BOOST_INTERPROCESS_AUX_PARAM_INC

/*!Describes a proxy class that implements named allocation syntax.
*/
template 
   < class CharType        //char type for the name (char, wchar_t...)
   , class T               //type of object to build
   , class NamedAlloc      //class to allocate and construct the object
   , bool find             //if true, we try to find the object before creating
   , bool dothrow          //if true, we throw an exception, otherwise, return 0
   , bool param_or_it      //passing parameters are normal object or iterators?
   >
class named_proxy
{
   const CharType *     mp_name;
   NamedAlloc     *     mp_alloc;
   mutable std::size_t  m_num;

 public:
   named_proxy(const CharType *name, NamedAlloc *alloc) 
      : mp_name(name), mp_alloc(alloc), m_num(1){}

   /*!makes a named allocation and calls the default constructor*/
   T *operator()() const
   {  
      Ctor0Arg<T> ctor_obj;
      return mp_alloc->template 
         generic_construct<T>(mp_name, m_num, find, dothrow, ctor_obj);
   }
   /**/

   // Boost preprocessor used to create operator() overloads
   #define BOOST_INTERPROCESS_AUX_TYPE_LIST(z, n, data) \
      BOOST_PP_CAT(P, n) \
   /**/

   #define BOOST_INTERPROCESS_AUX_PARAM_LIST(z, n, data) \
      const BOOST_PP_CAT(P, n) BOOST_PP_CAT(&p, n) \
   /**/

   #define BOOST_PP_LOCAL_MACRO(n)                                            \
      template<BOOST_PP_ENUM_PARAMS(n, class P)>                              \
      T *operator()(BOOST_PP_ENUM (n, BOOST_INTERPROCESS_AUX_PARAM_LIST, _)) const   \
      {                                                                       \
         typedef BOOST_PP_CAT(BOOST_PP_CAT(Ctor, n), Arg)                     \
            <T, param_or_it, BOOST_PP_ENUM (n, BOOST_INTERPROCESS_AUX_TYPE_LIST, _)> \
            ctor_obj_t;                                                       \
         ctor_obj_t ctor_obj (BOOST_PP_ENUM_PARAMS(n, p));                    \
         return mp_alloc->template generic_construct<T>                       \
            (mp_name, m_num, find, dothrow, ctor_obj);                        \
      }                                                                       \
   /**/

   #define BOOST_PP_LOCAL_LIMITS ( 1, BOOST_INTERPROCESS_MAX_CONSTRUCTOR_PARAMETERS )
   #include BOOST_PP_LOCAL_ITERATE()
   #undef BOOST_INTERPROCESS_AUX_PARAM_LIST
   #undef BOOST_INTERPROCESS_AUX_TYPE_LIST

   ////////////////////////////////////////////////////////////////////////
   //             What the macro should generate (n == 2)
   ////////////////////////////////////////////////////////////////////////
   //
   // template <class P1, class P2>
   // T *operator()(P1 &p1, P2 &p2) const 
   // {
   //    typedef Ctor2Arg
   //       <T, param_or_it, P1, P2>
   //       ctor_obj_t;
   //    ctor_obj_t ctor_obj(p1, p2);
   //
   //    return mp_alloc->template generic_construct<T>
   //       (mp_name, m_num, find, dothrow, ctor_obj);
   // }
   //
   //////////////////////////////////////////////////////////////////////////

   //This operator allows --> named_new("Name")[3]; <-- syntax
   const named_proxy &operator[](std::size_t num) const
      {  m_num *= num; return *this;  }
};

}}}   //namespace boost { namespace interprocess { namespace detail {

#include <boost/interprocess/detail/config_end.hpp>

#endif //#ifndef BOOST_INTERPROCESS_NAMED_PROXY_HPP
