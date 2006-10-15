   #include <boost/interprocess/shared_memory_object.hpp>
   #include <boost/interprocess/mapped_region.hpp>
   #include <iostream>
   #include <cstring>

   int main ()
   {
      using namespace boost::interprocess;
      try{
         //Erase previous shared memory
         shared_memory_object::remove("shared_memory");

         //Create a shared memory object.
         shared_memory_object shm
            (create_only               //only create
            ,"shared_memory"           //name
            ,read_write   //read-write mode
            );

         //Set size
         shm.truncate(1000);

         //Map the whole shared memory in this process
         mapped_region region
            (shm                       //What to map
            ,read_write //Map it as read-write
            );

         //Get the address of the mapped region
         void * addr       = region.get_address();
         std::size_t size  = region.get_size();

         //Write all the memory to 1
         std::memset(addr, 1, size);

      }
      catch(interprocess_exception &ex){
         std::cout << ex.what() << std::endl;
         return 1;
      }

      return 0;
   }
