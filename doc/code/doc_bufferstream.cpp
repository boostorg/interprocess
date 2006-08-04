   #include <boost/interprocess/detail/config_begin.hpp>
   #include <boost/interprocess/detail/workaround.hpp>

   #include <boost/interprocess/managed_shared_memory.hpp>
   #include <boost/interprocess/streams/bufferstream.hpp>
   #include <vector>
   #include <iterator>
   
   using namespace boost::interprocess;

   int main ()
   {
      shared_memory_object::remove("MySharedMemory");
      //Create shared memory
      managed_shared_memory segment(create_only, 
                                  "MySharedMemory",  //segment name
                                  65536);

      //Fill data
      std::vector<int> data, data2;
      data.reserve(100);
      for(int i = 0; i < 100; ++i){
         data.push_back(i);
      }

      //Allocate a buffer in shared memory to write data
      char *my_cstring = 
         segment.construct<char>("MyCString")[100*5](0);
      bufferstream mybufstream(my_cstring, 100*5);

      //Now write data to the buffer
      for(int i = 0; i < 100; ++i){
         mybufstream << data[i] << std::endl;
      }

      //Check there was no overflow attempt
      assert(mybufstream.good());

      //Extract all values from the shared memory string
      //directly to a vector.
      data2.reserve(100);
      std::istream_iterator<int> it(mybufstream), itend;
      std::copy(it, itend, std::back_inserter(data2));

      //This extraction should have ended will fail error since 
      //the numbers formatted in the buffer end before the end
      //of the buffer. (Otherwise it would trigger eofbit)
      assert(mybufstream.fail());

      //Compare data
      assert(std::equal(data.begin(), data.end(), data2.begin()));

      //Clear errors and rewind
      mybufstream.clear();
      mybufstream.seekp(0, std::ios::beg);
      
      //Now write again the data trying to do a buffer overflow
      for(int i = 0; i < 500; ++i){
         mybufstream << data[i] << std::endl;
      }

      //Now make sure badbit is active
      //which means overflow attempt.
      assert(!mybufstream.good());
      assert(mybufstream.bad());
      segment.destroy_ptr(my_cstring);      
      return 0;
   }

   #include <boost/interprocess/detail/config_end.hpp>
