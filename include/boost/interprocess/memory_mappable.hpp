//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_memory_mappable_HPP
#define BOOST_INTERPROCESS_memory_mappable_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/utilities.hpp>
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
   Describes memory_mappable and mapped region classes
*/

namespace boost {

namespace interprocess {

/*!A base class for all objects that can be memory mapped. The derived
   classes must initialize this base class with a mapping handle and the
   mode of the mapping handle.*/
class memory_mappable
{
   //Non-copyable
   memory_mappable(const memory_mappable &);
   memory_mappable &operator=(const memory_mappable &);

   public:
   #if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   typedef void *    mapping_handle_t;
   typedef long long mapping_offset_t;
   enum {   invalid_handle = 0   };
   #else    //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   typedef int       mapping_handle_t;
   typedef off64_t   mapping_offset_t;
   enum {   invalid_handle = -1   };
   #endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

   typedef enum { read_only = 0x01
                , read_write = 0x02
                , invalid_mode = 0xff 
                } accessmode_t;

   /*!Constructor. Initializes internal mapping handle to an invalid value.
      Does not throw.*/
   memory_mappable()
      :  m_handle(mapping_handle_t(invalid_handle))
   {}

   /*!Stores the mapping handle and the mode of the handle. This method must be called
      by the derived class. The assigned handle will be used to create mapped regions.
      Does not throw.*/
   void assign_data(mapping_handle_t handle, accessmode_t mode)
   {  m_handle = handle;   m_mode = mode; }

   /*!Stores the mapping handle and the mode of the handle. This method must be called
      by the derived class and the assigned handle will be used to create mapped regions.
      memory_mappable does not duplicate the handle using operating system functions. Just
      stores the handle internally. Does not throw.*/
   mapping_handle_t get_mapping_handle() const
   {  return m_handle;  }

   /*!Returns the registered access mode. Does not throw.*/
   accessmode_t get_mode() const
   {  return m_mode; }

   /*!Virtual pure destructor. memory_mappable does not destroy the handle. This must
      be done by the derived class. Does not throw.*/
   virtual ~memory_mappable() = 0;
   
   private:
   mapping_handle_t  m_handle;
   accessmode_t      m_mode;
};

inline memory_mappable::~memory_mappable()
{}

/*!The mapped_region class represents a portion or region created from a
   memory_mappable object.*/
class mapped_region
{
   //Non-copyable
   mapped_region(const mapped_region &);
   mapped_region &operator=(const mapped_region &);

   typedef memory_mappable::mapping_offset_t  mapping_offset_t;

   public:

   #if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   typedef long long offset_t;
   #else    //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   typedef off64_t   offset_t;
   #endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

   typedef enum { read_only      = 0x01
                , read_write     = 0x02
                , copy_on_write  = 0x03
                , invalid_mode   = 0xff 
                } accessmode_t;


