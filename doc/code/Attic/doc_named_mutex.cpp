   #include <boost/interprocess/sync/scoped_lock.hpp>
   #include <boost/interprocess/sync/named_mutex.hpp>
   #include <fstream>

   int main ()
   {
      using namespace boost::interprocess;
      try{
         
         //Open or create the named mutex
         named_mutex mutex(open_or_create, "cout_named_mutex");

         std::ofstream file("file_name");

         for(int i = 0; i < 10; ++i){
            
            //Do some operations...

            //Write to file atomically
            scoped_lock<named_mutex> lock(mutex);
            file << "Process name, ";
            file << "This is iteration #" << i;
            file << std::endl;
         }
      }
      catch(interprocess_exception &ex){
         std::cout << ex.what() << std::endl;
         return 1;
      }

      return 0;
   }
