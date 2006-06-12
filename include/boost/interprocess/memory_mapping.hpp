//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_MEMORY_MAPPING_HPP
#define BOOST_INTERPROCESS_MEMORY_MAPPING_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/noncopyable.hpp>

#include <string>

#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
#  include <boost/interprocess/sync/win32/win32_sync_primitives.hpp>
#else
#  ifdef BOOST_HAS_UNISTD_H
#    include <fcntl.h>
#    include <sys/mman.h>     //mmap, off64_t
#    include <unistd.h>
#    include <sys/stat.h>
#  else
#    error Unknown platform
#  endif

#endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

/*!\file
   Describes memory_mapping and mapped region classes
*/

namespace boost {

namespace interprocess {

class memory_mapping  : private boost::noncopyable
{
   public:
   #if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   typedef void *    mapping_handle_t;
   typedef long long mapping_offset_t;
   enum {   invalid_mapping_handle = 0   };
   #else    //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   typedef int       mapping_handle_t;
   typedef off64_t   mapping_offset_t;
   enum {   invalid_mapping_handle = -1   };
   #endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

   typedef enum { ro_mode = 0x01
                , rw_mode = 0x02
                , access_mode_mask = 0xff 
                } accessmode_t;

   memory_mapping()
      :  m_handle(mapping_handle_t(invalid_mapping_handle))
   {}

   void assign_data(mapping_handle_t handle, accessmode_t mode)
   {  m_handle = handle;   m_mode = mode; }

   mapping_handle_t get_mapping_handle() const
   {  return m_handle;  }

   /*!Returns access mode*/
   accessmode_t get_mode() const
   {  return m_mode; }

   virtual ~memory_mapping() = 0;
   
   private:
   mapping_handle_t  m_handle;
   accessmode_t      m_mode;
};

inline memory_mapping::~memory_mapping()
{}

class mapped_region : private boost::noncopyable
{
   typedef memory_mapping::mapping_offset_t  mapping_offset_t;
   public:
   /*!Creates a mapping region of the mapped memory "mapping", starting in
      offset "offset", and the mapping's size will be "size". The mapping 
      can be opened for read-only "memory_mapping::ro_mode" or read-write 
      "memory_mapping::rw_mode.*/
   mapped_region(const memory_mapping& mapping
                ,memory_mapping::mapping_offset_t offset
                ,std::size_t size
                ,memory_mapping::accessmode_t mode
                ,const void *address = 0);

   /*!Default constructor. Address and size and offset will be 0. Does not throw*/
   mapped_region();

   /*!Destroys the mapped region. Does not throw*/
   ~mapped_region();

   /*!Returns the size of the mapping. Never throws.*/
   std::size_t get_size() const;

   /*!Returns the base address of the mapping. Never throws.*/
   void*       get_address() const;

   /*!Returns the offset of the mapping from the beginning of the
      mapped memory. Never throws.*/
   mapping_offset_t   get_offset() const;

   /*!Flushes to the disk a byte range within the mapped memory. 
      Never throws*/
   bool flush(std::size_t mapping_offset = 0, std::size_t numbytes = 0);

   /*!Swaps the mapped_region with another mapped region*/
   void swap(mapped_region &other);

   friend void swap(mapped_region &x, mapped_region &y)
   {  x.swap(y);  }

   private:
   /*!Closes a previously opened memory mapping. Never throws.*/
   void priv_close();