   /*!Creates a mapping region of the mapped memory "mapping", starting in
      offset "offset", and the mapping's size will be "size". The mapping 
      can be opened for read-only "memory_mappable::read_only" or read-write 
      "memory_mappable::read_write.*/
   mapped_region(const memory_mappable& mapping
                ,mapped_region::accessmode_t mode
                ,offset_t offset = 0
                ,std::size_t size = 0
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
   offset_t   get_offset() const;

   /*!Flushes to the disk a byte range within the mapped memory. 
      Never throws*/
   bool flush(std::size_t mapping_offset = 0, std::size_t numbytes = 0);

   /*!Swaps the mapped_region with another mapped region*/
   void swap(mapped_region &other);

   static std::size_t get_page_size();

   private:
   /*!Closes a previously opened memory mapping. Never throws.*/
   void priv_close();

   void*             m_base;
   std::size_t       m_size;
   offset_t          m_offset;
//   #if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   offset_t          m_extra_offset;
//   #else    //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
//   #endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
};

inline void swap(mapped_region &x, mapped_region &y)
{  x.swap(y);  }

inline mapped_region::mapped_region()
   :  m_base(0), m_size(0), m_offset(0)
//   #if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
      ,  m_extra_offset(0)
//   #else    //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
      // empty
//   #endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
{}

inline mapped_region::~mapped_region() 
{  this->priv_close(); }

inline std::size_t mapped_region::get_size()  const  
{  return m_size; }

inline mapped_region::offset_t  mapped_region::get_offset()  const  
{  return m_offset;   }

inline void*    mapped_region::get_address()  const  
{  return m_base; }

#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

inline mapped_region::mapped_region
   (const memory_mappable &mapping
   ,mapped_region::accessmode_t mode
   ,mapped_region::offset_t offset
   ,std::size_t size
   ,const void *address)
   :  m_base(0)
{
   //Update mapping size if the user does not specify it
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

   //Set accesses
   unsigned long file_map_access = 0;
   unsigned long map_access = 0;

   switch(mode)
   {
      case read_only:
         file_map_access   |= winapi::page_readonly;
         map_access        |= winapi::file_map_read;
      break;
      case read_write:
         file_map_access   |= winapi::page_readwrite;
         map_access        |= winapi::file_map_write;
      break;
      case copy_on_write:
         file_map_access   |= winapi::page_writecopy;
         file_map_access   |= winapi::file_map_copy;
      break;
      default:
         {
            error_info err(mode_error);
            throw interprocess_exception(err);
         }
      break;
   }

   //Create file mapping
   memory_mappable::mapping_handle_t hnd = 
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

   //Update the mapping address
   if(address){
      address = static_cast<const char*>(address) - m_extra_offset;
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
   m_base = static_cast<char*>(m_base) + m_extra_offset;
}

inline bool mapped_region::flush(std::size_t mapping_offset, std::size_t numbytes)
{
   //Check some errors
   if(m_base == 0)
      return false;

   if(mapping_offset >= m_size || (mapping_offset + numbytes) > m_size){
      return false;
   }

   //Update flush size if the user does not provide it
   if(numbytes == 0){
      numbytes = m_size - mapping_offset;
   }

   //Flush it all
   return 0 == winapi::flush_view_of_file
      (static_cast<char*>(m_base)+mapping_offset, 
       static_cast<std::size_t>(numbytes));
}

inline void mapped_region::priv_close()
{
   if(m_base){
      this->flush();
      winapi::unmap_view_of_file(static_cast<char*>(m_base) - m_extra_offset);
      m_base = 0;
   }
}

inline std::size_t get_page_size()
{
   winapi::system_info info;
   get_system_info(&info);
   return std::size_t(info.dwAllocationGranularity);
}

#else    //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

inline mapped_region::mapped_region
   (const memory_mappable &mapping,
   mapped_region::accessmode_t mode,
   mapped_region::mapping_offset_t offset,
   std::size_t size,
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
   int prot    = 0;
   int flags   = 0;

   switch(mode)
   {
      case mapped_region::read_only:
         prot  |= PROT_READ;
         flags |= MAP_SHARED;
      break;
   
      case mapped_region::read_write:
         prot  |= (PROT_WRITE | PROT_READ);
         flags |= MAP_SHARED;
      break;

      case mapped_region::copy_on_write:
         prot  |= PROT_READ;
         flags |= MAP_PRIVATE;
      break;
   
      default:
         {
            error_info err(mode_error);
            throw interprocess_exception(err);
         }
      break;
   }

   //We calculate the difference between demanded and valid offset
   std::size_t page_size = this->get_page_size();
   m_extra_offset = (offset - (offset / page_size) * page_size);

   //Store user values in memory
   m_offset = offset;
   m_size   = size;

   //Update the mapping address
   if(address){
      address = static_cast<const char*>(address) - m_extra_offset;
   }

   //Map it to the address space
   m_base   = mmap64( (void*)address
                    , static_cast<std::size_t>(m_extra_offset + m_size)
                    , prot
                    , flags
                    , mapping.get_mapping_handle()
                    , offset);

   //Check if mapping was successful
   if(m_base == MAP_FAILED){
      error_info err = system_error_code();
      this->priv_close();
      throw interprocess_exception(err);
   }

   //Check for fixed mapping error
   if(address && (m_base != (void*)address)){
      error_info err = system_error_code();
      this->priv_close();
      throw interprocess_exception(err);
   }

   //Calculate new base for the user
   m_base = static_cast<char*>(m_base) + m_extra_offset;
   m_offset = offset;
   m_size   = size;
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
   return msync(static_cast<char*>(m_base)+mapping_offset, 
                numbytes, MS_SYNC) == 0;
}

inline void mapped_region::priv_close()
{
   if(m_base != MAP_FAILED){
      this->flush();
      munmap(static_cast<char*>(m_base) - m_extra_offset, m_size + m_extra_offset);
      m_base = MAP_FAILED;
   }
}

inline std::size_t mapped_region::get_page_size()
{  return std::size_t(sysconf(_SC_PAGESIZE)); }

#endif   //##if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)

inline void mapped_region::swap(mapped_region &other)
{
   detail::do_swap(this->m_base, other.m_base);
   detail::do_swap(this->m_size, other.m_size);
   detail::do_swap(this->m_offset, other.m_offset);
//   #if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
   detail::do_swap(this->m_extra_offset,     other.m_extra_offset);
//   #else
//   #endif   //#if (defined BOOST_WINDOWS) && !(defined BOOST_DISABLE_WIN32)
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

#endif   //BOOST_INTERPROCESS_memory_mappable_HPP

