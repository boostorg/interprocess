//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_IUNORDERED_SET_INDEX_HPP
#define BOOST_INTERPROCESS_IUNORDERED_SET_INDEX_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <functional>
#include <utility>
#include <boost/get_pointer.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

/*!\file
   Describes index adaptor of boost::intrusive::unordered_set container, to use it
   as name/shared memory index
*/

namespace boost { namespace interprocess {

/// @cond
/*!Helper class to define typedefs from IndexTraits*/
template <class MapConfig>
struct iunordered_set_index_aux
{
   typedef typename 
      MapConfig::basic_segment_manager                 basic_segment_manager;

   typedef typename 
      basic_segment_manager::void_pointer              void_pointer;

   typedef boost::intrusive::unordered_set_base_hook
      < boost::intrusive::tag
      , boost::intrusive::safe_link
      , void_pointer>                                       derivation_hook;

   typedef typename MapConfig::template 
      intrusive_value_type<derivation_hook>::type           value_type;

   typedef typename MapConfig::
      intrusive_compare_key_type                            intrusive_compare_key_type;

   typedef std::equal_to<value_type>                        value_equal;

   typedef typename MapConfig::char_type                    char_type;

   struct equal_function
   {
      bool operator()(const intrusive_compare_key_type &i, const value_type &b) const
      {  
         return (i.m_len == b.name_length()) &&
                  (std::char_traits<char_type>::compare 
                     (i.mp_str, b.name(), i.m_len) == 0);
      }

      bool operator()(const value_type &b, const intrusive_compare_key_type &i) const
      {  
         return (i.m_len == b.name_length()) &&
                  (std::char_traits<char_type>::compare 
                     (i.mp_str, b.name(), i.m_len) == 0);
      }

      bool operator()(const value_type &b1, const value_type &b2) const
      {  
         return (b1.name_length() == b2.name_length()) &&
                  (std::char_traits<char_type>::compare 
                     (b1.name(), b2.name(), b1.name_length()) == 0);
      }
   };

    struct hash_function
      : std::unary_function<value_type, std::size_t>
    {
        std::size_t operator()(const value_type &val) const
        {
            const char_type *beg = detail::get_pointer(val.name()),
                            *end = beg + val.name_length();
            return boost::hash_range(beg, end);
        }

        std::size_t operator()(const intrusive_compare_key_type &i) const
        {
            const char_type *beg = i.mp_str,
                            *end = beg + i.m_len;
            return boost::hash_range(beg, end);
        }
    };

   typedef boost::intrusive::unordered_set
      <typename derivation_hook::template 
         value_traits<value_type>, hash_function, equal_function, true>  index_t;

   typedef typename index_t::bucket_type                 bucket_type;

   typedef allocator
      <bucket_type, basic_segment_manager>          allocator_type;

