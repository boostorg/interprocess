   #include <boost/interprocess/detail/config_begin.hpp>
   #include <boost/interprocess/detail/workaround.hpp>

   #include <vector>
   #include <boost/interprocess/managed_shared_memory.hpp>

   int main ()
   {
      using namespace boost::interprocess;
      //Shared memory front-end that is able to construct objects
      //associated with a c-string
      //Open the memory segment at the specified address and initialize resources
      fixed_managed_shared_memory segment
         (open_only, 
         "MySharedMemory",   //segment name
         (void*)0x03000000);  //mapping address

      //Alias an STL compatible allocator of ints that allocates ints from the segment
      //and that defines allocator::pointer as "int *". This allocator will be compatible
      //with most STL implementations.
      typedef allocator<int, fixed_managed_shared_memory::segment_manager> 
         ShmemAllocator;

      //Alias a vector that uses the previous STL-like allocator
      typedef std::vector<int, ShmemAllocator> MyVector;

      //Find the vector using the c-string name
      MyVector *myvector = segment.find<MyVector>("MyVector").first;

      //Use vector as you want
      std::sort(myvector->rbegin(), myvector->rend());
      // . . .

      //When done, destroy and delete vector
      segment.destroy<MyVector>("MyVector");
      return 0;
   }

   #include <boost/interprocess/detail/config_end.hpp>
