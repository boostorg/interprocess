//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/detail/workaround.hpp>
//[doc_complex_map_uses_allocator
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/container/map.hpp>
#include <boost/container/vector.hpp>
#include <boost/container/string.hpp>
//<-
#include "../test/get_process_id_name.hpp"
//->

using namespace boost::interprocess;

//Typedefs of allocators and containers
typedef managed_shared_memory::segment_manager                       segment_manager_t;
typedef allocator<void, segment_manager_t>                           void_allocator;
typedef allocator<int, segment_manager_t>                            int_allocator;
typedef boost::container::vector<int, int_allocator>                 int_vector;
typedef allocator<int_vector, segment_manager_t>                     int_vector_allocator;
typedef boost::container::vector<int_vector, int_vector_allocator>   int_vector_vector;
typedef allocator<char, segment_manager_t>                           char_allocator;
typedef boost::container::basic_string<char, std::char_traits<char>, char_allocator>   char_string;

class complex_data
{
   int               id_;
   char_string       char_string_;
   int_vector_vector int_vector_vector_;

   public:
   //Mark this class as uses-allocator construction-ready (see Boost.Container's docs)
   //Boost.Interprocess machinery will pass the allocator argument automatically if
   //constructors takes the allocator_type as the last argument of constructors
   typedef void_allocator allocator_type;

   //Since void_allocator is convertible to any other allocator<T>, we can simplify
   //the initialization taking just one allocator for all inner containers.
   complex_data(int id, const char *name, const void_allocator &void_alloc)
      : id_(id), char_string_(name, void_alloc), int_vector_vector_(void_alloc)
   {}
   //Other members...
   //<-
   int get_id() { return id_; };
   char_string get_char_string() { return char_string_; };
   int_vector_vector get_int_vector_vector() { return int_vector_vector_; };
   //->
};

//A transparent comparison functor
//Allows creating associative container `value_type`s from comparable
//types (e.g. shared memory string types from "const char *" arguments)
struct less_transparent
{
   typedef void is_transparent;

   template<class T, class U>
   bool operator() (const T &t, const U &u) const
   {  return t < u; }
};

//Definition of the map holding a string as key and complex_data as mapped type
typedef std::pair<const char_string, complex_data>                      map_value_type;
typedef std::pair<char_string, complex_data>                            movable_to_map_value_type;
typedef allocator<map_value_type, segment_manager_t>                    map_value_type_allocator;
typedef boost::container::map< char_string, complex_data
           , less_transparent, map_value_type_allocator>                complex_map_type;

int main ()
{
   //Remove shared memory on construction and destruction
   struct shm_remove
   {
      shm_remove() { shared_memory_object::remove(test::get_process_id_name()); }
      ~shm_remove(){ shared_memory_object::remove(test::get_process_id_name()); }
   } remover;
   //<-
   (void)remover;
   //->

   //Create shared memory
   managed_shared_memory segment(create_only, test::get_process_id_name(), 65536);

   //Construct the shared memory map (associated with name "MyMap") and fill it using "extended uses allocator construction".
   //In this case "construct" tries to automatically pass the allocator argument to the constructed
   //type in addition to user-supplied arguments. "Uses-allocator construction" from the C++ standard
   //
   //In this case, no user arguments are provided, but the segment manager machinery has
   //detected that boost::container::map supports the user-allocator-construction protocol
   //so it automatically adds the allocator parameter and calls map::map(allocator_type) constructor.
   complex_map_type *mymap = segment.construct<complex_map_type>("MyMap")();

   //This transparent insertion function plus uses-allocator construction so that
   //the programmer does not need to explicitly pass allocator instances. The container,
   //through boost::container::allocator::construct function, automatically adds the needed allocator
   //arguments without forcing the user to supply them, calling the following constructors:
   //
   //key_type    --> basic_string(const char *, allocator_type)
   //mapped_type --> complex_data(int, const char *, allocator_type)
   //
   //The programmer only needs to specify the non-allocator arguments
   mymap->try_emplace("key_str", 3, "default_name");

   return 0;
}
//]
