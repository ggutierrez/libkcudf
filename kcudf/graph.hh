/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main authors:
 *     Yves Jaradin <yves.jaradin@uclouvain.be>
 *     Gustavo Gutierrez <gutierrez.gustavo@uclouvain.be>
 *
 *  Copyright:
 *     Yves Jaradin, 2010
 *     Gustavo Gutierrez, 2010
 *
 *  Last modified:
 *     $Date$ by $Author$
 *     $Revision$
 *
 *
 *  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __KCUDF_GRAPH__HH__
#define __KCUDF_GRAPH__HH__

/**
 * \file In this project we need some graph implementations in several parts. Then, to
 * not fill entire files with template code defining boost graphs, all the definitions
 * and some inline implementations are here.
 */

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/iterator/transform_iterator.hpp>

/**
 * \brief Boost graph type definitions for a graph with integer labels in the nodes.
 *
 * Additionally an internal property map associating nodes to integers from 0 to
 * the number of nodes in the graph.
 *
 * To get the types for a directed graph just pass \a boost::bidirectionalS as template
 * parameter. On the other hand to get an undirected version use \a boost::undirectedS
 */
template<class D>
class Bgraph {
private:
  /// \name Type definition for information stored in the graph
  //@{
  typedef boost::property <boost::vertex_name_t,int> Name;
  typedef boost::property <boost::vertex_index_t,std::size_t,Name> Index;
  //@}
public:
  /// \name Type definition for the graph
  //@{
  typedef
      typename boost::adjacency_list<boost::setS,boost::listS,D,Index> type;
  typedef
      typename boost::property_map<type,boost::vertex_name_t>::type map_type;
  //@}
  /// \name Graph related types
  //@{
  /// Traits for the graph
  typedef typename boost::graph_traits<type> traits;
  /// Vertex type
  typedef typename traits::vertex_descriptor vertex;
  /// Edge type
  typedef typename traits::edge_descriptor edge;
  /// Iterator on edges type
  typedef typename traits::edge_iterator edge_iterator;
  /// Iterator on out edges type
  typedef typename traits::out_edge_iterator out_edge_iterator;
  /// Iterator on in edges type
  typedef typename traits::in_edge_iterator in_edge_iterator;
  /// Iterator on vertices
  typedef typename traits::vertex_iterator vertex_iterator;
  //@}
  /// \name Type definition for vertex information
  //@{
  /// The indexes of nodes in the graph
  typedef Index index;
  /// The label of the nodes in the graph
  typedef Name node;
  //@}
  /// \name Vertex operations
  //@{
  /// Add a node with label \a lb and index \a idx to \a g
  static vertex add_vertex(unsigned int idx, unsigned int lb, type& g) {
    return boost::add_vertex(Index(idx,Name(lb)),g);
  }
  /// Get the index from a node in \a g
  static unsigned int get_index(vertex v, const type& g) {
    return boost::get(boost::vertex_index,g)[v];
  }
  /// Get the index from a node in \a g
  static unsigned int get_label(vertex v, const type& g) {
    return boost::get(boost::vertex_name,g)[v];
  }
  //@}
};

/// Types definition for a directed graph
typedef Bgraph<boost::bidirectionalS> Digraph;

/// Types definition for an undirected graph
typedef Bgraph<boost::undirectedS> Ugraph;

template <class GraphTraits, class Info>
class extract_vertex_info {
public:
  typedef unsigned int type;
  type operator()(const typename GraphTraits::vertex& v,
                  const typename GraphTraits::type& g)  {
    return get(Info(),g)[v];
  }
};

template <class GraphTraits, class Info>
class extract_edge_info {
public:
  typedef std::pair<unsigned int, unsigned int> type;
  type operator()(const typename GraphTraits::edge& v,
                  const typename GraphTraits::type& g)  {
    return std::make_pair(get(Info(),g)[source(v,g)],
                          get(Info(),g)[target(v,g)]);
  }
};

template <class GraphTraits, class Info>
class extract_edge_source {
public:
  typedef typename Info::type type;
  type operator()(const typename GraphTraits::edge& v,
                  const typename GraphTraits::type& g)  {
    Info i;
    return i(source(v,g),g);
  }
};