   void*             m_base;
   std::size_t       m_size;
   mapping_offset_t  m_offset;
   #if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   mapping_offset_t         m_extra_offset;
   #else    //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   #endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
};

inline mapped_region::mapped_region()
   :  m_base(0), m_size(0), m_offset(0)
   #if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
      ,  m_extra_offset(0)
   #else    //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
      // empty
   #endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
{}

inline mapped_region::~mapped_region() 
{  this->priv_close(); }

inline std::size_t mapped_region::get_size()  const  
{  return m_size; }

inline mapped_region::mapping_offset_t  mapped_region::get_offset()  const  
{  return m_offset;   }

inline void*    mapped_region::get_address()  const  
{  return m_base; }

#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

inline mapped_region::mapped_region
   (const memory_mapping &mapping
   ,mapped_region::mapping_offset_t offset
   ,std::size_t size
   ,memory_mapping::accessmode_t mode
   ,const void *address)
   :  m_base(0)
{
   if(size == 0){
      __int64 total_size;
      if(!winapi::get_file_size(mapping.get_mapping_handle(), total_size)){
         error_info err(winapi::get_last_error());
         throw interprocess_exception(err);
      }
      if(total_size > (__int64)((std::size_t)(-1))){
         error_info err(size_error);
         throw interprocess_exception(err);
      }
      size = static_cast<std::size_t>(total_size - offset);
   }

   unsigned long file_map_access = 0;

   //Set accesses
   if (mode == memory_mapping::rw_mode){
      file_map_access   |= winapi::page_readwrite;
   }
   else if (mode == memory_mapping::ro_mode){
      file_map_access   |= winapi::page_readonly;
   }
   else{
      error_info err(mode_error);
      throw interprocess_exception(err);
   }

   //Create file mapping
   memory_mapping::mapping_handle_t hnd = 
      winapi::create_file_mapping
      (mapping.get_mapping_handle(), file_map_access, 0, 0, 0);

   //Check if all is correct
   if(!hnd){
      error_info err = winapi::get_last_error();
      this->priv_close();
      throw interprocess_exception(err);
   }
   //We can't map any offset so we have to obtain system's 
   //memory granularity
   unsigned long granularity = 0;
   unsigned long foffset_low;
   unsigned long foffset_high;

   winapi::system_info info;
   get_system_info(&info);
   granularity = info.dwAllocationGranularity;

   //Now we calculate valid offsets
   foffset_low  = (unsigned long)(offset / granularity) * granularity;
   foffset_high = (unsigned long)(((offset / granularity) * granularity) >> 32);

   //We calculate the difference between demanded and valid offset
   m_extra_offset = (offset - (offset / granularity) * granularity);

   //Store user values in memory
   m_offset = offset;
   m_size   = size;

   unsigned long map_access = 0;

   if (mode == memory_mapping::rw_mode){
      map_access        |= winapi::file_map_write;
   }
   else if (mode == memory_mapping::ro_mode){
      map_access        |= winapi::file_map_read;
   }
   else{
      error_info err(mode_error);
      throw interprocess_exception(err);
   }

   //Map with new offsets and size
   m_base = winapi::map_view_of_file_ex
                               (hnd,
                                map_access, 
                                foffset_high,
                                foffset_low, 
                                static_cast<std::size_t>(m_extra_offset + m_size), 
                                (void*)address);

   //We don't need the file mapping anymore
   winapi::close_handle(hnd);
   //Check error
   if(!m_base){
      error_info err = winapi::get_last_error();
      this->priv_close();
      throw interprocess_exception(err);
   }
   //Calculate new base for the user
   m_base = reinterpret_cast<char*>(m_base) + m_extra_offset;
}

inline bool mapped_region::flush(std::size_t mapping_offset, std::size_t numbytes)
{
   //Check some errors
   if(m_base == 0)
      return false;

   if(mapping_offset >= m_size || (mapping_offset+numbytes)> m_size){
      return false;
   }

   if(numbytes == 0){
      numbytes = m_size-mapping_offset;
   }
   //Flush it all
   return 0 == winapi::flush_view_of_file
      (reinterpret_cast<char*>(m_base)+mapping_offset, 
       static_cast<std::size_t>(numbytes));
}

inline void mapped_region::priv_close()
{
   if(m_base){
      this->flush();
      winapi::unmap_view_of_file(reinterpret_cast<char*>(m_base) - m_extra_offset);
      m_base = 0;
   }
}

#else    //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

inline mapped_region::mapped_region
   (const memory_mapping &mapping,
   mapped_region::mapping_offset_t offset,
   std::size_t size,
   memory_mapping::accessmode_t mode,
   const void *address)
   :  m_base(MAP_FAILED)
{
   if(size == 0){
      mapping_offset_t filesize = lseek64(mapping.get_mapping_handle(), offset, SEEK_END);
      
      if(filesize == -1 ){
         error_info err(system_error_code());
         throw interprocess_exception(err);
      }
      if(offset >= filesize){
         error_info err(size_error);
         throw interprocess_exception(err);
      }
      filesize -= offset;
      
      size = (size_t)filesize;
      if((mapping_offset_t)size != filesize){
         error_info err(size_error);
         throw interprocess_exception(err);         
      }
   }

   //Create new mapping
   int prot    = PROT_READ;

   if(mode == memory_mapping::ro_mode){
   }
   else if(mode == memory_mapping::rw_mode){
      prot  |= PROT_WRITE;
   }
   else{
      error_info err(mode_error);
      throw interprocess_exception(err);
   }

   //Map it to the address space
   m_base   = mmap64( (void*)address
                    , size
                    , prot
                    , MAP_SHARED
                    , mapping.get_mapping_handle()
                    , offset);
   m_offset = offset;
   m_size   = size;

   //Check if mapping was successful
   if(m_base == MAP_FAILED){
      error_info err = system_error_code();
      this->priv_close();
      throw interprocess_exception(err);
   }
}

inline bool mapped_region::flush(std::size_t mapping_offset, std::size_t numbytes)
{
   if(mapping_offset >= m_size || (mapping_offset+numbytes)> m_size){
      return false;
   }

   if(numbytes == 0){
      numbytes = m_size - mapping_offset;
   }
   //Flush it all
   return msync(reinterpret_cast<char*>(m_base)+mapping_offset, 
                numbytes, MS_SYNC) == 0;
}

inline void mapped_region::priv_close()
{
   if(m_base != MAP_FAILED){
      this->flush();
      munmap(m_base, m_size);
      m_base = MAP_FAILED;
   }
}

#endif   //##if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

inline void mapped_region::swap(mapped_region &other)
{
   detail::do_swap(this->m_base, other.m_base);
   detail::do_swap(this->m_size, other.m_size);
   detail::do_swap(this->m_offset, other.m_offset);
   #if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   detail::do_swap(this->m_extra_offset,     other.m_extra_offset);
   #else
   #endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
}



/*!No-op functor*/
struct null_mapped_region_function
{
   bool operator()(const mapped_region &region, bool) const
      {   return true;   }
};


}  //namespace interprocess {

}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_MEMORY_MAPPING_HPP

