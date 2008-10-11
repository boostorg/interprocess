///////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
///////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_TEST_MOVABLE_INT_HEADER
#define BOOST_INTERPROCESS_TEST_MOVABLE_INT_HEADER

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/move.hpp>

namespace boost {
namespace interprocess {
namespace test {

class movable_int
{
   movable_int(const movable_int&);
   movable_int &operator= (const movable_int&);

   public:

   movable_int()
      :  m_int(0)
   {}

   explicit movable_int(int a)
      :  m_int(a)
   {}

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   movable_int(const detail::moved_object<movable_int> &mmi)
      :  m_int(mmi.get().m_int)
   {  mmi.get().m_int = 0; }
   #else
   movable_int(movable_int &&mmi)
      :  m_int(mmi.m_int)
   {  mmi.m_int = 0; }
   #endif

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   movable_int & operator= (const detail::moved_object<movable_int> &mmi)
   {  this->m_int = mmi.get().m_int;   mmi.get().m_int = 0;  return *this;  }
   #else
   movable_int & operator= (movable_int &&mmi)
   {  this->m_int = mmi.m_int;   mmi.m_int = 0;  return *this;  }
   #endif

   movable_int & operator= (int i)
   {  this->m_int = i;  return *this;  }

   bool operator ==(const movable_int &mi) const
   {  return this->m_int == mi.m_int;   }

   bool operator !=(const movable_int &mi) const
   {  return this->m_int != mi.m_int;   }

   bool operator <(const movable_int &mi) const
   {  return this->m_int < mi.m_int;   }

   bool operator <=(const movable_int &mi) const
   {  return this->m_int <= mi.m_int;   }

   bool operator >=(const movable_int &mi) const
   {  return this->m_int >= mi.m_int;   }

   bool operator >(const movable_int &mi) const
   {  return this->m_int > mi.m_int;   }

   int get_int() const
   {  return m_int;  }

   private:
   int m_int;
};

template<class E, class T> 
std::basic_ostream<E, T> & operator<< 
   (std::basic_ostream<E, T> & os, movable_int const & p)

{
    os << p.get_int();
    return os;
}

class movable_and_copyable_int
{
   public:

   movable_and_copyable_int()
      :  m_int(0)
   {}

   explicit movable_and_copyable_int(int a)
      :  m_int(a)
   {}

   movable_and_copyable_int(const movable_and_copyable_int& mmi)
      :  m_int(mmi.m_int)
   {}
   
   movable_and_copyable_int &operator= (const movable_and_copyable_int& mi)
   {  this->m_int = mi.m_int;    return *this;  }

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   movable_and_copyable_int(const detail::moved_object<movable_and_copyable_int> &mmi)
      :  m_int(mmi.get().m_int)
   {  mmi.get().m_int = 0; }
   #else
   movable_and_copyable_int(movable_and_copyable_int &&mmi)
      :  m_int(mmi.m_int)
   {  mmi.m_int = 0; }
   #endif

   #ifndef BOOST_INTERPROCESS_RVALUE_REFERENCE
   movable_and_copyable_int & operator= (const detail::moved_object<movable_and_copyable_int> &mmi)
   {  this->m_int = mmi.get().m_int;   mmi.get().m_int = 0;    return *this;  }
   #else
   movable_and_copyable_int & operator= (movable_and_copyable_int &&mmi)
   {  this->m_int = mmi.m_int;   mmi.m_int = 0;    return *this;  }
   #endif

   movable_and_copyable_int & operator= (int i)
   {  this->m_int = i;  return *this;  }

   bool operator ==(const movable_and_copyable_int &mi) const
   {  return this->m_int == mi.m_int;   }

   bool operator !=(const movable_and_copyable_int &mi) const
   {  return this->m_int != mi.m_int;   }

   bool operator <(const movable_and_copyable_int &mi) const
   {  return this->m_int < mi.m_int;   }

   bool operator <=(const movable_and_copyable_int &mi) const
   {  return this->m_int <= mi.m_int;   }

   bool operator >=(const movable_and_copyable_int &mi) const
   {  return this->m_int >= mi.m_int;   }

   bool operator >(const movable_and_copyable_int &mi) const
   {  return this->m_int > mi.m_int;   }

   int get_int() const
   {  return m_int;  }

   private:
   int m_int;
};

template<class E, class T> 
std::basic_ostream<E, T> & operator<< 
   (std::basic_ostream<E, T> & os, movable_and_copyable_int const & p)

{
    os << p.get_int();
    return os;
}

}  //namespace test {
}  //namespace interprocess {
}  //namespace boost {

namespace boost{
namespace interprocess{

template<>
struct is_movable<test::movable_int>
{
   public:
   enum {   value = true   };
};

template<>
struct is_movable<test::movable_and_copyable_int>
{
   public:
   enum {   value = true   };
};

}  //namespace interprocess{
}  //namespace boost{

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_TEST_MOVABLE_INT_HEADER