   struct allocator_holder
   {
      allocator_holder(basic_segment_manager *mngr)
         :  alloc(mngr)
      {}
      allocator_type alloc;
   };
};
/// @endcond

/*!Index type based in boost::intrusive::set. Just derives from boost::intrusive::set 
   and defines the interface needed by managed memory segments*/
template <class MapConfig>
class iunordered_set_index
      //Derive class from map specialization
   :  private iunordered_set_index_aux<MapConfig>::allocator_holder
   ,  public iunordered_set_index_aux<MapConfig>::index_t
{
   /// @cond
   typedef iunordered_set_index_aux<MapConfig>                 index_aux;
   typedef typename index_aux::index_t                   index_type;
   typedef typename MapConfig::
      intrusive_compare_key_type                         intrusive_compare_key_type;
   typedef typename index_aux::equal_function            equal_function;
   typedef typename index_aux::hash_function             hash_function;
   typedef typename MapConfig::char_type                 char_type;
   typedef typename 
      iunordered_set_index_aux<MapConfig>::allocator_type      allocator_type;
   typedef typename 
      iunordered_set_index_aux<MapConfig>::allocator_holder    allocator_holder;
   /// @endcond

   public:
   typedef typename index_type::iterator                 iterator;
   typedef typename index_type::const_iterator           const_iterator;
   typedef typename index_type::insert_commit_data       insert_commit_data;
   typedef typename index_type::value_type               value_type;
   typedef typename index_type::bucket_ptr               bucket_ptr;
   typedef typename index_type::bucket_type              bucket_type;
   typedef typename index_type::size_type                size_type;

   /// @cond
   private:
   typedef typename index_aux::
      basic_segment_manager             basic_segment_manager;

   enum {   InitBufferSize = 64};

   static bucket_ptr create_buckets(allocator_type &alloc, std::size_t num)
   {
      num = index_type::suggested_upper_bucket_count(num);
      bucket_ptr buckets = alloc.allocate(num);
      bucket_ptr buckets_init = buckets;
      for(std::size_t i = 0; i < num; ++i){
         new(get_pointer(buckets_init++))bucket_type();
      }
      return buckets;
   }

   static void destroy_buckets
      (allocator_type &alloc, bucket_ptr buckets, std::size_t num)
   {
      bucket_ptr buckets_destroy = buckets;
      for(std::size_t i = 0; i < num; ++i){
         get_pointer(buckets_destroy++)->~bucket_type();
      }
      alloc.deallocate(buckets, num);
   }
   /// @endcond

   public:
   /*!Constructor. Takes a pointer to the
      segment manager. Can throw*/
   iunordered_set_index(basic_segment_manager *mngr)
      :  allocator_holder(mngr)
      ,  index_type
            (create_buckets( allocator_holder::alloc
                           , index_type::suggested_upper_bucket_count(InitBufferSize))
                           , index_type::suggested_upper_bucket_count(InitBufferSize))
   {}

   ~iunordered_set_index()
   {  destroy_buckets(this->alloc, index_type::bucket_pointer(), index_type::bucket_count()); }

   /*!This reserves memory to optimize the insertion of n
      elements in the index*/
   void reserve(std::size_t new_n)
   {
      //Let's maintain a 1.0f load factor
      size_type old_n  = this->bucket_count();
      if(new_n <= old_n)
         return;
      bucket_ptr old_p = this->bucket_pointer();
      new_n = index_type::suggested_upper_bucket_count(new_n);
      bucket_ptr new_p = create_buckets(this->alloc, new_n);
      this->rehash(new_p, new_n);
      destroy_buckets(this->alloc, old_p, old_n);
   }

   iterator find(const intrusive_compare_key_type &key)
   {  return index_type::find(key, hash_function(), equal_function());  }

   const_iterator find(const intrusive_compare_key_type &key) const
   {  return index_type::find(key, hash_function(), equal_function());  }

   std::pair<iterator, bool>insert_check
      (const intrusive_compare_key_type &key, insert_commit_data &commit_data)
   {  return index_type::insert_check(key, hash_function(), equal_function(), commit_data); }

   iterator insert_commit(value_type &val, insert_commit_data &commit_data)
   {
      iterator it = index_type::insert_commit(val, commit_data);
      size_type cur_size      = this->size();
      if(cur_size > this->bucket_count()){
         try{
            this->reserve(cur_size);
         }
         catch(...){
            //Strong guarantee: if something goes wrong
            //we should remove the insertion.
            //
            //We can use the iterator because the hash function   
            //can't throw and this means that "reserve" will
            //throw only because of the memory allocation:
            //the iterator has not been invalidated.
            index_type::erase(it);
            throw;
         }
      }
      return it;
   }
};

/// @cond
/*!Trait class to detect if an index is an intrusive
   index.*/
template<class MapConfig>
struct is_intrusive_index
   <boost::interprocess::iunordered_set_index<MapConfig> >
{
   enum{ value = true };
};
/// @endcond

}}   //namespace boost { namespace interprocess {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_IUNORDERED_SET_INDEX_HPP
