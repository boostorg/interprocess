#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/streams/vectorstream.hpp>
#include <boost/interprocess/streams/bufferstream.hpp>
#include <sstream>
//#include <strstream>
#include <cstring>
#include <vector>
#include <iostream>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/lexical_cast.hpp>
#include <stdio.h>

using namespace boost::interprocess;

//Force instantiations to catch compile-time errors
typedef basic_vectorstream<basic_string<char> > my_stringstream_t;
typedef basic_string<char> my_string;
template class basic_vectorstream<my_string>;
template class basic_vectorstream<std::vector<char> >;

static int vectorstream_test()
{
   //Pre-reserved string
   {
      my_stringstream_t my_stringstream;
      std::stringstream std_stringstream;
      std::string str1, str2, str3("testline:");
      int number1, number2;

      my_stringstream.reserve(10000);
      for(int i = 0; i < 100; ++i){
         my_stringstream  << "testline: " << i << std::endl;
         std_stringstream << "testline: " << i << std::endl;
      }

      if(std::strcmp(my_stringstream.vector().c_str(), std_stringstream.str().c_str()) != 0){
         return 1;
      }

      for(int i = 0; i < 100; ++i){
         my_stringstream  >> str1 >> number1;
         std_stringstream >> str2 >> number2;
         if((str1 != str2) || (str1 != str3)){
            assert(0); return 1;  
         }
         if((number1 != number2) || (number1 != i)){
            assert(0); return 1;   
         }
      }
   }
   //Pre-reserved vector
   {
      basic_vectorstream<std::vector<char> > my_vectorstream;
      std::vector<char> myvector;
      std::stringstream std_stringstream;
      std::string str1, str2, str3("testline:");
      int number1, number2;

      my_vectorstream.reserve(10000);
      for(int i = 0; i < 100; ++i){
         my_vectorstream  << "testline: " << i << std::endl;
         std_stringstream << "testline: " << i << std::endl;
      }
      //Add final null to form a c string
      myvector.push_back(0);
      if(std::strcmp(&(my_vectorstream.vector()[0]), std_stringstream.str().c_str()) != 0){
         return 1;
      }
      myvector.pop_back();
      for(int i = 0; i < 100; ++i){
         my_vectorstream  >> str1 >> number1;
         std_stringstream >> str2 >> number2;
         if((str1 != str2) || (str1 != str3)){
            assert(0); return 1;  
         }
         if((number1 != number2) || (number1 != i)){
            assert(0); return 1;   
         }
      }
   }

   //No pre-reserved or pre-reserved string
   {
      my_stringstream_t my_stringstream;
      std::stringstream std_stringstream;
      std::string str1, str2, str3("testline:");
      int number1, number2;

      for(int i = 0; i < 100; ++i){
         my_stringstream  << "testline: " << i << std::endl;
         std_stringstream << "testline: " << i << std::endl;
      }
      if(std::strcmp(my_stringstream.vector().c_str(), std_stringstream.str().c_str()) != 0){
         assert(0);   return 1;
      }
      for(int i = 0; i < 100; ++i){
         my_stringstream  >> str1 >> number1;
         std_stringstream >> str2 >> number2;
         if((str1 != str2) || (str1 != str3)){
            assert(0); return 1;  
         }
         if((number1 != number2) || (number1 != i)){
            assert(0); return 1;   
         }
      }
   }
   return 0;
}
/*
static int vectorstream_performance_test()
{
   const int NumStrings = 1000000;
   std::string empty_string;
   
   #define TEST_AVOID_SSO

   std::string prefix
      #ifdef TEST_AVOID_SSO
         = "this_is_quite_a_long_prefix_to_avoid_small_string_optimization";
      #else
         = "short";
      #endif


   boost::posix_time::ptime  tini, tend;

   double sprintf_time;

   {
      std::vector<std::string>   str_vector;
      str_vector.resize(NumStrings);
      char buffer[512];

      tini = boost::posix_time::microsec_clock::universal_time();

      //Try with sprintf
      for(int i = 0; i < NumStrings; ++i){
         sprintf(buffer, "%s%d", prefix.c_str(), i);
         str_vector[i] = buffer;
      }

      tend = boost::posix_time::microsec_clock::universal_time();

      std::cout << "sprintf time: " << (tend - tini).total_microseconds() << std::endl;
   }

   sprintf_time = (tend - tini).total_microseconds();

   {
      std::stringstream          sstream;
      std::vector<std::string>   str_vector;
      str_vector.resize(NumStrings);

      tini = boost::posix_time::microsec_clock::universal_time();

      //Try with stringstream
      for(int i = 0; i < NumStrings; ++i){
         sstream << prefix << i << std::ends;
         str_vector[i] = sstream.str();
         sstream.seekp(0);
      }

      tend = boost::posix_time::microsec_clock::universal_time();

      std::cout << "stringstream with seekp time: " << ((tend - tini).total_microseconds()/sprintf_time) << std::endl;
   }

   {
      std::stringstream          sstream;
      std::vector<std::string>   str_vector;
      str_vector.resize(NumStrings);

      tini = boost::posix_time::microsec_clock::universal_time();

      //Try with stringstream
      for(int i = 0; i < NumStrings; ++i){
         sstream.str("");
         sstream << prefix << i << std::ends;
         str_vector[i] = sstream.str();
      }

      tend = boost::posix_time::microsec_clock::universal_time();

      std::cout << "stringstream with str time: " << ((tend - tini).total_microseconds()/sprintf_time) << std::endl;
   }

   {
      std::strstream             stream;
      std::vector<std::string>   str_vector;
      str_vector.resize(NumStrings);

      tini = boost::posix_time::microsec_clock::universal_time();

      //Try with strstream
      for(int i = 0; i < NumStrings; ++i){
         stream << prefix << i << std::ends;
         str_vector[i] = stream.str();
         stream.freeze(false);
         stream.seekp(0);
      }

      tend = boost::posix_time::microsec_clock::universal_time();

      std::cout << "strstream time: " << ((tend - tini).total_microseconds()/sprintf_time) << std::endl;
   }

   {
      std::stringstream          sstream;
      std::vector<std::string>   str_vector;
      str_vector.resize(NumStrings);

      tini = boost::posix_time::microsec_clock::universal_time();

      //Try with lexical cast
      for(int i = 0; i < NumStrings; ++i){
         str_vector[i] += prefix;
         str_vector[i] += boost::lexical_cast<std::string>(i);
      }

      tend = boost::posix_time::microsec_clock::universal_time();

      std::cout << "lexical_cast time: " << ((tend - tini).total_microseconds()/sprintf_time) << std::endl;
   }

   {
      basic_vectorstream<std::vector<char> > vvstream;
      std::vector<std::vector<char> >        vector_vector;
      vector_vector.resize(NumStrings);

      tini = boost::posix_time::microsec_clock::universal_time();

      //Now with vectorstream
      for(int i = 0; i < NumStrings; ++i){
         vvstream.reserve(prefix.size() + 10);
         vvstream << prefix << i << std::ends;
         vvstream.swap_vector(vector_vector[i]);
      }

      tend = boost::posix_time::microsec_clock::universal_time();

      std::cout << "vectorstream time: " << ((tend - tini).total_microseconds()/sprintf_time) << std::endl;

   }

   {
      char formatting_buffer[256];
      bufferstream               bstream(formatting_buffer, 256);
      std::vector<std::string>   string_vector;
      string_vector.resize(NumStrings);

      tini = boost::posix_time::microsec_clock::universal_time();

      //Now with bufferstream
      for(int i = 0; i < NumStrings; ++i){
         bstream << prefix << i << std::ends;
         string_vector[i].assign(formatting_buffer, bstream.tellp());
         bstream.seekp(0);
      }

      tend = boost::posix_time::microsec_clock::universal_time();

      std::cout << "bufferstream time: " << ((tend - tini).total_microseconds()/sprintf_time) << std::endl;
   }

   return 0;
}
*/
int main ()
{
   if(vectorstream_test()==-1){
      return 1;
   }
/*
   if(vectorstream_performance_test()==-1){
      return 1;
   }*/
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
