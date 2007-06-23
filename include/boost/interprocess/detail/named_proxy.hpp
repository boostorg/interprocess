//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2007. Distributed under the Boost
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
#include <boost/interprocess/detail/in_place_interface.hpp>
#include <boost/preprocessor/iteration/local.hpp> 
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/interprocess/detail/mpl.hpp>

/*!\file
   Describes a proxy class that implements named allocation syntax.
*/

namespace boost {
namespace interprocess { 
namespace detail {

/*!Function object that makes placement new without arguments*/
template<class T>
struct Ctor0Arg   :  public placement_destroy<T>
{
   typedef Ctor0Arg self_t;
   typedef T target_t;

   Ctor0Arg(){}

   self_t& operator++()       {  return *this;  }
   self_t  operator++(int)    {  return *this;  }

   virtual void construct(void *mem)
   {  new(mem)T;  }

   virtual void construct_n(void *mem, std::size_t num, std::size_t &constructed)
   {
      T* memory = static_cast<T*>(mem);
      for(constructed = 0; constructed < num; ++constructed)
         new(memory++)T;
   }
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
//       typedef Ctor2Arg self_t;
//
//       void do_increment(detail::false_)
//       { ++m_p1; ++m_p2;  }
//
//       void do_increment(detail::true_){}
//
//       self_t& operator++()
//       {
//          typedef detail::bool_<param_or_it> Result;
//          this->do_increment(Result());
//          return *this;
//       }
//
//       self_t  operator++(int) {  return ++*this;   *this;  }
//
//       Ctor2Arg(const P1 &p1, const P2 &p2)
//          : p1((P1 &)p_1), p2((P2 &)p_2) {}
//
//       virtual void construct(void *mem)
//       {  new(object)T(m_p1, m_p2); }
//
//       virtual void construct_n(void *mem
//                                , std::size_t num
//                                , std::size_t &constructed)
//       {
//          T* memory      = static_cast<T*>(mem);
//          for(constructed = 0; constructed < num; ++constructed){
//             new(memory++)T(m_p1, m_p2);
//             typedef detail::bool_<param_or_it> Result;
//             this->do_increment(Result());
//          }
//       }
//
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

#define BOOST_INTERPROCESS_AUX_PARAM_DEREFERENCE(z, n, data)   \
   BOOST_PP_CAT(P, n) & BOOST_PP_CAT(m_p, n);       \
/**/

#define BOOST_PP_LOCAL_MACRO(n)                                         \
   template<class T, bool param_or_it, BOOST_PP_ENUM_PARAMS(n, class P) >\
   struct BOOST_PP_CAT(BOOST_PP_CAT(Ctor, n), Arg)                      \
      :  public placement_destroy<T>                                                 \
   {                                                                    \
      typedef BOOST_PP_CAT(BOOST_PP_CAT(Ctor, n), Arg) self_t;          \
      typedef T target_t;                                               \
                                                                        \
      void do_increment(detail::false_)                             \
         { BOOST_PP_ENUM(n, BOOST_INTERPROCESS_AUX_PARAM_INC, _); }     \
                                                                        \
      void do_increment(detail::true_){}                            \
                                                                        \
      self_t& operator++()                                              \
      {                                                                 \
         typedef detail::bool_<param_or_it> Result;                     \
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
      virtual void construct(void *mem)                                 \
      {  new(mem)T(BOOST_PP_ENUM_PARAMS(n, m_p)); }                     \
                                                                        \
      virtual void construct_n(void *mem                                \
                        , std::size_t num                               \
                        , std::size_t &constructed)                     \
      {                                                                 \
         T* memory      = static_cast<T*>(mem);                         \
         for(constructed = 0; constructed < num; ++constructed){        \
            new(memory++)T(BOOST_PP_ENUM_PARAMS(n, m_p));               \
            typedef detail::bool_<param_or_it> Result;                  \
            this->do_increment(Result());                               \
         }                                                              \
      }                                                                 \
                                                                        \
      private:                                                          \
         BOOST_PP_REPEAT(n, BOOST_INTERPROCESS_AUX_PARAM_DEFINE, _)     \
   };                                                                   \
/**/


#define BOOST_PP_LOCAL_LIMITS (1, BOOST_INTERPROCESS_MAX_CONSTRUCTOR_PARAMETERS)
#include BOOST_PP_LOCAL_ITERATE()

#undef BOOST_INTERPROCESS_AUX_PARAM_LIST
#undef BOOST_INTERPROCESS_AUX_PARAM_INIT
#undef BOOST_INTERPROCESS_AUX_PARAM_DEFINE
#undef BOOST_INTERPROCESS_AUX_PARAM_INC

//!Describes a proxy class that implements named
//!allocation syntax.
template 
   < class SegmentManager  //segment manager to construct the object
   , class T               //type of object to build
   , bool param_or_it      //passing parameters are normal object or iterators?
   >
class named_proxy
{
   typedef typename SegmentManager::char_type char_type;
   const char_type *     mp_name;
   SegmentManager *     mp_mngr;
   mutable std::size_t  m_num;
   const bool           m_find;
   const bool           m_dothrow;

   public:
   named_proxy(SegmentManager *mngr, const char_type *name, bool find, bool dothrow)
      :  mp_name(name), mp_mngr(mngr), m_num(1)
      ,  m_find(find),  m_dothrow(dothrow)
   {}

   /*!makes a named allocation and calls the default constructor*/
   T *operator()() const
   {  
      Ctor0Arg<T> ctor_obj;
      return mp_mngr->template 
         generic_construct<T>(mp_name, m_num, m_find, m_dothrow, ctor_obj);
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
         return mp_mngr->template generic_construct<T>                       \
            (mp_name, m_num, m_find, m_dothrow, ctor_obj);                    \
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
   //    return mp_mngr->template generic_construct<T>
   //       (mp_name, m_num, m_find, m_dothrow, ctor_obj);
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
