//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2004-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/intermodule_singleton.hpp>
#include <iostream>

using namespace boost::interprocess;

class MyClass
{
   public:
   MyClass()
   {
      std::cout << "Constructor\n";
   }

   void shout() const
   {
      std::cout << "Shout\n";
   }

   ~MyClass()
   {
      std::cout << "Destructor\n";
   }
};

class MyDerivedClass
   : public MyClass
{};

class MyThrowingClass
{
   public:
   MyThrowingClass()
   {
      throw int(0);
   }
};



int main ()
{

   bool exception_thrown = false;
   bool exception_2_thrown = false;

   try{
      detail::intermodule_singleton<MyThrowingClass, true>::get();
   }
   catch(int &){
      exception_thrown = true;
      //Second try
      try{
         detail::intermodule_singleton<MyThrowingClass, true>::get();
      }
      catch(interprocess_exception &){
         exception_2_thrown = true;
      }
   }

   if(!exception_thrown || !exception_2_thrown){
      return 1;
   }

   MyClass & mc = detail::intermodule_singleton<MyClass>::get();
   mc.shout();
   detail::intermodule_singleton<MyClass>::get().shout();
   detail::intermodule_singleton<MyDerivedClass>::get().shout();

   //Second try
   exception_2_thrown = false;
   try{
      detail::intermodule_singleton<MyThrowingClass, true>::get();
   }
   catch(interprocess_exception &){
      exception_2_thrown = true;
   }
   if(!exception_2_thrown){
      return 1;
   }

   return 0;   
}

#include <boost/interprocess/detail/config_end.hpp>