template <class GraphTraits, class Info>
class extract_edge_target {
public:
  typedef typename Info::type type;
  type operator()(const typename GraphTraits::edge& v,
                  const typename GraphTraits::type& g)  {
    Info i;
    return i(target(v,g),g);
  }
};

template <class GraphTraits>
class extract_labels {
public:
  typedef std::pair<unsigned int,unsigned int> type;
  type operator()(const typename GraphTraits::edge& v,
                  const typename GraphTraits::type& g)  {
    unsigned int s(get(boost::vertex_index,g)[source(v,g)]),
                 t(get(boost::vertex_index,g)[target(v,g)]);
    return std::make_pair(s,t);
  }
};


/**
 * \brief Simple class to avoid writing a pair of iterators [begin,end) all over the
 * place.
 *
 * Just look at this as a class that takes a type and returns a pair of types type.
 */
template <class T>
class type_pair {
public:
  typedef std::pair<T,T> type;
};

/**
 * \brief Class to adapt a vertex graph iterator to iterate on the information stored
 * in nodes of the graph.
 *
 * A description of the template parameters:
 * - \a Iterator the type of a boost graph iterator on vertices of the graph
 * - \a Graph is the typpe of the graph on which the iterator was created
 * - \a Prop is the type of the property needed from the node (it can be either
 *   \a boost::vertex_name_t or \a boost::vertex_index_t
 */
template<class Iterator, class GraphTraits, class Apply>
class custom_info_iterator
  : public boost::iterator_adaptor<custom_info_iterator<Iterator,GraphTraits,Apply>,
                                   Iterator,
                                   boost::use_default,
                                   boost::use_default,
                                   typename Apply::type>
{
private:
  /// Type of the iterator adaptor
  typedef boost::iterator_adaptor<custom_info_iterator<Iterator,GraphTraits,Apply>,
                                  Iterator,
                                  boost::use_default,
                                  boost::use_default,
                                  typename Apply::type> super_t;
  friend class boost::iterator_core_access;
public:
  /// Default constructor
  custom_info_iterator(void) : g(NULL) {}
  /// Constructor from iterator \a x and graph \a gr
  explicit custom_info_iterator(Iterator x, const typename GraphTraits::type& gr)
    : super_t(x), g(&gr) {}
protected:
  /// Redefinition (adaptation) of the dereference opertor of the iterator
  typename super_t::reference dereference() const {
    // take the current value of the original iterator
    typename Iterator::value_type rt = *(this->base());
    Apply a;
    return a(rt,*g);
  }
  /// The graph on which the iterator was created (needed to get the property map from it)
  const typename GraphTraits::type* const g;
};

template <class GraphTraits>
class vit_extract {
private:
  typedef boost::vertex_name_t label_;
  typedef boost::vertex_index_t index_;
public:
  typedef extract_vertex_info<GraphTraits,label_> label;
  typedef extract_vertex_info<GraphTraits,index_> index;
  typedef extract_edge_info<GraphTraits,label_> elabel;
  typedef extract_edge_info<GraphTraits,index_> eindex;
  typedef extract_edge_source<GraphTraits,
                              extract_vertex_info<GraphTraits,label_> > source_info;
  typedef extract_edge_target<GraphTraits,
                              extract_vertex_info<GraphTraits,label_> > target_info;
};

template <class GraphTraits, class Iterator, class Filter>
class vit_adaptor {
public:
  typedef
      typename
        type_pair<boost::iterator_adaptor<
          custom_info_iterator<Iterator,GraphTraits,Filter>,
                               Iterator,
                               boost::use_default,
                               boost::use_default,
                               typename Filter::type>
                               >::type
  type;
};
/**
 * \brief Creates an iterator [begin,end) on the labels of all the vertices of \a g
 */
