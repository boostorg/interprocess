//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#ifdef _MSC_VER
#pragma warning (disable : 4503)
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/offset_ptr.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/vector.hpp>

using namespace boost::interprocess;

class MyClass
{
   public:
   MyClass()
   {}
};

//Deleter. Takes a pointer to the segment manager which
//has a function to delete the object from the shared memory
//segment. 
struct MyDeleter
{
   typedef offset_ptr<MyClass> pointer;

   MyDeleter(managed_shared_memory::segment_manager *mngr)
      :  m_mngr(mngr)
   {}

   void operator()(pointer ptr)
   {  m_mngr->destroy_ptr(ptr.get());  }

   offset_ptr<managed_shared_memory::segment_manager> m_mngr;
};

typedef unique_ptr<MyClass, MyDeleter> my_unique_ptr_class;
/*
//Explicit instantiation of shared memory set
template class set   <my_unique_ptr_class
                     ,std::less<my_unique_ptr_class>
                     ,allocator  <my_unique_ptr_class
                                 ,managed_shared_memory::segment_manager>
                     >;

typedef set <my_unique_ptr_class
            ,std::less<my_unique_ptr_class>
            ,allocator  <my_unique_ptr_class
                        ,managed_shared_memory::segment_manager>
            > MySet;

//Explicit instantiation of shared memory list
template class list  <my_unique_ptr_class
                     ,allocator  <my_unique_ptr_class
                                 ,managed_shared_memory::segment_manager>
                     >;


typedef list<my_unique_ptr_class
            ,allocator  <my_unique_ptr_class
                        ,managed_shared_memory::segment_manager>
            > MyList;
*/
//This explicit instantiation fails
//Because, for example, this iterator copy expression fails:
// *target = *source;
//
//template class vector   <my_unique_ptr_class
//                        ,allocator  <my_unique_ptr_class
//                                    ,managed_shared_memory::segment_manager>
//                        >;


int main()
{
   //Create managed shared memory
   shared_memory_object::remove("mysegment");
   managed_shared_memory segment(create_only, "mysegment", 1000);
   
   //Create to unique_ptr using dynamic allocation
   my_unique_ptr_class my_ptr (segment.construct<MyClass>(anonymous_instance)()
                              ,segment.get_segment_manager());
   my_unique_ptr_class my_ptr2(segment.construct<MyClass>(anonymous_instance)()
                              ,segment.get_segment_manager());

   //Backup relative pointers to future tests
   offset_ptr<MyClass> ptr1 = my_ptr.get();
   offset_ptr<MyClass> ptr2 = my_ptr2.get();

   //Test some copy constructors
   my_unique_ptr_class my_ptr3(0, segment.get_segment_manager());
   my_unique_ptr_class my_ptr4(move(my_ptr3));

   //This does not compile
//   my_unique_ptr_class my_ptr4         (my_ptr3);
/*
   //Construct a list and fill
   MyList list(segment.get_segment_manager());

   //Insert from my_unique_ptr_class
   list.push_back(my_ptr);

   //Insert from CONST my_unique_ptr_class
   list.push_back(my_ptr2);

   //Check pointers
   assert(my_ptr.get() == 0);
   assert(my_ptr2.get() == 0);
   assert(list.begin()->get() == ptr1);
   assert(list.rbegin()->get() == ptr2);

   //Construct a set and fill
   MySet set(std::less<my_unique_ptr_class>(), segment.get_segment_manager());

   //Insert in set from list passing ownership
   set.insert(*list.begin());
   set.insert(*list.rbegin());

   //Check pointers
   assert(list.begin()->get() == 0);
   assert(list.rbegin()->get()== 0);

   //A set is ordered by std::less<my_unique_ptr_class> so
   //be careful when comparing pointers
   if(ptr1 < ptr2){
      assert(set.begin()->get()  == ptr1);
      assert(set.rbegin()->get() == ptr2);
   }
   else{
      assert(set.rbegin()->get() == ptr1);
      assert(set.begin()->get()  == ptr2);
   }
*/
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
