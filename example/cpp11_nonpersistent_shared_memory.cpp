//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#if defined(BOOST_INTERPROCESS_BSD_DERIVATIVE) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

//[doc_managed_nonpersistent_shared_memory
#include <boost/interprocess/managed_nonpersistent_shared_memory.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <cstring>
#include <cstdlib>
#include <string>
#include <iostream>
#include <semaphore.h>

namespace {
   struct semaphore final {
      semaphore(char const* pszName) noexcept
         : m_psem(sem_open(pszName, O_CREAT, /* accessible by current user only */ S_IRUSR|S_IWUSR, 1))
      {}
      
      ~semaphore() {
         sem_close(m_psem);
      }
      
      void wait() const& noexcept {
         sem_wait(m_psem);
      }

      void post() const& noexcept {
         sem_post(m_psem);
      }
	private:
		sem_t* m_psem;
   };

   struct SShared final {
      char m_sz[12];
   };
}

int main(int argc, char *argv[])
{
   using namespace boost::interprocess;

   bool bFirstProcess = false;
   basic_managed_nonpersistent_shared_memory<char, rbtree_best_fit<mutex_family>, iset_index> shm(
      open_or_create, 
      "MySharedMemory",
      1000,
      [&]() noexcept {
         std::cout << "Shared memory segment created. Resetting semaphore.\n";
         bFirstProcess = true;
         if(-1==sem_unlink("MySharedSemaphore")) {
            if(EINVAL!=errno && ENOENT!=errno) {
               std::cerr << "Deleting semaphore returned unexpected error.\n";
               std::exit(1);
            }
         }
      }
   );

   semaphore sem("MySharedSemaphore");

   sem.wait();

   SShared* pshared = shm.find_or_construct<SShared>("A")();
   char const str[] = { "Hello World" };

   //Map the whole shared memory in this process
   if(bFirstProcess) {
      std::cout << "Acquired semaphore. Writing to shared memory.\n";
      std::copy(std::begin(str), std::end(str), pshared->m_sz);
      
      sem.post(); // release semaphore before starting process

      std::cout << "Start child process.\n";
      if(0 != std::system(argv[0])) {
          std::cerr << "Child process failed.\n";
          return 1;
      }
   } else {
      std::cout << "Acquired semaphore. Reading shared memory.\n";
      std::cout << pshared->m_sz << "\n";
      
      bool const bEqual = std::equal(std::begin(str), std::end(str), pshared->m_sz);
      sem.post(); 

      if(!bEqual) {
         std::cerr << "Error reading shared memory\n";
         return 1;
      }
   }

   return 0;
}
//]

#else //BOOST_INTERPROCESS_BSD_DERIVATIVE

int main()
{
   return 0;
}

#endif //BOOST_INTERPROCESS_BSD_DERIVATIVE

#include <boost/interprocess/detail/config_end.hpp>
