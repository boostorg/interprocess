///////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_ALLOCATOR_HPP
#define BOOST_INTERPROCESS_ALLOCATOR_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/allocators/allocation_type.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/version_type.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/assert.hpp>
#include <boost/type_traits/alignment_of.hpp>

#include <memory>
#include <algorithm>
#include <cstddef>
#include <stdexcept>

/*!\file
   Describes an allocator that allocates portions of fixed size
   memory buffer (shared memory, mapped file...)
*/

namespace boost {

namespace interprocess {

/*!An STL compatible allocator that uses a segment manager as 
   memory source. The internal pointer type will of the same type (raw, smart) as
   "typename SegmentManager::void_pointer" type. This allows
   placing the allocator in shared memory, memory mapped-files, etc...*/
template<class T, class SegmentManager>
class allocator 
{
   /// @cond
   private:
   /*!Self type*/
   typedef allocator<T, SegmentManager>   self_t;

   /*!Segment manager*/
   typedef SegmentManager                 segment_manager;

   /*!Pointer to void */
   typedef typename segment_manager::void_pointer  aux_pointer_t;

   /*!Typedef to const void pointer */
   typedef typename 
      detail::pointer_to_other
         <aux_pointer_t, const void>::type   cvoid_ptr;

   /*!Pointer to the allocator*/
   typedef typename detail::pointer_to_other
      <cvoid_ptr, segment_manager>::type     alloc_ptr_t;

   /*!Not assignable from related allocator*/
   template<class T2, class SegmentManager2>
   allocator& operator=(const allocator<T2, SegmentManager2>&);

   /*!Not assignable from other allocator*/
   allocator& operator=(const allocator&);

   /*!Pointer to the allocator*/
   alloc_ptr_t mp_mngr;
   /// @endcond

   public:
   typedef T                                    value_type;
   typedef typename detail::pointer_to_other
      <cvoid_ptr, T>::type                      pointer;
   typedef typename detail::
      pointer_to_other<pointer, const T>::type  const_pointer;
   typedef typename workaround::random_it
      <value_type>::reference                   reference;
   typedef typename workaround::random_it
      <value_type>::const_reference             const_reference;
   typedef std::size_t                          size_type;
   typedef std::ptrdiff_t                       difference_type;

   typedef detail::version_type<allocator, 2>   version;

   /*!Obtains an allocator of other type*/
   template<class T2>
   struct rebind
   {   
      typedef allocator<T2, SegmentManager>     other;
   };

   /*!Returns the segment manager. Never throws*/
   segment_manager* get_segment_manager()const
   {  return detail::get_pointer(mp_mngr);   }

   /*!Returns address of mutable object. Never throws*/
   pointer address(reference value) const
   {  return pointer(boost::addressof(value));  }

   /*!Returns address of non mutable object. Never throws*/
   const_pointer address(const_reference value) const
   {  return const_pointer(boost::addressof(value));  }

   /*!Constructor from the segment manager. Never throws*/
   allocator(segment_manager *segment_mngr) 
      : mp_mngr(segment_mngr) { }

   /*!Constructor from other allocator. Never throws*/
   allocator(const allocator &other) 
      : mp_mngr(other.get_segment_manager()){ }

   /*!Constructor from related allocator. Never throws*/
   template<class T2>
   allocator(const allocator<T2, SegmentManager> &other) 
      : mp_mngr(other.get_segment_manager()){}

   /*!Allocates memory for an array of count elements. 
      Throws boost::interprocess::bad_alloc if there is no enough memory*/
   pointer allocate(size_type count, cvoid_ptr hint = 0)
   {
      (void)hint;
      if(count > ((size_type)-1)/sizeof(value_type))
         throw bad_alloc();
      return pointer((value_type*)mp_mngr->allocate(count*sizeof(value_type)));
   }

   /*!Deallocates memory previously allocated. Never throws*/
   void deallocate(const pointer &ptr, size_type)
   {  mp_mngr->deallocate(detail::get_pointer(ptr));  }

   /*!Returns the number of elements that could be allocated. Never throws*/
   size_type max_size() const
   {  return mp_mngr->get_size()/sizeof(value_type);   }

   /*!Swap segment manager. Does not throw. If each allocator is placed in
      different memory segments, the result is undefined.*/
   friend void swap(self_t &alloc1, self_t &alloc2)
   {  detail::do_swap(alloc1.mp_mngr, alloc2.mp_mngr);   }

   //Experimental version 2 allocator functions

   std::pair<pointer, bool>
      allocation_command(allocation_type command,
                         size_type limit_size, 
                         size_type preferred_size,
                         size_type &received_size, const pointer &reuse = 0)
   {
      std::pair<pointer, bool> ret;
      size_type max_count = ((size_type)-1)/sizeof(value_type);
      if(limit_size > max_count || preferred_size > max_count){
         if(command & nothrow_allocation){
            throw bad_alloc();
         }
         else{
            ret.first = 0;
            return ret;
         }
      }

      std::size_t l_size = limit_size*sizeof(value_type);
      std::size_t p_size = preferred_size*sizeof(value_type);
      std::size_t r_size;
      std::pair<void *, bool> result =
         mp_mngr->allocation_command
            (command, l_size, p_size, r_size, detail::get_pointer(reuse), sizeof(value_type));
      received_size = r_size/sizeof(value_type);
      BOOST_ASSERT(0 == ((std::size_t)result.first % boost::alignment_of<value_type>::value));
      return std::pair<pointer, bool> 
         (static_cast<value_type*>(result.first), result.second);
   }

   /*!Returns maximum the number of objects the previously allocated memory
      pointed by p can hold.*/
   size_type size(const pointer &p) const
   {  
      return (size_type)mp_mngr->size
         (detail::get_pointer(p))/sizeof(value_type);
   }

   /*!Allocates just one object. Memory allocated with this function
      must be deallocated only with deallocate_one().
      Throws boost::interprocess::bad_alloc if there is no enough memory*/
   pointer allocate_one()
   {  return this->allocate(1);  }

   /*!Deallocates memory previously allocated with allocate_one().
      You should never use deallocate_one to deallocate memory allocated
      with other functions different from allocate_one(). Never throws*/
   void deallocate_one(const pointer &p)
   {  return this->deallocate(p, 1);  }
};

/*!Equality test for same type of allocator*/
template<class T, class SegmentManager> inline
bool operator==(const allocator<T , SegmentManager>  &alloc1, 
                const allocator<T, SegmentManager>  &alloc2)
   {  return alloc1.get_segment_manager() == alloc2.get_segment_manager(); }

/*!Inequality test for same type of allocator*/
template<class T, class SegmentManager> inline
bool operator!=(const allocator<T, SegmentManager>  &alloc1, 
                const allocator<T, SegmentManager>  &alloc2)
   {  return alloc1.get_segment_manager() != alloc2.get_segment_manager(); }

}  //namespace interprocess {

/// @cond

template<class T>
struct has_trivial_destructor;

template<class T, class SegmentManager>
struct has_trivial_destructor
   <boost::interprocess::allocator <T, SegmentManager> >
   :  public ::boost::true_type
{};
/// @endcond

}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_ALLOCATOR_HPP

