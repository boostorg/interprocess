//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztañaga 2007. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_NODE_TOOLS_HPP
#define BOOST_INTERPROCESS_DETAIL_NODE_TOOLS_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#  pragma warning (disable : 4503)
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/intrusive/slist.hpp>

namespace boost {
namespace interprocess {
namespace detail {


template<class VoidPointer>
struct node_slist
{
   //This hook will be used to chain the individual nodes
   typedef boost::intrusive::slist_base_hook
      <boost::intrusive::tag, boost::intrusive::normal_link, VoidPointer> slist_hook_t;

   //A node object will hold node_t when it's not allocated
   struct node_t
      :  public slist_hook_t
   {};

   typedef boost::intrusive::slist
      <typename slist_hook_t::template value_traits<node_t> > node_slist_t;
};

}  //namespace detail {
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_DETAIL_NODE_TOOLS_HPP
