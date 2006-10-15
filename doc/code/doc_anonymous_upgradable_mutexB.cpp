   #include <boost/interprocess/shared_memory_object.hpp>
   #include <boost/interprocess/mapped_region.hpp>
   #include <boost/interprocess/sync/scoped_lock.hpp>
   #include "doc_upgradable_mutex_shared_data.hpp"
   #include <iostream>
   #include <cstdio>

   using namespace boost::interprocess;
   
   int main ()
   {
      try{
         //Open the shared memory object.
         shared_memory_object shm
            (open_only                    //only create
            ,"shared_memory"              //name
            ,read_write  //read-write mode
            );

         //Map the whole shared memory in this process
         mapped_region region
            (shm                       //What to map
            ,read_write //Map it as read-write
            );

         //Get the address of the mapped region
         void * addr       = region.get_address();

         //Construct the shared structure in memory
         shared_data * data = static_cast<shared_data*>(addr);

         //Write some logs
         for(int i = 0; i < 100; ++i){
            //Lock the upgradable_mutex
            scoped_lock<interprocess_upgradable_mutex> lock(data->upgradable_mutex);
            std::sprintf(data->items[(data->current_line++) % shared_data::NumItems]
                   ,"%s_%d", "process_a", i);
            if(i == (shared_data::NumItems-1))
               data->end_b = true;
            //Mutex is released here
         }

         //Wait until the other process ends
         while(1){
            scoped_lock<interprocess_upgradable_mutex> lock(data->upgradable_mutex);
            if(data->end_a)
               break;
         }
      }
      catch(interprocess_exception &ex){
         std::cout << ex.what() << std::endl;
         return 1;
      }

      return 0;
   }
