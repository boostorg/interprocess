   #include <boost/interprocess/shared_memory_object.hpp>
   #include <boost/interprocess/mapped_region.hpp>
   #include <iostream>
   #include <cstring>

   int main ()
   {
      using namespace boost::interprocess;
      try{
         //Open already created shared memory object.
         shared_memory_object shm
            (open_only                 //only create
            ,"shared_memory"           //name
            ,read_only    //read-write mode
            );

         //Map the whole shared memory in this process
         mapped_region region
            (shm                       //What to map
            ,read_only  //Map it as read-write
            );

         //Get the address of the mapped region
         void * addr       = region.get_address();
         std::size_t size  = region.get_size();

         //Check that memory was initialized to 1
         const char *mem = static_cast<char*>(addr);
         for(std::size_t i = 0; i < size; ++i){
            if(*mem++ != 1){
               std::cout << "Error checking memory!" << std::endl;               
               return 1;
            }
         }

         std::cout << "Test successful!" << std::endl;
      }
      catch(interprocess_exception &ex){
         std::cout << "Unexpected exception: " << ex.what() << std::endl;
         return 1;
      }

      return 0;
   }