template <class GraphTraits>
typename vit_adaptor<GraphTraits,typename GraphTraits::vertex_iterator,typename vit_extract<GraphTraits>::label >::type
make_vertex_label_iterator(const typename GraphTraits::type& g) {
  typedef typename GraphTraits::vertex_iterator it_type;
  it_type begin, end;
  boost::tie(begin,end) = vertices(g);
  return make_pair(custom_info_iterator<it_type,GraphTraits,typename vit_extract<GraphTraits>::label >(begin,g),
                   custom_info_iterator<it_type,GraphTraits,typename vit_extract<GraphTraits>::label >(end,g));

}
/**
 * \brief Creates an iterator [begin,end) on the indexes of all the vertices of \a g
 */
template <class GraphTraits>
typename vit_adaptor<GraphTraits,typename GraphTraits::vertex_iterator,typename vit_extract<GraphTraits>::index >::type
make_vertex_index_iterator(const typename GraphTraits::type& g) {
  typedef typename GraphTraits::vertex_iterator it_type;
  it_type begin, end;
  boost::tie(begin,end) = vertices(g);
  return make_pair(custom_info_iterator<it_type,GraphTraits,typename vit_extract<GraphTraits>::index >(begin,g),
                   custom_info_iterator<it_type,GraphTraits,typename vit_extract<GraphTraits>::index >(end,g));

}

/**
 * \brief Creates an iterator [begin,end) on the labels of all the edges of \a g
 */
template <class GraphTraits>
typename vit_adaptor<GraphTraits,typename GraphTraits::edge_iterator,typename vit_extract<GraphTraits>::elabel>::type
make_edge_label_iterator(const typename GraphTraits::type& g) {
  typedef typename GraphTraits::edge_iterator it_type;
  it_type begin, end;
  boost::tie(begin,end) = edges(g);
  return make_pair(custom_info_iterator<it_type,GraphTraits,typename vit_extract<GraphTraits>::elabel>(begin,g),
                   custom_info_iterator<it_type,GraphTraits,typename vit_extract<GraphTraits>::elabel>(end,g));

}
/**
 * \brief Creates an iterator [begin,end) on the edges of \a g
 */
template <class GraphTraits>
typename vit_adaptor<GraphTraits,typename GraphTraits::out_edge_iterator,typename vit_extract<GraphTraits>::target_info>::type
make_edge_target_label_iterator(const typename GraphTraits::type& g) {
  typedef typename GraphTraits::edge_iterator it_type;
  it_type begin, end;
  boost::tie(begin,end) = edges(g);
  return make_pair(custom_info_iterator<it_type,GraphTraits,typename vit_extract<GraphTraits>::target_info>(begin,g),
                   custom_info_iterator<it_type,GraphTraits,typename vit_extract<GraphTraits>::target_info>(end,g));
}
/**
 * \brief Creates an iterator [begin,end) on the out edges of \a v in \a g
 */
template <class GraphTraits>
typename vit_adaptor<GraphTraits,typename GraphTraits::out_edge_iterator,typename vit_extract<GraphTraits>::target_info>::type
make_out_edge_label_iterator(const typename GraphTraits::vertex v, const typename GraphTraits::type& g) {
  typedef typename GraphTraits::out_edge_iterator it_type;
  it_type begin, end;
  boost::tie(begin,end) = out_edges(v,g);
  return make_pair(custom_info_iterator<it_type,GraphTraits,typename vit_extract<GraphTraits>::target_info>(begin,g),
                   custom_info_iterator<it_type,GraphTraits,typename vit_extract<GraphTraits>::target_info>(end,g));
}

/**
 * \brief Creates an iterator [begin,end) on the in edges of \a v in \a g
 */
template <class GraphTraits>
typename vit_adaptor<GraphTraits,typename GraphTraits::in_edge_iterator,typename vit_extract<GraphTraits>::source_info>::type
make_in_edge_label_iterator(const typename GraphTraits::vertex v, const typename GraphTraits::type& g) {
  typedef typename GraphTraits::in_edge_iterator it_type;
  it_type begin, end;
  boost::tie(begin,end) = in_edges(v,g);
  return make_pair(custom_info_iterator<it_type,GraphTraits,typename vit_extract<GraphTraits>::source_info>(begin,g),
                   custom_info_iterator<it_type,GraphTraits,typename vit_extract<GraphTraits>::source_info>(end,g));
}

#endif
