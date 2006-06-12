//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_FWD_HPP
#define BOOST_INTERPROCESS_FWD_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#if BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
// old Dinkumware
#  include <boost/compatibility/cpp_c_headers/cstddef>
#else
#  include <cstddef>
#endif

//////////////////////////////////////////////////////////////////////////////
//                        Standard predeclarations
//////////////////////////////////////////////////////////////////////////////

namespace std {

template <class T>
class allocator;

template <class T>
struct less;

template <class T1, class T2>
struct pair;

template <class CharType> 
struct char_traits;

}  //namespace std {

namespace boost { namespace interprocess {

//////////////////////////////////////////////////////////////////////////////
//                            shared_memory
//////////////////////////////////////////////////////////////////////////////

class shared_memory;

//////////////////////////////////////////////////////////////////////////////
//              mapped file/mapped region/mapped_file
//////////////////////////////////////////////////////////////////////////////

class file_mapping;
class mapped_region;
class mapped_file;

//////////////////////////////////////////////////////////////////////////////
//                               Mutexes
//////////////////////////////////////////////////////////////////////////////

class null_mutex;

class interprocess_mutex;
class interprocess_recursive_mutex;

class named_mutex;
class named_recursive_mutex;

class interprocess_semaphore;
class named_semaphore;

//////////////////////////////////////////////////////////////////////////////
//                         Mutex families
//////////////////////////////////////////////////////////////////////////////

struct mutex_family;
struct null_mutex_family;

//////////////////////////////////////////////////////////////////////////////
//                   Other synchronization classes
//////////////////////////////////////////////////////////////////////////////

class barrier;
class interprocess_sharable_mutex;
class interprocess_condition;

//////////////////////////////////////////////////////////////////////////////
//                              Locks
//////////////////////////////////////////////////////////////////////////////

template <class Mutex>
class scoped_lock;

template <class SharableMutex>
class sharable_lock;

//////////////////////////////////////////////////////////////////////////////
//                      STL compatible allocators
//////////////////////////////////////////////////////////////////////////////

template<class T, class SegmentManager>
class allocator;

template<class T, std::size_t N, class SegmentManager>
class node_allocator;

template<class T, std::size_t N, class SegmentManager>
class private_node_allocator;

template<class T, class SegmentManager>
class cached_node_allocator;

//////////////////////////////////////////////////////////////////////////////
//                            offset_ptr
//////////////////////////////////////////////////////////////////////////////

template <class T>
class offset_ptr;

//////////////////////////////////////////////////////////////////////////////
//                         intersegment_ptr
//////////////////////////////////////////////////////////////////////////////

template <class Mutex>
struct flat_map_intersegment;

template <class T/*, class PT = flat_map_intersegment<interprocess_mutex> */>
class intersegment_ptr;

//////////////////////////////////////////////////////////////////////////////
//                    Memory allocation algorithms
//////////////////////////////////////////////////////////////////////////////

//Single segment memory allocation algorithms
template<class MutexFamily, class VoidMutex = offset_ptr<void> >
class seq_fit;

template<class MutexFamily, class VoidMutex = offset_ptr<void> >
class simple_seq_fit;

//Single segment memory allocation algorithms
template<class MutexFamily, class VoidMutex = intersegment_ptr<void> >
class multi_seq_fit;

template<class MutexFamily, class VoidMutex = intersegment_ptr<void> >
class multi_simple_seq_fit;

//////////////////////////////////////////////////////////////////////////////
//                         Index Types
//////////////////////////////////////////////////////////////////////////////

template<class IndexConfig> class flat_map_index;
template<class IndexConfig> class map_index;
template<class IndexConfig> class null_index;

//////////////////////////////////////////////////////////////////////////////
//                         Segment manager
//////////////////////////////////////////////////////////////////////////////

template <class CharType
         ,class MemoryAlgorithm
         ,template<class IndexConfig> class IndexType>
class segment_manager;

//////////////////////////////////////////////////////////////////////////////
//                  External buffer front-ends
//////////////////////////////////////////////////////////////////////////////

template <class CharType
         ,class MemoryAlgorithm
         ,template<class IndexConfig> class IndexType>
class basic_managed_external_buffer;

typedef basic_managed_external_buffer
   <char
   ,simple_seq_fit<null_mutex_family>
   ,flat_map_index>
managed_external_buffer;

typedef basic_managed_external_buffer
   <wchar_t
   ,simple_seq_fit<null_mutex_family>
   ,flat_map_index>
wmanaged_external_buffer;

//////////////////////////////////////////////////////////////////////////////
//                      Shared memory front-ends
//////////////////////////////////////////////////////////////////////////////

template <class CharType
         ,class MemoryAlgorithm
         ,template<class IndexConfig> class IndexType>
class basic_managed_shared_memory;

typedef basic_managed_shared_memory 
   <char
   ,simple_seq_fit<mutex_family>
   ,flat_map_index>
managed_shared_memory;

typedef basic_managed_shared_memory
   <wchar_t
   ,simple_seq_fit<mutex_family>
   ,flat_map_index>
wmanaged_shared_memory;

//////////////////////////////////////////////////////////////////////////////
//                      Fixed address shared memory
//////////////////////////////////////////////////////////////////////////////

typedef basic_managed_shared_memory
   <char
   ,simple_seq_fit<mutex_family, void *>
   ,flat_map_index>
fixed_managed_shared_memory;

typedef basic_managed_shared_memory
   <wchar_t
   ,simple_seq_fit<mutex_family, void *>
   ,flat_map_index>
wfixed_managed_shared_memory;

//////////////////////////////////////////////////////////////////////////////
//                      Heap memory front-ends
//////////////////////////////////////////////////////////////////////////////

template
   <class CharType
   ,class MemoryAlgorithm
   ,template<class IndexConfig> class IndexType>
class basic_managed_heap_memory;

typedef basic_managed_heap_memory
   <char
   ,simple_seq_fit<null_mutex_family>
   ,flat_map_index>
managed_heap_memory;

typedef basic_managed_heap_memory
   <wchar_t
   ,simple_seq_fit<null_mutex_family>
   ,flat_map_index>
wmanaged_heap_memory;

//////////////////////////////////////////////////////////////////////////////
//                         Mapped file front-ends
//////////////////////////////////////////////////////////////////////////////

template
   <class CharType
   ,class MemoryAlgorithm
   ,template<class IndexConfig> class IndexType>
class basic_managed_mapped_file;

typedef basic_managed_mapped_file
   <char
   ,simple_seq_fit<mutex_family>
   ,flat_map_index>
managed_mapped_file;

typedef basic_managed_mapped_file
   <wchar_t
   ,simple_seq_fit<mutex_family>
   ,flat_map_index>
wmanaged_mapped_file;

//////////////////////////////////////////////////////////////////////////////
//                            Exceptions
//////////////////////////////////////////////////////////////////////////////

class interprocess_exception;
class lock_exception;
class bad_alloc;

//////////////////////////////////////////////////////////////////////////////
//                            Bufferstream
//////////////////////////////////////////////////////////////////////////////

//bufferstream
template <class CharT
         ,class CharTraits = std::char_traits<CharT> >
class basic_bufferbuf;

template <class CharT
         ,class CharTraits = std::char_traits<CharT> >
class basic_ibufferstream;

template <class CharT
         ,class CharTraits = std::char_traits<CharT> >
class basic_obufferstream;

template <class CharT
         ,class CharTraits = std::char_traits<CharT> >
class basic_bufferstream;

//////////////////////////////////////////////////////////////////////////////
//                            Vectorstream
//////////////////////////////////////////////////////////////////////////////

template <class CharVector
         ,class CharTraits = std::char_traits<typename CharVector::value_type> >
class basic_vectorbuf;

template <class CharVector
         ,class CharTraits = std::char_traits<typename CharVector::value_type> >
class basic_ivectorstream;

template <class CharVector
         ,class CharTraits = std::char_traits<typename CharVector::value_type> >
class basic_ovectorstream;

template <class CharVector
         ,class CharTraits = std::char_traits<typename CharVector::value_type> >
class basic_vectorstream;

//////////////////////////////////////////////////////////////////////////////
//                             Smart pointers
//////////////////////////////////////////////////////////////////////////////

template<class T, class Deleter>
class scoped_ptr;

template<class T, class VoidPointer>
class intrusive_ptr;

//////////////////////////////////////////////////////////////////////////////
//                                  IPC
//////////////////////////////////////////////////////////////////////////////

class message_queue;

//////////////////////////////////////////////////////////////////////////////
//                             Containers
//////////////////////////////////////////////////////////////////////////////

//vector class
template <class T
         ,class A = std::allocator<T> >
class vector;

//list class
template <class T
         ,class A = std::allocator<T> >
class list;

//slist class
template <class T
         ,class Alloc = std::allocator<T> >
class slist;

//set class
template <class T
         ,class Pred  = std::less<T>
         ,class Alloc = std::allocator<T> >
class set;

//multiset class
template <class T
         ,class Pred  = std::less<T>
         ,class Alloc = std::allocator<T> >
class multiset;

//map class
template <class Key
         ,class T
         ,class Pred  = std::less<Key>
         ,class Alloc = std::allocator<std::pair<const Key, T> > >
class map;

//multimap class
template <class Key
         ,class T
         ,class Pred  = std::less<Key>
         ,class Alloc = std::allocator<std::pair<const Key, T> > >
class multimap;

//flat_set class
template <class T
         ,class Pred  = std::less<T>
         ,class Alloc = std::allocator<T> >
class flat_set;

//flat_multiset class
template <class T
         ,class Pred  = std::less<T>
         ,class Alloc = std::allocator<T> >
class flat_multiset;

//flat_map class
template <class Key
         ,class T
         ,class Pred  = std::less<Key>
         ,class Alloc = std::allocator<std::pair<Key, T> > >
class flat_map;

//flat_multimap class
template <class Key
         ,class T
         ,class Pred  = std::less<Key>
         ,class Alloc = std::allocator<std::pair<Key, T> > >
class flat_multimap;

//basic_string class
template <class CharT
         ,class Traits = std::char_traits<CharT>
         ,class Alloc  = std::allocator<CharT> > 
class basic_string;

//string class
typedef basic_string
   <char
   ,std::char_traits<char>
   ,std::allocator<char> >
string;

}}  //namespace boost { namespace interprocess {

#include <boost/interprocess/detail/config_end.hpp>

#endif //#ifndef BOOST_INTERPROCESS_FWD_HPP

