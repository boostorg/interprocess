//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2004-2005. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <fstream>
#include <iostream>
#include <boost/interprocess/mapped_file.hpp>

using namespace boost::interprocess;

int main ()
{
   try{
      const std::size_t FileSize = 99999*2;
      {
         //Create file with given size
         std::ofstream file("my_file", std::ios::binary | std::ios::trunc);
         file.seekp(static_cast<std::streamoff>(FileSize-1));
         file.write("", 1);
      }

      {
         //Create a file mapping
         file_mapping mapping("my_file", file_mapping::read_write);
         //Create two mapped regions, one half of the file each
         mapped_region region (mapping
                              ,mapped_region::read_write
                              ,0
                              ,FileSize/2
                              );

         mapped_region region2(mapping
                              ,mapped_region::read_write
                              ,FileSize/2
                              ,FileSize - FileSize/2
                              );

         //Fill two regions with a pattern   
         unsigned char *filler = static_cast<unsigned char*>(region.get_address());
         for(std::size_t i = 0
            ;i < FileSize/2
            ;++i){
            *filler++ = static_cast<unsigned char>(i);
         }

         filler = static_cast<unsigned char*>(region2.get_address());
         for(std::size_t i = FileSize/2
            ;i < FileSize
            ;++i){
            *filler++ = static_cast<unsigned char>(i);
         }
      }

      //See if the pattern is correct in the file
      {
         //Open the file
         std::ifstream file("my_file", std::ios::binary);

         //Create a memory buffer
         std::auto_ptr<unsigned char> memory (new unsigned char [FileSize/2 +1]);
         
         //Fill buffer
         file.read(static_cast<char*>(static_cast<void*>(memory.get()))
                  , FileSize/2);

         unsigned char *checker = memory.get();
         //Check pattern
         for(std::size_t i = 0
            ;i < FileSize/2
            ;++i){
            if(*checker++ != static_cast<unsigned char>(i)){
               return 1;
            }
         }

         //Fill buffer
         file.read(static_cast<char*>(static_cast<void*>(memory.get()))
                  , FileSize - FileSize/2);

         checker = memory.get();
         //Check pattern
         for(std::size_t i = FileSize/2
            ;i < FileSize
            ;++i){
            if(*checker++ != static_cast<unsigned char>(i)){
               return 1;
            }
         }
      }

      //Now check the pattern mapping a single read only mapped_region
      {
         //Create a file mapping
         file_mapping mapping("my_file", file_mapping::read_only);

         //Create a single regions, mapping all the file
         mapped_region region (mapping
                              ,mapped_region::read_only
                              );

         //Check pattern
         unsigned char *pattern = static_cast<unsigned char*>(region.get_address());
         for(std::size_t i = 0
            ;i < FileSize
            ;++i, ++pattern){
            if(*pattern != static_cast<unsigned char>(i)){
               return 1;
            }
         }
      }
   }
   catch(std::exception &exc){
      std::cout << "Unhandled exception: " << exc.what() << std::endl;
   }

   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>

/*
#include <fstream>
#include <string>
#include <cstdio>
#include <vector>
#include <boost/unordered_set.hpp>
#include <boost/pool/pool_alloc.hpp>

char allchars []    = "abcdefghijklmnopqrstuvwxyz";

struct DictionaryValue
{
   std::string str;
   std::size_t count;
   bool operator == (const DictionaryValue &other) const
      {  return str == other.str; }
};

struct DictionaryHasher
: std::unary_function<DictionaryValue, std::size_t>
{
   std::size_t operator()(const DictionaryValue &val) const
   {
    const char *str = val.str.data();
    std::size_t len = val.str.size();
    std::size_t hash = 0;

    for (;;)
    {
	   switch (len)
	   {
	      case 0:
		   return hash;

	      case 1:
		   hash *= 37;
		   hash += *(unsigned char *)str;
		   return hash;

	      case 2:
		   hash *= 37;
		   hash += *(unsigned short *)str;
		   return hash;

	      case 3:
		   hash *= 37;
		   hash += (*(unsigned short *)str << 8) +
			   ((unsigned char *)str)[2];
		   return hash;

	      default:
		   hash *= 37;
		   hash += *(long *)str;
		   str += 4;
		   len -= 4;
		   break;
	   }
    }
   }

};

int main( int argc, char* argv[] )
{
  std::size_t w_total = 0;
  std::size_t l_total = 0;
  std::size_t c_total = 0;

  typedef boost::unordered_set<DictionaryValue
                              ,DictionaryHasher
                              ,std::equal_to<DictionaryValue>
                              ,boost::fast_pool_allocator<DictionaryValue>
                              > Dictionary;

  Dictionary dictionary;
  typedef Dictionary::iterator iterator;
  DictionaryValue dict_value;

   std::ifstream input_file(argv[1], std::ios_base::in | std::ios_base::binary );
   input_file.seekg(0, std::ios::end);

   std::vector<char> data(static_cast<std::size_t>(input_file.tellg()));
   input_file.seekg(0, std::ios::beg);
   input_file.read(&data[0], static_cast<std::streamsize>(data.size()));
   input_file.close();

  std::size_t w_count = 0;
  std::size_t w_diff = 'z'-'a';

  for ( int a = 0; a < 100; ++a )
  {
    std::string &word = dict_value.str;
    word.reserve(64);
    dict_value.count = 0;

    std::size_t w_cnt = 0;
    std::size_t l_cnt = 0;
    std::size_t c_cnt = 0;
    bool inword = false;
    std::size_t wstart = 0;
    for ( std::size_t j = 0, size = data.size(); j < size; j++ )
    {
      char c = data[j];
      if (c == '\n')
        ++l_cnt;
      if (c >= '0' && c <= '9')
      {
      }
      else if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z')
      {
        if (!inword)
        {
          wstart = j;
          inword = true;
          ++w_cnt;
        }
      }
      else if (inword)
      {
        word.assign(&data[wstart], &data[j]);
        ++w_count;
        std::size_t num = w_count%25;
        word.insert(1, 1, allchars[num]);
//        dictionary.insert(dict_value).first->count++;
        inword = false;
      }
      ++c_cnt;
    }

    if (inword)
    {
        word.assign(&data[wstart], &data[data.size()]);
        ++w_count;
        std::size_t num = w_count%25;
        word.insert(1, 1, allchars[num]);
//        dictionary.insert(dict_value).first->count++;
    }
    l_total += l_cnt;
    w_total += w_cnt;
    c_total += c_cnt;
  }

  return 0;
}
*/
