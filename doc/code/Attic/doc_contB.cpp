   #include <boost/interprocess/detail/config_begin.hpp>
   #include <boost/interprocess/detail/workaround.hpp>

   #include <boost/interprocess/managed_shared_memory.hpp>
   #include <boost/interprocess/containers/vector.hpp>
   #include <boost/interprocess/allocators/allocator.hpp>
   #include <algorithm>

   int main ()
   {
      using namespace boost::interprocess;

      //An special shared memory where we can
      //construct objects associated with a name.
      //Connect to the already created shared memory segment
      //and initialize needed resources
      managed_shared_memory segment
         (open_only 
         ,"MySharedMemory");  //segment name

      //Alias an STL compatible allocator of ints that allocates ints from the managed
      //shared memory segment.  This allocator will allow to place containers
      //in managed shared memory segments
      typedef allocator<int, managed_shared_memory::segment_manager> 
         ShmemAllocator;

      //Alias a vector that uses the previous STL-like allocator
      typedef vector<int, ShmemAllocator> MyVector;

      //Find the vector using the c-string name
      MyVector *myvector = segment.find<MyVector>("MyVector").first;

      //Use vector in reverse order
      std::sort(myvector->rbegin(), myvector->rend());
      // . . .

      //When done, destroy the vector from the segment
      segment.destroy<MyVector>("MyVector");
      return 0;
   }

   #include <boost/interprocess/detail/config_end.hpp>
