   #include <boost/interprocess/detail/config_begin.hpp>
   #include <boost/interprocess/detail/workaround.hpp>

   #include <boost/interprocess/managed_shared_memory.hpp>
   #include <cstddef>
   #include <utility>
   #include <assert.h>

   int main ()
   {
      using namespace boost::interprocess;
      typedef std::pair<double, int> MyType;

      //An special shared memory where we can
      //construct objects associated with a name.
      //Connect to the already created shared memory segment
      //and initialize needed resources
      managed_shared_memory segment(
         open_only, 
         "MySharedMemory");

      //Find the array and object
      std::pair<MyType*, std::size_t> res;
      res = segment.find<MyType> ("MyType array");   

      std::size_t array_len   = res.second;
      //Length should be 10
      assert(array_len == 10);

      //Find the array and the object
      res = segment.find<MyType> ("MyType instance");   

      std::size_t len   = res.second;

      //Length should be 1
      assert(len == 1);

      //Use data
      // . . . 

      //We're done, delete array from memory
      segment.destroy<MyType>("MyType array");

      //We're done, delete object from memory
      segment.destroy<MyType>("MyType instance");
      return 0;
   }

   #include <boost/interprocess/detail/config_end.hpp>
