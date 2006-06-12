//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2004-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/offset_ptr.hpp>
#include <string>
#include <vector>
#include <algorithm>
#include <string.h>
#include <stdio.h>
#include <cstddef>
#include <new>

using namespace boost::interprocess;

//Explicit instantiation of interprocess::basic_string
//Explicit instantiation of a interprocess::vector of interprocess::strings
typedef allocator<char, managed_shared_memory::segment_manager> ShmemAllocatorChar;
typedef basic_string<char, std::char_traits<char>, ShmemAllocatorChar> ShmString;
typedef allocator<ShmString, managed_shared_memory::segment_manager> ShmemVectorAllocator;
typedef vector<ShmString, ShmemVectorAllocator> ShmStringVector;
template class basic_string<char, std::char_traits<char>, ShmemAllocatorChar>;
template class vector<ShmString, ShmemVectorAllocator>;

typedef std::allocator<char>  StdAllocatorChar;
typedef std::basic_string<char, std::char_traits<char>, StdAllocatorChar> StdString;
typedef std::allocator<StdString> StdVectorAllocator;
typedef vector<StdString, StdVectorAllocator> StdStringVector;

struct StringEqual
{
   bool operator ()(const StdString &stdstring, const ShmString &shmstring) const
   {
      return strcmp(stdstring.c_str(), shmstring.c_str()) == 0; 
   }

   bool operator ()(const ShmString &shmstring, const StdString &stdstring) const
   {
      return strcmp(stdstring.c_str(), shmstring.c_str()) == 0; 
   }
};

//Function to check if both lists are equal
bool CheckEqual(ShmStringVector *shmvect, StdStringVector *stdvect)
{
   return std::equal(stdvect->begin(), stdvect->end(), 
                     shmvect->begin(), StringEqual() );
}

//Function to check if both lists are equal
bool Print(ShmStringVector *shmvect, StdStringVector *stdvect)
{
   std::cout << "shmvect: " << static_cast<unsigned int>(shmvect->size()) << std::endl;
   for(ShmStringVector::iterator beg = shmvect->begin(),
                                 end = shmvect->end();
       beg != end; ++beg){
      std::cout << *beg << std::endl;
   }

   std::cout << "stdvect" << static_cast<unsigned int>(stdvect->size()) << std::endl;
   for(StdStringVector::iterator beg = stdvect->begin(),
                                 end = stdvect->end();
       beg != end; ++beg){
      std::cout << *beg << std::endl;
   }
   return true;
}

int main ()
{
   const int MaxSize = 100;

   //Create shared memory
   managed_shared_memory::remove("MySharedMemory");
   managed_shared_memory segment
         (create_only,
         "MySharedMemory",//segment name
         65536);           //segment size in bytes
   
   ShmemAllocatorChar shmallocator (segment.get_segment_manager());

   //Initialize vector with a range or iterators and allocator
   ShmStringVector *shmStringVect = 
      segment.find_or_construct<ShmStringVector>
                                ("ShmStringVector", std::nothrow)  //object name 
                                (shmallocator);

   StdStringVector *stdStringVect = new StdStringVector;

   ShmString auxShmString (segment.get_segment_manager());
   StdString auxStdString(StdString(auxShmString.begin(), auxShmString.end() ));

   char buffer [20];

   int i;
   for(i = 0; i < MaxSize; ++i){
      auxShmString = "String";
      auxStdString = "String";
      sprintf(buffer, "%i", i);
      auxShmString += buffer;
      auxStdString += buffer;
      shmStringVect->push_back(auxShmString);
      stdStringVect->push_back(auxStdString);
   }

   if(!CheckEqual(shmStringVect, stdStringVect)) return 1;
   
   ShmString shm_swapper(segment.get_segment_manager());
   StdString std_swapper;
   shm_swapper.swap(auxShmString);
   std_swapper.swap(auxStdString);
   if(!StringEqual()(auxShmString, auxStdString)) return 1;   

   shm_swapper.swap(auxShmString);
   std_swapper.swap(auxStdString);

   if(!StringEqual()(auxShmString, auxStdString)) return 1;   

   std::sort(shmStringVect->begin(), shmStringVect->end());
   std::sort(stdStringVect->begin(), stdStringVect->end());

   if(!CheckEqual(shmStringVect, stdStringVect)) return 1;

   const char prefix []    = "Prefix";
   const int  prefix_size  = sizeof(prefix)/sizeof(prefix[0])-1;
   const char sufix []     = "Suffix";

   for(i = 0; i < MaxSize; ++i){
      (*shmStringVect)[i].append(sufix);
      (*stdStringVect)[i].append(sufix);
      (*shmStringVect)[i].insert((*shmStringVect)[i].begin(), 
                                 prefix, prefix + prefix_size);
      (*stdStringVect)[i].insert((*stdStringVect)[i].begin(), 
                                 prefix, prefix + prefix_size);
   }

   if(!CheckEqual(shmStringVect, stdStringVect)) return 1;

   for(i = 0; i < MaxSize; ++i){
      std::reverse((*shmStringVect)[i].begin(), (*shmStringVect)[i].end());
      std::reverse((*stdStringVect)[i].begin(), (*stdStringVect)[i].end());
   }

   if(!CheckEqual(shmStringVect, stdStringVect)) return 1;

   for(i = 0; i < MaxSize; ++i){
      std::reverse((*shmStringVect)[i].begin(), (*shmStringVect)[i].end());
      std::reverse((*stdStringVect)[i].begin(), (*stdStringVect)[i].end());
   }

   if(!CheckEqual(shmStringVect, stdStringVect)) return 1;

   for(i = 0; i < MaxSize; ++i){
      std::sort(shmStringVect->begin(), shmStringVect->end());
      std::sort(stdStringVect->begin(), stdStringVect->end());
   }

   if(!CheckEqual(shmStringVect, stdStringVect)) return 1;

   for(i = 0; i < MaxSize; ++i){
      (*shmStringVect)[i].replace((*shmStringVect)[i].begin(), 
                                  (*shmStringVect)[i].end(),
                                  "String");
      (*stdStringVect)[i].replace((*stdStringVect)[i].begin(), 
                                  (*stdStringVect)[i].end(),
                                  "String");
   }

   if(!CheckEqual(shmStringVect, stdStringVect)) return 1;

   shmStringVect->erase(std::unique(shmStringVect->begin(), shmStringVect->end()),
                        shmStringVect->end());
   stdStringVect->erase(std::unique(stdStringVect->begin(), stdStringVect->end()),
                        stdStringVect->end());
//   Print(shmStringVect, stdStringVect);
   if(!CheckEqual(shmStringVect, stdStringVect)) return 1;

   //When done, delete vector
   segment.destroy<ShmStringVector>("ShmStringVector");
   delete stdStringVect;

   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
